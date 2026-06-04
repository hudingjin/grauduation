#ifndef FLIGHTDISPLAYWIDGET_H
#define FLIGHTDISPLAYWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QPainterPath>

namespace core {
struct TrajectoryPoint;
struct TrajectoryResult;
}

class FlightDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlightDisplayWidget(QWidget* parent = nullptr);

    // 设置完整轨迹数据（绘制参考路径）
    void setTrajectoryData(const core::TrajectoryResult& result);

    // 实时更新飞机位置（仿真播放时调用）
    void updateAircraftState(const core::TrajectoryPoint& state);

    // 按时间定位飞机（独立播放用）
    void setTime(double time);

    // 获取轨迹总时长
    double trajectoryDuration() const;

    // 清空
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // 世界坐标 → 屏幕坐标转换
    QPointF worldToScreen(double x, double y) const;
    void updateViewTransform();

    // 飞机图标绘制
    static QPainterPath createAircraftShape(double size);

    // 数据
    struct {
        double xMin = -10, xMax = 510;
        double yMin = -10, yMax = 310;
    } m_worldBounds;

    QVector<core::TrajectoryPoint> m_trajectoryPoints; // 原始轨迹数据（用于时间查找）
    QVector<QPointF> m_trajectoryPath;  // 完整参考轨迹（屏幕坐标）
    QVector<QPointF> m_flownPath;        // 已飞过的轨迹
    double m_totalDuration = 0;          // 轨迹总时长

    // 当前飞机状态
    double m_currentX = 0, m_currentY = 0;
    double m_currentHeading = 0;  // 航向角（度）
    bool m_hasData = false;

    // 视图变换参数
    double m_scaleX = 1.0, m_scaleY = 1.0;
    double m_offsetX = 0, m_offsetY = 0;
    int m_margin = 40;
};

#endif // FLIGHTDISPLAYWIDGET_H
