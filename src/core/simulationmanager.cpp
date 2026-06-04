#include "SimulationManager.h"
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

namespace core {

// 构造函数
SimulationManager::SimulationManager(QObject* parent)
    : QObject(parent)
    , m_state(SimulationState::Idle)
    , m_currentTime(0.0)
    , m_simulationSpeed(1.0)
    , m_isPlaying(false)
{
    qDebug() << "仿真管理器初始化...";

    // 创建仿真定时器
    m_simulationTimer.reset(new QTimer(this));
    m_simulationTimer->setInterval(16); // 约60Hz刷新率

    // 连接定时器信号
    connect(m_simulationTimer.get(), &QTimer::timeout,
            this, &SimulationManager::onTimerTimeout);
}

// 析构函数
SimulationManager::~SimulationManager()
{
    qDebug() << "仿真管理器销毁开始...";

    // 停止仿真
    stopSimulation();

    qDebug() << "仿真管理器销毁完成";
}

// 生成轨迹 - 使用QtConcurrent多线程
void SimulationManager::generateTrajectory(const FlightParams& params)
{
   qDebug() << "=== 调用真正的Ruckig库生成轨迹 ===";

    // 检查是否在GUI线程
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        qWarning() << "警告：generateTrajectory 在非GUI线程中被调用";
    }

    try {
        // 更新状态
        updateSimulationState(SimulationState::Generating);
        qDebug() << "状态更新完成";

        // 保存当前参数
        {
            QMutexLocker locker(&m_dataMutex);
            m_currentParams = params;
        }
        qDebug() << "参数保存完成";

        // 发射开始信号
        emit trajectoryGenerationStarted();
        qDebug() << "开始信号发射完成";

        // 调用真正的TrajectoryGenerator生成轨迹
        TrajectoryResult result = m_trajectoryGenerator.generateTrajectory(params);

        qDebug() << "轨迹数据生成完成，成功:" << result.success
                 << "，点数:" << result.points.size();

        if (!result.success) {
            qCritical() << "轨迹生成失败:" << result.errorMessage;
            updateSimulationState(SimulationState::Error);
            emit errorOccurred("轨迹生成失败: " + result.errorMessage);
            emit trajectoryGenerationFinished(false);
            return;
        }

        // 保存结果
        {
            QMutexLocker locker(&m_dataMutex);
            m_currentTrajectory = result;
        }

        m_currentTime = 0.0;

        // 更新状态
        updateSimulationState(SimulationState::Ready);
        qDebug() << "状态更新为Ready";

        // 发射信号
        emit trajectoryGenerated(result);
        qDebug() << "轨迹生成信号发射完成";

        emit trajectoryGenerationFinished(true);
        qDebug() << "轨迹完成信号发射完成";

    } catch (const std::exception& e) {
        qCritical() << "generateTrajectory异常:" << e.what();
        updateSimulationState(SimulationState::Error);
        emit errorOccurred(QString("异常: %1").arg(e.what()));
        emit trajectoryGenerationFinished(false);
    } catch (...) {
        qCritical() << "generateTrajectory未知异常";
        updateSimulationState(SimulationState::Error);
        emit errorOccurred("未知异常");
        emit trajectoryGenerationFinished(false);
    }

    qDebug() << "=== 轨迹生成完成 ===";
}

// 开始仿真
void SimulationManager::startSimulation()
{
    SimulationState currentState = m_state.load();

    // 检查状态是否允许开始
    if (currentState != SimulationState::Ready && currentState != SimulationState::Paused) {
        qWarning() << "当前状态无法开始仿真，状态:" << static_cast<int>(currentState);
        return;
    }

    // 检查是否有轨迹数据
    if (m_currentTrajectory.points.isEmpty()) {
        emit errorOccurred("无可用的轨迹数据，请先生成轨迹");
        return;
    }

    // 更新状态
    updateSimulationState(SimulationState::Running);
    m_isPlaying = true;

    // 启动定时器
    m_simulationTimer->start();

    // 发射信号
    emit simulationStarted();

    qDebug() << "仿真开始，总时长:" << m_currentTrajectory.duration << "s";
}

    void SimulationManager::pauseSimulation()
{
    SimulationState currentState = m_state.load();

    if (currentState != SimulationState::Running) {
        qWarning() << "当前状态无法暂停，状态:" << static_cast<int>(currentState);
        return;
    }

    // 更新状态
    updateSimulationState(SimulationState::Paused);
    m_isPlaying = false;

    // 停止定时器
    m_simulationTimer->stop();

    // 发射信号
    emit simulationPaused();

    qDebug() << "仿真暂停，当前时间:" << m_currentTime << "s";
}

// 继续仿真
void SimulationManager::resumeSimulation()
{
    SimulationState currentState = m_state.load();

    if (currentState != SimulationState::Paused) {
        return;
    }

    // 更新状态
    updateSimulationState(SimulationState::Running);
    m_isPlaying = true;

    // 重启定时器
    m_simulationTimer->start();

    // 发射信号
    emit simulationResumed();

    qDebug() << "仿真继续，当前时间:" << m_currentTime << "s";
}

