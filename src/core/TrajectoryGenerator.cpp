#include "TrajectoryGenerator.h"
#include <QDebug>
#include <QElapsedTimer>
#include <cmath>
#include <mutex>

// 包含Ruckig头文
#ifdef HAVE_RUCKIG
    #include "ruckig/ruckig.hpp"
#endif

namespace core {

TrajectoryGenerator::TrajectoryGenerator(QObject* parent)
    : QObject(parent)
{
    qDebug() << "轨迹生成器初始化...";
}

bool TrajectoryGenerator::isRuckigAvailable() const
{
#ifdef HAVE_RUCKIG
    return true;
#else
    qWarning() << "Ruckig未启用，请检查构建配置";
    return false;
#endif
}

TrajectoryResult TrajectoryGenerator::generateTrajectory(const FlightParams& params)
{
    qDebug() << "TrajectoryGenerator::generateTrajectory 被调用";

    TrajectoryResult result;

    try {
        // 根据DOF选择不同的生成算
        int dof = params.getConfig().dof;

        switch (dof) {
        case 1:
            result = generate1DOF(params);
            break;
        case 3:
            result = generate3DOF(params);
            break;
        case 6:
            result = generate6DOF(params);
            break;
        default:
            result.errorMessage = QString("不支持的DOF: %1").arg(dof);
            break;
        }

        if (result.success) {
            qDebug() << "轨迹生成成功，点" << result.points.size()
                     << "，时" << result.duration << "s";
        } else {
            qCritical() << "轨迹生成失败:" << result.errorMessage;
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = QString("轨迹生成异常: %1").arg(e.what());
        qCritical() << result.errorMessage;
    } catch (...) {
        result.success = false;
        result.errorMessage = "未知异常";
        qCritical() << result.errorMessage;
    }

    // 保存结果
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastResult = result;
    }

    return result;
}

static ruckig::Synchronization parseSyncMode(const QString& mode)
{
    if (mode.contains("PHASE", Qt::CaseInsensitive))
        return ruckig::Synchronization::Phase;
    if (mode.contains("NONE", Qt::CaseInsensitive))
        return ruckig::Synchronization::None;
    // TIME (default) and TimeIfNecessary both map to Time for simplicity
    return ruckig::Synchronization::Time;
}

TrajectoryResult TrajectoryGenerator::generate1DOF(const FlightParams& params)
{
    TrajectoryResult result;
    result.dof = 1;
    
#ifdef HAVE_RUCKIG
    try {
        // 创建Ruckig计算
        ruckig::Ruckig<1> otg{params.getConfig().deltaT};
        ruckig::InputParameter<1> input;
        ruckig::OutputParameter<1> output;
        ruckig::Trajectory<1> trajectory;
        
        // 设置输入参数
        input.current_position = {params.getStartState().x};
        input.current_velocity = {params.getStartState().vx};
        input.current_acceleration = {0.0};
        
        input.target_position = {params.getTargetState().x};
        input.target_velocity = {params.getTargetState().vx};
        input.target_acceleration = {0.0};
        
        input.max_velocity = {params.getLimits().velX};
        input.max_acceleration = {params.getLimits().accX};
        input.max_jerk = {params.getLimits().jerkX};
        input.synchronization = parseSyncMode(params.getConfig().syncMode);
        
        // 计算轨迹
        auto calcResult = otg.calculate(input, trajectory);
        
        if (calcResult == ruckig::Result::ErrorInvalidInput) {
            result.errorMessage = "输入参数无效";
            return result;
        }
        
        if (calcResult == ruckig::Result::ErrorTrajectoryDuration) {
            result.errorMessage = "无法在约束条件下到达目标";
            return result;
        }
        
        // 获取轨迹时长
        result.duration = trajectory.get_duration();
        
        // 采样轨迹
        const double samplingTime = params.getConfig().deltaT;
        const int numSamples = static_cast<int>(result.duration / samplingTime) + 1;
        
        result.points.reserve(numSamples);
        
        for (int i = 0; i <= numSamples; ++i) {
            double time = i * samplingTime;
            if (time > result.duration) time = result.duration;
            
            std::array<double, 1> new_position, new_velocity, new_acceleration;
            trajectory.at_time(time, new_position, new_velocity, new_acceleration);
            
            TrajectoryPoint point;
            point.time = time;
            point.x = new_position[0];
            point.vx = new_velocity[0];
            point.ax = new_acceleration[0];
            
            result.points.append(point);
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = QString("1-DOF生成错误: %1").arg(e.what());
    }
#else
    // 如果没有Ruckig，使用简单线性插值（仅用于测试）
    qWarning() << "Ruckig未启用，使用1-DOF简单线性插值";
    
    const auto& start = params.getStartState();
    const auto& target = params.getTargetState();
    const auto& limits = params.getLimits();
    
    // 简化计算：匀速运
    double distance = target.x - start.x;
    double maxSpeed = limits.velX;
    
    result.duration = std::abs(distance) / maxSpeed;
    if (result.duration < 0.1) result.duration = 1.0;  // 最小时
    
    const double samplingTime = params.getConfig().deltaT;
    const int numSamples = static_cast<int>(result.duration / samplingTime) + 1;
    
    for (int i = 0; i <= numSamples; ++i) {
        double time = i * samplingTime;
        if (time > result.duration) time = result.duration;
        
        double t = time / result.duration;
        TrajectoryPoint point;
        point.time = time;
        point.x = start.x + distance * t;
        point.vx = distance / result.duration;
        point.ax = 0.0;
        
        result.points.append(point);
    }
    
    result.success = true;
#endif
    
    return result;
}

TrajectoryResult TrajectoryGenerator::generate3DOF(const FlightParams& params)
{
    TrajectoryResult result;
    result.dof = 3;
    
#ifdef HAVE_RUCKIG
    qDebug() << "=== 进入Ruckig 3-DOF计算分支 ===";
    qDebug() << "HAVE_RUCKIG已定义，使用Ruckig 3-DOF";
    try {
        // 验证Ruckig输入参数的合理
        if (!validateRuckigParameters(params)) {
            result.errorMessage = "Ruckig参数验证失败";
            qCritical() << result.errorMessage;
            return result;
        }

        // 验证时间步长
        double deltaT = params.getConfig().deltaT;
        if (deltaT <= 0) {
            result.errorMessage = QString("无效的时间步 %1").arg(deltaT);
            qCritical() << result.errorMessage;
            return result;
        }

        // 调试输出参数
        const auto& start = params.getStartState();
        const auto& target = params.getTargetState();
        const auto& limits = params.getLimits();

        qDebug() << "=== Ruckig 3-DOF轨迹计算 ===";
        qDebug() << "时间步长:" << deltaT * 1000.0 << "ms (" <<deltaT << "s)";
        qDebug() << "起始位置: (" << start.x << "," << start.y << "," << start.z << ")";
        qDebug() << "目标位置: (" << target.x << "," << target.y << "," << target.z << ")";
        qDebug() << "速度约束: (" << limits.velX << "," << limits.velY << "," << limits.velZ << ")";
        qDebug() << "加度约束: (" << limits.accX << "," << limits.accY << "," << limits.accZ << ")";
        // 创建3-DOF Ruckig计算
        ruckig::Ruckig<3> otg{params.getConfig().deltaT};
        ruckig::InputParameter<3> input;
        ruckig::Trajectory<3> trajectory;
        
        // 设置起始状态
        //const auto& start = params.getStartState();
        input.current_position = {start.x, start.y, start.z};
        input.current_velocity = {start.vx, start.vy, start.vz};
        input.current_acceleration = {0.0, 0.0, 0.0};
        
        // 设置目标状态
       // const auto& target = params.getTargetState();
        input.target_position = {target.x, target.y, target.z};
        input.target_velocity = {target.vx, target.vy, target.vz};
        input.target_acceleration = {0.0, 0.0, 0.0};
        
        // 设置约束
        //const auto& limits = params.getLimits();
        input.max_velocity = {limits.velX, limits.velY, limits.velZ};
        input.max_acceleration = {limits.accX, limits.accY, limits.accZ};
        input.max_jerk = {limits.jerkX, limits.jerkY, limits.jerkZ};
        input.synchronization = parseSyncMode(params.getConfig().syncMode);
        
        // 计算轨迹
        auto calcResult = otg.calculate(input, trajectory);
        qDebug() << "Ruckig计算结果枚举" << static_cast<int>(calcResult);
        if (calcResult != ruckig::Result::Working) {
            QString errorStr;
            switch (calcResult) {
            case ruckig::Result::ErrorInvalidInput:
                errorStr = "输入参数无效";
                break;
            case ruckig::Result::ErrorPositionalLimits:
                errorStr = "位置限制错误";
                break;
            case ruckig::Result::ErrorTrajectoryDuration:
                errorStr = "无法在约束条件下到达目标";
                break;
            default:
                // 使用通用的错误处理，避免特定枚举
                    if (static_cast<int>(calcResult) == 4) {  // ErrorExecutionTime的
                        errorStr = "计算超时";
                    } else {
                        errorStr = QString("Ruckig计算失败，错误码: %1").arg(static_cast<int>(calcResult));
                    }
                break;
            }

            result.errorMessage = errorStr;
            qCritical() << "3-DOF Ruckig计算失败:" << errorStr;

            return result;
        }
        
        result.duration = trajectory.get_duration();
        
        // 采样轨迹
        const double samplingTime = params.getConfig().deltaT;
        const int numSamples = static_cast<int>(result.duration / samplingTime) + 1;
        
        result.points.reserve(numSamples);
        
        for (int i = 0; i <= numSamples; ++i) {
            double time = i * samplingTime;
            if (time > result.duration) time = result.duration;
            
            std::array<double, 3> new_position, new_velocity, new_acceleration;
            trajectory.at_time(time, new_position, new_velocity, new_acceleration);
            
            TrajectoryPoint point;
            point.time = time;
            point.x = new_position[0];
            point.y = new_position[1];
            point.z = new_position[2];
            point.vx = new_velocity[0];
            point.vy = new_velocity[1];
            point.vz = new_velocity[2];
            point.ax = new_acceleration[0];
            point.ay = new_acceleration[1];
            point.az = new_acceleration[2];
            
            result.points.append(point);
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = QString("3-DOF生成错误: %1").arg(e.what());
    }
#else
    qDebug() << "=== 进入简化算法分===";
    qDebug() << "HAVE_RUCKIG未定义，使用简化算法";
    // 测试用的简单3D直线插
    qWarning() << "Ruckig未启用，使用简单3D插值";
    
    const auto& start = params.getStartState();
    const auto& target = params.getTargetState();
    const auto& limits = params.getLimits();
    
    // 计算3D距离
    double dx = target.x - start.x;
    double dy = target.y - start.y;
    double dz = target.z - start.z;
    double distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    // 使用最大速度分量计算时间
    double maxSpeed = sqrt(limits.velX*limits.velX + limits.velY*limits.velY + limits.velZ*limits.velZ);
    result.duration = distance / maxSpeed;
    if (result.duration < 0.1) result.duration = 1.0;
    
    const double samplingTime = params.getConfig().deltaT;
    const int numSamples = static_cast<int>(result.duration / samplingTime) + 1;
    
    for (int i = 0; i <= numSamples; ++i) {
        double time = i * samplingTime;
        if (time > result.duration) time = result.duration;
        
        double t = time / result.duration;
        TrajectoryPoint point;
        point.time = time;
        point.x = start.x + dx * t;
        point.y = start.y + dy * t;
        point.z = start.z + dz * t;
        point.vx = dx / result.duration;
        point.vy = dy / result.duration;
        point.vz = dz / result.duration;
        
        result.points.append(point);
    }
    
    result.success = true;
#endif
    
    return result;
}

TrajectoryResult TrajectoryGenerator::generate6DOF(const FlightParams& params)
{
    TrajectoryResult result;
    result.dof = 6;

#ifdef HAVE_RUCKIG
    try {
        const auto& start = params.getStartState();
        const auto& target = params.getTargetState();
        const auto& limits = params.getLimits();
        const auto& attitude = params.getAttitudeParams();
        const double deltaT = params.getConfig().deltaT;

        if (deltaT <= 0.0) {
            result.errorMessage = QString("无效的时间步长：%1").arg(deltaT);
            return result;
        }

        if (!attitude.isValid()) {
            result.errorMessage = "6-DOF姿态约束参数无效";
            return result;
        }

        ruckig::Ruckig<6> otg{deltaT};
        ruckig::InputParameter<6> input;
        ruckig::Trajectory<6> trajectory;

        input.current_position = {
            start.x, start.y, start.z,
            attitude.roll0, attitude.pitch0, attitude.yaw0
        };
        input.current_velocity = {
            start.vx, start.vy, start.vz,
            attitude.rollRate0, attitude.pitchRate0, attitude.yawRate0
        };
        input.current_acceleration = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

        input.target_position = {
            target.x, target.y, target.z,
            attitude.rollf, attitude.pitchf, attitude.yawf
        };
        input.target_velocity = {
            target.vx, target.vy, target.vz,
            attitude.rollRatef, attitude.pitchRatef, attitude.yawRatef
        };
        input.target_acceleration = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

        input.max_velocity = {
            limits.velX, limits.velY, limits.velZ,
            attitude.maxRollVel, attitude.maxPitchVel, attitude.maxYawVel
        };
        input.max_acceleration = {
            limits.accX, limits.accY, limits.accZ,
            attitude.maxRollAcc, attitude.maxPitchAcc, attitude.maxYawAcc
        };
        input.max_jerk = {
            limits.jerkX, limits.jerkY, limits.jerkZ,
            attitude.maxRollJerk, attitude.maxPitchJerk, attitude.maxYawJerk
        };
        input.synchronization = parseSyncMode(params.getConfig().syncMode);

        auto calcResult = otg.calculate(input, trajectory);
        if (calcResult != ruckig::Result::Working) {
            switch (calcResult) {
            case ruckig::Result::ErrorInvalidInput:
                result.errorMessage = "6-DOF输入参数无效";
                break;
            case ruckig::Result::ErrorPositionalLimits:
                result.errorMessage = "6-DOF位置或姿态限制错误";
                break;
            case ruckig::Result::ErrorTrajectoryDuration:
                result.errorMessage = "当前约束下无法到达6-DOF目标";
                break;
            default:
                result.errorMessage = QString("6-DOF Ruckig计算失败，错误码：%1")
                                          .arg(static_cast<int>(calcResult));
                break;
            }
            return result;
        }

        result.duration = trajectory.get_duration();
        const int numSamples = static_cast<int>(result.duration / deltaT) + 1;
        result.points.reserve(numSamples + 1);

        for (int i = 0; i <= numSamples; ++i) {
            double time = i * deltaT;
            if (time > result.duration) {
                time = result.duration;
            }

            std::array<double, 6> newPosition, newVelocity, newAcceleration;
            trajectory.at_time(time, newPosition, newVelocity, newAcceleration);

            TrajectoryPoint point;
            point.time = time;
            point.x = newPosition[0];
            point.y = newPosition[1];
            point.z = newPosition[2];
            point.roll = newPosition[3];
            point.pitch = newPosition[4];
            point.yaw = newPosition[5];
            point.vx = newVelocity[0];
            point.vy = newVelocity[1];
            point.vz = newVelocity[2];
            point.rollRate = newVelocity[3];
            point.pitchRate = newVelocity[4];
            point.yawRate = newVelocity[5];
            point.ax = newAcceleration[0];
            point.ay = newAcceleration[1];
            point.az = newAcceleration[2];

            result.points.append(point);

            if (qFuzzyCompare(time, result.duration)) {
                break;
            }
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.errorMessage = QString("6-DOF生成错误：%1").arg(e.what());
    }
#else
    qWarning() << "Ruckig未启用，使用简化6-DOF线性插值";

    const auto& start = params.getStartState();
    const auto& target = params.getTargetState();
    const auto& limits = params.getLimits();
    const auto& attitude = params.getAttitudeParams();
    const double samplingTime = params.getConfig().deltaT;

    if (samplingTime <= 0.0) {
        result.errorMessage = QString("无效的时间步长：%1").arg(samplingTime);
        return result;
    }

    const double dx = target.x - start.x;
    const double dy = target.y - start.y;
    const double dz = target.z - start.z;
    const double distance = sqrt(dx * dx + dy * dy + dz * dz);
    const double maxSpeed = sqrt(limits.velX * limits.velX + limits.velY * limits.velY + limits.velZ * limits.velZ);

    result.duration = maxSpeed > 1e-6  distance / maxSpeed : 1.0;
    if (result.duration < 0.1) {
        result.duration = 1.0;
    }

    const int numSamples = static_cast<int>(result.duration / samplingTime) + 1;

    for (int i = 0; i <= numSamples; ++i) {
        double time = i * samplingTime;
        if (time > result.duration) {
            time = result.duration;
        }

        const double t = result.duration > 1e-6  time / result.duration : 1.0;
        TrajectoryPoint point;
        point.time = time;
        point.x = start.x + dx * t;
        point.y = start.y + dy * t;
        point.z = start.z + dz * t;
        point.vx = dx / result.duration;
        point.vy = dy / result.duration;
        point.vz = dz / result.duration;
        point.roll = attitude.roll0 + (attitude.rollf - attitude.roll0) * t;
        point.pitch = attitude.pitch0 + (attitude.pitchf - attitude.pitch0) * t;
        point.yaw = attitude.yaw0 + (attitude.yawf - attitude.yaw0) * t;
        point.rollRate = (attitude.rollf - attitude.roll0) / result.duration;
        point.pitchRate = (attitude.pitchf - attitude.pitch0) / result.duration;
        point.yawRate = (attitude.yawf - attitude.yaw0) / result.duration;
        result.points.append(point);
    }

    result.success = true;
#endif

    return result;
}
bool TrajectoryGenerator::validateRuckigParameters(const FlightParams& params)
{
    const auto& start = params.getStartState();
    const auto& target = params.getTargetState();
    const auto& limits = params.getLimits();
    const auto& config = params.getConfig();

    // 1. 检查DOF
    if (config.dof != 3) {
        qCritical() << "DOF验证失败：期望3，实际为" << config.dof;
        return false;
    }

    // 2. 检查时间步
    if (config.deltaT <= 0) {
        qCritical() << "时间步长必须大于0: " << config.deltaT;
        return false;
    }
    if (config.deltaT > 0.1) {  // 如果大于100毫秒，可能有问题
        qWarning() << "时间步长较大: " << config.deltaT
                   << "s (建议小于0.1s以获得平滑轨";
    }
    if (config.deltaT < 0.001) {  // 小于1毫秒，可能过
        qWarning() << "时间步长过小: " << config.deltaT
                   << "s (可能产生过多采样";
    }

    // 3. 检查约束是否为
    if (limits.velX <= 0 || limits.velY <= 0 || limits.velZ <= 0) {
        qCritical() << "速度约束必须为正";
        return false;
    }

    if (limits.accX <= 0 || limits.accY <= 0 || limits.accZ <= 0) {
        qCritical() << "加度约束必须为正";
        return false;
    }

    if (limits.jerkX <= 0 || limits.jerkY <= 0 || limits.jerkZ <= 0) {
        qCritical() << "加加速度约束必须为正";
        return false;
    }

    // 4. 检查位置是否有显著变化
    double dx = target.x - start.x;
    double dy = target.y - start.y;
    double dz = target.z - start.z;
    double distance = sqrt(dx*dx + dy*dy + dz*dz);

    if (distance < 0.1) {  // 最小距.1
        qCritical() << "起始和目标位置过于接近，距离:" << distance << "m";
        qDebug() << "起始: (" << start.x << "," << start.y << "," << start.z << ")";
        qDebug() << "目标: (" << target.x << "," << target.y << "," << target.z << ")";
        return false;
    }

    // 5. 检查度是否在约束范围内
    double startSpeed = sqrt(start.vx*start.vx + start.vy*start.vy + start.vz*start.vz);
    double maxSpeed = sqrt(limits.velX*limits.velX + limits.velY*limits.velY + limits.velZ*limits.velZ);

    if (startSpeed > maxSpeed + 0.001) {  // 加上小容
        qCritical() << "起始速度超出约束，起始度:" << startSpeed << "m/s，最大允" << maxSpeed << "m/s";
        return false;
    }

    double targetSpeed = sqrt(target.vx*target.vx + target.vy*target.vy + target.vz*target.vz);
    if (targetSpeed > maxSpeed + 0.001) {
        qCritical() << "目标速度超出约束，目标度:" << targetSpeed << "m/s，最大允" << maxSpeed << "m/s";
        return false;
    }

    // 6. 检查加速度是否合理
    double maxAcc = sqrt(limits.accX*limits.accX + limits.accY*limits.accY + limits.accZ*limits.accZ);
    if (maxAcc < 0.1) {
        qCritical() << "加度约束过小:" << maxAcc;
        return false;
    }

    qDebug() << "参数验证通过:";
    qDebug() << "  距离:" << distance << "m";
    qDebug() << "  起始速度:" << startSpeed << "m/s";
    qDebug() << "  目标速度:" << targetSpeed << "m/s";
    qDebug() << "  最大速度:" << maxSpeed << "m/s";
    qDebug() << "  最大加速度:" << maxAcc << "m/s²";

    return true;
}


void TrajectoryGenerator::sampleTrajectory(const FlightParams& params,
                                          TrajectoryResult& result,
                                          const QVector<double>& times)
{
    // 通用采样函数，可以根据需要扩
    Q_UNUSED(params);
    Q_UNUSED(result);
    Q_UNUSED(times);
    // 预留接口
}

} // namespace core
