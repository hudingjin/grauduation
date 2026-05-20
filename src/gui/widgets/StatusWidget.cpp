//
// Created by 胡定进 on 2026/5/8.
//

#include "StatusWidget.h"
#include <QDebug>
#include <cmath>

StatusWidget::StatusWidget(QWidget* parent)
    : QWidget(parent)
{
    qDebug() << "StatusWidget构造函数";
}

StatusWidget::~StatusWidget()
{
    qDebug() << "StatusWidget析构函数";
}

void StatusWidget::setupUI(QLabel* labelRollValue,
                           QLabel* labelPitchValue,
                           QLabel* labelYawValue,
                           QLabel* valueTimeCurrent,
                           QLabel* valueTimeTotal,
                           QLabel* valuePosition,
                           QLabel* valueSpeed,
                           QLabel* valueStatus,
                           QLabel* valueRuckig,
                           QProgressBar* progressSimulation)
{
    qDebug() << "StatusWidget::setupUI 设置界面组件";

    // 保存组件指针
    m_labelRollValue = labelRollValue;
    m_labelPitchValue = labelPitchValue;
    m_labelYawValue = labelYawValue;
    m_valueTimeCurrent = valueTimeCurrent;
    m_valueTimeTotal = valueTimeTotal;
    m_valuePosition = valuePosition;
    m_valueSpeed = valueSpeed;
    m_valueStatus = valueStatus;
    m_valueRuckig = valueRuckig;
    m_progressSimulation = progressSimulation;

    // 检查组件是否找到
    if (!m_labelRollValue || !m_labelPitchValue || !m_labelYawValue) {
        qWarning() << "姿态标签指针无效";
    }
    if (!m_valueTimeCurrent || !m_valueTimeTotal) {
        qWarning() << "时间标签指针无效";
    }
    if (!m_valuePosition || !m_valueSpeed) {
        qWarning() << "位置/速度标签指针无效";
    }
    if (!m_valueStatus || !m_valueRuckig) {
        qWarning() << "状态标签指针无效";
    }

    // 初始化为默认值
    clearStatus();
}

void StatusWidget::updateAttitude(double roll, double pitch, double yaw)
{
    m_currentRoll = roll;
    m_currentPitch = pitch;
    m_currentYaw = yaw;

    if (m_labelRollValue) {
        m_labelRollValue->setText(formatAngle(roll));
    }
    if (m_labelPitchValue) {
        m_labelPitchValue->setText(formatAngle(pitch));
    }
    if (m_labelYawValue) {
        m_labelYawValue->setText(formatAngle(yaw));
    }
}

void StatusWidget::updateTime(double currentTime, double totalTime)
{
    m_currentTime = currentTime;
    m_totalTime = totalTime;

    if (m_valueTimeCurrent) {
        m_valueTimeCurrent->setText(formatTime(currentTime));
    }
    if (m_valueTimeTotal) {
        m_valueTimeTotal->setText(formatTime(totalTime));
    }

    // 更新进度条
    if (m_progressSimulation && totalTime > 0) {
        int progress = static_cast<int>((currentTime / totalTime) * 100.0);
        progress = qBound(0, progress, 100);
        m_progressSimulation->setValue(progress);

        // 更新进度条文本
        if (progress < 100) {
            m_progressSimulation->setFormat(QString("%1%").arg(progress));
        } else {
            m_progressSimulation->setFormat("完成");
        }
    }
}

void StatusWidget::updatePosition(float x, float y, float z)
{
    m_currentPosition = {x, y, z};

    if (m_valuePosition) {
        QString posText = QString("[%1, %2, %3]")
            .arg(formatPosition(x))
            .arg(formatPosition(y))
            .arg(formatPosition(z));
        m_valuePosition->setText(posText);
    }
}

void StatusWidget::updateSpeed(float vx, float vy, float vz)
{
    m_currentVelocity = {vx, vy, vz};

    if (m_valueSpeed) {
        // 计算合速度
        double speed = std::sqrt(vx * vx + vy * vy + vz * vz);
        m_valueSpeed->setText(formatSpeed(speed));
    }
}

void StatusWidget::updateStatus(const QString& status, bool isError)
{
    if (m_valueStatus) {
        m_valueStatus->setText(status);
    }
}

void StatusWidget::updateRuckigStatus(const QString& ruckigStatus, bool isError)
{
    if (m_valueRuckig) {
        m_valueRuckig->setText(ruckigStatus);
    }
}

void StatusWidget::updateSimulationProgress(int progress)
{
    if (m_progressSimulation) {
        progress = qBound(0, progress, 100);
        m_progressSimulation->setValue(progress);

        if (progress < 100) {
            m_progressSimulation->setFormat(QString("%1%").arg(progress));
        } else {
            m_progressSimulation->setFormat("完成");
        }
    }
}

void StatusWidget::clearStatus()
{
    // 清空所有显示
    if (m_labelRollValue) m_labelRollValue->setText("0.00°");
    if (m_labelPitchValue) m_labelPitchValue->setText("0.00°");
    if (m_labelYawValue) m_labelYawValue->setText("0.00°");

    if (m_valueTimeCurrent) m_valueTimeCurrent->setText("0.000s");
    if (m_valueTimeTotal) m_valueTimeTotal->setText("0.000s");

    // 位置两位小数显示
    if (m_valuePosition) m_valuePosition->setText("[0.00, 0.00, 0.00]");

    // 速度也保持两位小数
    if (m_valueSpeed) m_valueSpeed->setText("0.00");

    // 修改：恢复为"就绪"
    if (m_valueStatus) m_valueStatus->setText("就绪");
    if (m_valueRuckig) m_valueRuckig->setText("就绪");

    if (m_progressSimulation) {
        m_progressSimulation->setValue(0);
        m_progressSimulation->setFormat("0%");
    }

    // 重置缓存
    m_currentRoll = m_currentPitch = m_currentYaw = 0.0;
    m_currentTime = m_totalTime = 0.0;
    m_currentPosition = {0.0, 0.0, 0.0};
    m_currentVelocity = {0.0, 0.0, 0.0};
}

QString StatusWidget::formatAngle(double angle)
{
    return QString::number(angle, 'f', 2) + "°";
}

QString StatusWidget::formatTime(double time)
{
    if (time < 1e-6) {
        return "0.000s";
    }
    return QString::number(time, 'f', 3) + "s";
}

QString StatusWidget::formatPosition(double value)
{
    if (qAbs(value) < 1e-6) {
        return "0.00";
    }
    return QString::number(value, 'f', 2);
}

QString StatusWidget::formatSpeed(double value)
{
    if (value < 1e-6) {
        return "0.000";
    }
    return QString::number(value, 'f', 3);
}