// 停止仿真
void SimulationManager::stopSimulation()
{
    SimulationState currentState = m_state.load();

    if (currentState == SimulationState::Idle || currentState == SimulationState::Stopped) {
        return;
    }

    // 停止定时器
    m_simulationTimer->stop();

    // 重置时间
    m_currentTime = 0.0;
    m_isPlaying = false;

    // 更新状态
    updateSimulationState(SimulationState::Stopped);

    // 发射信号
    emit simulationStopped();
    emit simulationTimeUpdated(0.0);

    // 如果之前是运行或暂停状态，发射重置信号
    if (currentState == SimulationState::Running || currentState == SimulationState::Paused) {
        emit simulationReset();
    }

    qDebug() << "仿真停止，已重置";
}

// 重置仿真
void SimulationManager::resetSimulation()
{
    // 先停止仿真
    stopSimulation();

    // 清除轨迹数据
    {
        QMutexLocker locker(&m_dataMutex);
        m_currentTrajectory.clear();
    }

    // 更新状态
    updateSimulationState(SimulationState::Idle);

    qDebug() << "仿真重置完成";
}

// 获取当前状态
SimulationState SimulationManager::getState() const
{
    return m_state.load();
}

// 获取当前参数
const FlightParams& SimulationManager::getCurrentParams() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_currentParams;
}

// 获取当前轨迹
const TrajectoryResult& SimulationManager::getCurrentTrajectory() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_currentTrajectory;
}

// 获取指定时间的状态
TrajectoryPoint SimulationManager::getCurrentState(double time) const
{
    QMutexLocker locker(&m_dataMutex);

    if (m_currentTrajectory.points.isEmpty()) {
        return TrajectoryPoint();
    }

    // 查找最接近的时间点
    if (time <= 0.0) {
        return m_currentTrajectory.points.first();
    }

    if (time >= m_currentTrajectory.duration) {
        return m_currentTrajectory.points.last();
    }

    // 线性插值查找
    for (int i = 0; i < m_currentTrajectory.points.size() - 1; ++i) {
        const auto& p1 = m_currentTrajectory.points[i];
        const auto& p2 = m_currentTrajectory.points[i + 1];

        if (p1.time <= time && p2.time >= time) {
            double alpha = (time - p1.time) / (p2.time - p1.time);

            TrajectoryPoint result;
            result.time = time;

            // 位置线性插值
            result.x = p1.x + (p2.x - p1.x) * alpha;
            result.y = p1.y + (p2.y - p1.y) * alpha;
            result.z = p1.z + (p2.z - p1.z) * alpha;

            // 速度线性插值
            result.vx = p1.vx + (p2.vx - p1.vx) * alpha;
            result.vy = p1.vy + (p2.vy - p1.vy) * alpha;
            result.vz = p1.vz + (p2.vz - p1.vz) * alpha;

            // 加速度线性插值
            result.ax = p1.ax + (p2.ax - p1.ax) * alpha;
            result.ay = p1.ay + (p2.ay - p1.ay) * alpha;
            result.az = p1.az + (p2.az - p1.az) * alpha;

            result.roll = p1.roll + (p2.roll - p1.roll) * alpha;
            result.pitch = p1.pitch + (p2.pitch - p1.pitch) * alpha;
            result.yaw = p1.yaw + (p2.yaw - p1.yaw) * alpha;
            result.rollRate = p1.rollRate + (p2.rollRate - p1.rollRate) * alpha;
            result.pitchRate = p1.pitchRate + (p2.pitchRate - p1.pitchRate) * alpha;
            result.yawRate = p1.yawRate + (p2.yawRate - p1.yawRate) * alpha;

            return result;
        }
    }

    return m_currentTrajectory.points.last();
}

// 获取仿真时间
double SimulationManager::getSimulationTime() const
{
    return m_currentTime;
}

// 获取轨迹时长
double SimulationManager::getTrajectoryDuration() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_currentTrajectory.duration;
}

// 槽：定时器超时 - 仿真更新
void SimulationManager::onTimerTimeout()
{
    if (!m_isPlaying) {
        return;
    }

    // 获取当前状态确保线程安全
    SimulationState currentState = m_state.load();
    if (currentState != SimulationState::Running) {
        return;
    }

    // 计算时间增量
    double deltaTime = m_simulationTimer->interval() * m_simulationSpeed / 1000.0; // 转换为秒

    // 更新当前时间
    double duration = getTrajectoryDuration();
    m_currentTime += deltaTime;

    // 检查是否到达轨迹终点
    if (m_currentTime >= duration) {
        m_currentTime = duration;
        pauseSimulation(); // 到达终点后自动暂停

        qDebug() << "仿真完成，到达轨迹终点";
    }

    // 获取当前状态
    TrajectoryPoint currentStatePoint = getCurrentState(m_currentTime);

    // 发射更新信号
    emit simulationTimeUpdated(m_currentTime);
    emit realtimeStateUpdated(currentStatePoint);

    // 计算进度
    if (duration > 0) {
        double progress = m_currentTime / duration;
        emit progressUpdated(progress);
    }
}

// 更新仿真状态
void SimulationManager::updateSimulationState(SimulationState newState)
{
    SimulationState oldState = m_state.exchange(newState);

    if (oldState != newState) {
        emit stateChanged(newState);
        qDebug() << "仿真状态变更: " << static_cast<int>(oldState)
                 << "->" << static_cast<int>(newState);
    }
}

// 清理工作线程（保持但可能不再需要）
void SimulationManager::cleanupWorkerThread()
{

}

} // namespace core
