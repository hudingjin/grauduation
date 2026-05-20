#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

//管理仿真状态和控制流程

#include "FlightParams.h"
#include "TrajectoryGenerator.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <atomic>
#include <memory>

namespace core {

// 仿真状态枚
//定义仿真系统的状态机，控制仿真流程的各个阶段。
enum class SimulationState {
    Idle,           // 空闲
    Generating,     // 正在生成轨迹
    Ready,          // 轨迹就绪
    Running,        // 仿真运行中
    Paused,         // 仿真暂停
    Stopped,        // 仿真停止
    Error           // 错误状态
};

// 仿真控制信号
//定义用户可以执行的仿真控制命令
enum class ControlCommand {
    Generate,       // 生成轨迹
    Start,          // 开始仿真
    Pause,          // 暂停仿真
    Resume,         // 继续仿真
    Stop,           // 停止仿真
    Reset           // 重置
};

class SimulationManager : public QObject
{
    Q_OBJECT

public:
    explicit SimulationManager(QObject* parent = nullptr);
    ~SimulationManager();

    // 仿真控制
    void generateTrajectory(const FlightParams& params);//启动轨迹生成（在后台线程执行）使用QThread或QtConcurrent避免界面卡顿
    void startSimulation();
    void pauseSimulation();
    void resumeSimulation();
    void stopSimulation();
    void resetSimulation();

    // 状态查询
    SimulationState getState() const;//获取当前仿真状态，返回原子变量的值，无需加锁

    //获取当前的参数和轨迹数据
    const FlightParams& getCurrentParams() const;
    const TrajectoryResult& getCurrentTrajectory() const;

    // 实时状态获取
    TrajectoryPoint getCurrentState(double time) const;//通过时间插值获取指定时刻的飞行状态，支持实时状态查询和图表更新

    //获取仿真时间和轨迹总时长
    double getSimulationTime() const;
    double getTrajectoryDuration() const;

signals:
    // 通知界面状态变化和错误信息，状态变化信号
    void stateChanged(SimulationState newState);
    void errorOccurred(const QString& error);

    // 轨迹相关信号，轨迹生成过程的开始、完成、结果通知
    void trajectoryGenerated(const TrajectoryResult& result);
    void trajectoryGenerationStarted();
    void trajectoryGenerationFinished(bool success);

    // 仿真播放信号，仿真控制操作的结果通知
    void simulationStarted();
    void simulationPaused();
    void simulationResumed();
    void simulationStopped();
    void simulationReset();

    // 实时更新信号，实时发射仿真状态，驱动界面更新
    void simulationTimeUpdated(double time);
    void realtimeStateUpdated(const TrajectoryPoint& state);
    void progressUpdated(double progress);  // 0.0 ~ 1.0

private slots:
    // 内部槽函数，处理异步事件的槽函数，连接工作线程和主线程的信号槽
   // void onTrajectoryGenerated(const TrajectoryResult& result);
    //void onGenerationError(const QString& error);
    void onTimerTimeout();

private:

    class SimpleWorker
    {
    public:
        SimpleWorker(const FlightParams& params, TrajectoryGenerator& generator)
            : m_params(params), m_generator(generator) {}

        TrajectoryResult doWork() {
            return m_generator.generateTrajectory(m_params);
        }

    private:
        FlightParams m_params;
        TrajectoryGenerator& m_generator;
    };

    // 内部工作函数
    void doGenerateTrajectory(const FlightParams& params);//在工作线程中执行轨迹生成
    void updateSimulationState(SimulationState newState);//安全更新状态并发射信号
    void cleanupWorkerThread();

    // 成员变量
    std::atomic<SimulationState> m_state{SimulationState::Idle};//存储当前仿真状态，使用原子操作保证线程安全,默认值idle
    std::unique_ptr<QThread> m_workerThread;//用于执行耗时的轨迹生成任务，避免界面冻结
    std::unique_ptr<QTimer> m_simulationTimer;//驱动仿真播放的定时器，控制仿真步进

    mutable QMutex m_dataMutex;//保护共享数据（参数、轨迹）的线程安全访问
    FlightParams m_currentParams;
    TrajectoryResult m_currentTrajectory;//存储生成的轨迹数据点

    TrajectoryGenerator m_trajectoryGenerator;//封装Ruckig轨迹生成算法

    // 仿真播放相关
    double m_currentTime{0.0};//记录仿真已运行的时间
    double m_simulationSpeed{1.0};  //控制播放速度（1.0=正常速度）
    bool m_isPlaying{false};//播放状态标志

    Q_DISABLE_COPY(SimulationManager)
};

} // namespace core

#endif // SIMULATIONMANAGER_H