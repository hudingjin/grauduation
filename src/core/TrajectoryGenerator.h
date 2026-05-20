#ifndef TRAJECTORYGENERATOR_H
#define TRAJECTORYGENERATOR_H

#include "FlightParams.h"
#include <QObject>
#include <QVector>
#include <mutex>

namespace core {

    // 轨迹点结构体
    struct TrajectoryPoint {
        double time = 0.0;
        double x = 0.0, y = 0.0, z = 0.0;
        double vx = 0.0, vy = 0.0, vz = 0.0;
        double ax = 0.0, ay = 0.0, az = 0.0;
        double jx = 0.0, jy = 0.0, jz = 0.0;

        // 用于6-DOF
        double roll = 0.0, pitch = 0.0, yaw = 0.0;
        double rollRate = 0.0, pitchRate = 0.0, yawRate = 0.0;
    };

    // 轨迹结果结构体
    struct TrajectoryResult {
        bool success = false;
        QString errorMessage;
        double duration = 0.0;  // 轨迹总时长
        QVector<TrajectoryPoint> points;  // 轨迹点序列
        int dof = 3;  // 实际使用的自由度

        // 清除数据
        void clear() {
            success = false;
            errorMessage.clear();
            duration = 0.0;
            points.clear();
        }
    };

    class TrajectoryGenerator : public QObject
    {
        Q_OBJECT

    public:
        explicit TrajectoryGenerator(QObject* parent = nullptr);

        // 生成轨迹
        TrajectoryResult generateTrajectory(const FlightParams& params);

        // 获取上次生成的结果
        TrajectoryResult getLastResult() const { return m_lastResult; }

        // 检查Ruckig库是否可用
        bool isRuckigAvailable() const;
        static bool validateRuckigParameters(const FlightParams& params);

        signals:
            // 轨迹生成完成信号
            void trajectoryGenerated(const TrajectoryResult& result);
        void generationError(const QString& error);

    private:
        // 1-DOF轨迹生成
        static TrajectoryResult generate1DOF(const FlightParams& params);

        // 3-DOF轨迹生成
        static TrajectoryResult generate3DOF(const FlightParams& params);

        // 6-DOF轨迹生成
        static TrajectoryResult generate6DOF(const FlightParams& params);

        // 通用轨迹采样函数
        static void sampleTrajectory(const FlightParams& params,
                             TrajectoryResult& result,
                             const QVector<double>& times);

        // 线程安全锁
        mutable std::mutex m_mutex;

        // 上一次生成结果
        TrajectoryResult m_lastResult;
    };

} // namespace core

#endif // TRAJECTORYGENERATOR_H