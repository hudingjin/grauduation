#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QDebug>

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatusWidget(QWidget* parent = nullptr);
    ~StatusWidget();

    // 设置界面组件
    void setupUI(QLabel* labelRollValue,
                 QLabel* labelPitchValue,
                 QLabel* labelYawValue,
                 QLabel* valueTimeCurrent,
                 QLabel* valueTimeTotal,
                 QLabel* valuePosition,
                 QLabel* valueSpeed,
                 QLabel* valueStatus,
                 QLabel* valueRuckig,
                 QProgressBar* progressSimulation = nullptr);

    // 更新姿态信息
    void updateAttitude(double roll, double pitch, double yaw);
    
    // 更新时间信息
    void updateTime(double currentTime, double totalTime);
    
    // 更新位置信息
    void updatePosition(float x, float y, float z);
    
    // 更新速度信息
    void updateSpeed(float vx, float vy, float vz);
    
    // 更新状态信息
    void updateStatus(const QString& status, bool isError = false);
    
    // 更新Ruckig输出状态
    void updateRuckigStatus(const QString& ruckigStatus, bool isError = false);
    
    // 更新仿真进度
    void updateSimulationProgress(int progress);
    
    // 清空所有状态
    void clearStatus();

private:
    // 格式化数值显示
    static QString formatAngle(double angle);
    static QString formatTime(double time);
    static QString formatPosition(double value);
    static QString formatSpeed(double value);

private:
    // 界面组件指针
    QLabel* m_labelRollValue = nullptr;
    QLabel* m_labelPitchValue = nullptr;
    QLabel* m_labelYawValue = nullptr;
    QLabel* m_valueTimeCurrent = nullptr;
    QLabel* m_valueTimeTotal = nullptr;
    QLabel* m_valuePosition = nullptr;
    QLabel* m_valueSpeed = nullptr;
    QLabel* m_valueStatus = nullptr;
    QLabel* m_valueRuckig = nullptr;
    QProgressBar* m_progressSimulation = nullptr;
    
    // 当前状态缓存
    double m_currentRoll = 0.0;
    double m_currentPitch = 0.0;
    double m_currentYaw = 0.0;
    double m_currentTime = 0.0;
    double m_totalTime = 0.0;
    struct { float x, y, z; } m_currentPosition = {0.0, 0.0, 0.0};
    struct { float x, y, z; } m_currentVelocity = {0.0, 0.0, 0.0};
};

#endif // STATUSWIDGET_H