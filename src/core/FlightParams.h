#ifndef FLIGHTPARAMS_H
#define FLIGHTPARAMS_H

#include <QString>
#include <QDebug>

//命名空间
// 如果没有命名空间，这些标识符就位于全局命名空间
// 如果其他库也有 KinematicLimits或 FlightParams类，会发生冲突
// 通过 core::KinematicLimits和 core::FlightParams区分
namespace core {

    // Ruckig 顶层配置结构体
    struct RuckigConfig {
        int dof = 3;           // 自由度: 1, 3, 或 6
        QString syncMode = "TIME"; // 同步模式: TIME, PHASE, NONE
        double deltaT = 10.0;   // 控制周期
    };

    // 运动学约束结构体
    struct KinematicLimits {
        double velX = 80.0; double velY = 80.0; double velZ = 30.0;
        double accX = 15.0; double accY = 15.0; double accZ = 8.0;
        double jerkX = 50.0; double jerkY = 50.0; double jerkZ = 20.0;
        double velRoll = 30.0;double velPitch = 30.0;  double velYaw = 45.0;
        double accRoll = 60.0;double accPitch = 60.0;double accYaw = 90.0;
        double jerkRoll = 180.0;double jerkPitch = 180.0; double jerkYaw = 270.0;
    };

    // 状态点结构体 (位置+速度)
    struct StatePoint {
        double x = 0.0; double y = 0.0; double z = 1000.0;
        double vx = 0.0; double vy = 0.0; double vz = 0.0;
        double roll = 0.0;double pitch = 0.0;  double yaw = 0.0;
    };
    // 姿态参数结构
    // 姿态参数结构
    struct AttitudeParams {
        // 起始姿态
        double roll0 = 0.0;        // 起始Roll角 (°)
        double pitch0 = 0.0;       // 起始Pitch角 (°)
        double yaw0 = 0.0;         // 起始Yaw角 (°)
        double rollRate0 = 0.0;    // 起始Roll角速度 (°/s)
        double pitchRate0 = 0.0;   // 起始Pitch角速度 (°/s)
        double yawRate0 = 0.0;     // 起始Yaw角速度 (°/s)

        // 目标姿态
        double rollf = 0.0;        // 目标Roll角 (°)
        double pitchf = 0.0;       // 目标Pitch角 (°)
        double yawf = 0.0;         // 目标Yaw角 (°)
        double rollRatef = 0.0;    // 目标Roll角速度 (°/s)
        double pitchRatef = 0.0;   // 目标Pitch角速度 (°/s)
        double yawRatef = 0.0;     // 目标Yaw角速度 (°/s)

        // 角运动学限制
        double maxRollVel = 30.0;   // 最大Roll角速度 (°/s)
        double maxPitchVel = 30.0;  // 最大Pitch角速度 (°/s)
        double maxYawVel = 45.0;    // 最大Yaw角速度 (°/s)
        double maxRollAcc = 60.0;   // 最大Roll角加速度 (°/s²)
        double maxPitchAcc = 60.0;  // 最大Pitch角加速度 (°/s²)
        double maxYawAcc = 90.0;    // 最大Yaw角加速度 (°/s²)
        double maxRollJerk = 120.0; // 最大Roll角加加速度 (°/s³)
        double maxPitchJerk = 120.0; // 最大Pitch角加加速度 (°/s³)
        double maxYawJerk = 180.0;  // 最大Yaw角加加速度 (°/s³)

        // 验证函数
        bool isValid() const {
            return maxRollVel > 0 && maxPitchVel > 0 && maxYawVel > 0 &&
                   maxRollAcc > 0 && maxPitchAcc > 0 && maxYawAcc > 0 &&
                   maxRollJerk > 0 && maxPitchJerk > 0 && maxYawJerk > 0;
        }

        // 重置函数
        void reset() {
            roll0 = pitch0 = yaw0 = 0.0;
            rollRate0 = pitchRate0 = yawRate0 = 0.0;
            rollf = pitchf = yawf = 0.0;
            rollRatef = pitchRatef = yawRatef = 0.0;
            maxRollVel = 30.0;
            maxPitchVel = 30.0;
            maxYawVel = 45.0;
            maxRollAcc = 60.0;
            maxPitchAcc = 60.0;
            maxYawAcc = 90.0;
            maxRollJerk = 120.0;
            maxPitchJerk = 120.0;
            maxYawJerk = 180.0;
        }
    };
    class FlightParams {
    public:
        FlightParams() = default;

        // 获取参数
        [[nodiscard]] const RuckigConfig& getConfig() const { return m_config; }
        [[nodiscard]] const KinematicLimits& getLimits() const { return m_limits; }
        [[nodiscard]] const StatePoint& getStartState() const { return m_start; }
        [[nodiscard]] const StatePoint& getTargetState() const { return m_target; }
        [[nodiscard]] const AttitudeParams& getAttitudeParams() const { return m_attitudeParams; }  // 新增
        [[nodiscard]] bool is6DOF() const { return m_is6DOF; }  // 新增

        // 设置参数
        void setConfig(const RuckigConfig& config) { m_config = config; }
        void setLimits(const KinematicLimits& limits) { m_limits = limits; }
        void setStartState(const StatePoint& state) { m_start = state; }
        void setTargetState(const StatePoint& state) { m_target = state; }
        void setAttitudeParams(const AttitudeParams& params) { m_attitudeParams = params; }  // 新增
        void setIs6DOF(bool is6dof) { m_is6DOF = is6dof; }  // 新增

        // 打印参数 (用于调试)
        void printDebug() const;

    private:
        RuckigConfig m_config;
        KinematicLimits m_limits;
        StatePoint m_start;
        StatePoint m_target;
        AttitudeParams m_attitudeParams;  // 姿态参数
        bool m_is6DOF = false;  // 标记是否为6-DOF模式
    };

} // namespace core

#endif