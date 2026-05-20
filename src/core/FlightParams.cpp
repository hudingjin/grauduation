//
// Created by 胡定进 on 2026/4/13.
//
// FlightParams.cpp
#include "FlightParams.h"

namespace core {

    void FlightParams::printDebug() const {
        qDebug() << "======== [Core] 参数更新 ========";
        qDebug() << "自由度:" << m_config.dof
                 << "| 6-DOF模式:" << (m_is6DOF ? "是" : "否")
                 << "| 同步模式:" << m_config.syncMode
                 << "| 周期:" << m_config.deltaT << "ms";

        qDebug() << "最大速度:" << m_limits.velX << m_limits.velY << m_limits.velZ;
        qDebug() << "起始位置:" << m_start.x << m_start.y << m_start.z;
        qDebug() << "目标位置:" << m_target.x << m_target.y << m_target.z;

        if (m_is6DOF) {
            qDebug() << "姿态参数 - 起始: ("
                     << m_attitudeParams.roll0 << ", "
                     << m_attitudeParams.pitch0 << ", "
                     << m_attitudeParams.yaw0 << ")°";
            qDebug() << "姿态参数 - 目标: ("
                     << m_attitudeParams.rollf << ", "
                     << m_attitudeParams.pitchf << ", "
                     << m_attitudeParams.yawf << ")°";
        }
        qDebug() << "=================================";
    }

} // namespace core