#include "FlightDisplayWidget.h"
#include "../../core/TrajectoryGenerator.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <QDebug>

FlightDisplayWidget::FlightDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(280, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background-color: #061424; border: 1px solid rgba(0, 212, 255, 0.25);");
}

void FlightDisplayWidget::setTrajectoryData(const core::TrajectoryResult& result)
{
    if (result.points.isEmpty()) return;

    const int totalPts = result.points.size();

    // 计算世界坐标范围（遍历全部点，这是必须的 O(n) 操作）
    m_worldBounds.xMin = std::numeric_limits<double>::max();
    m_worldBounds.xMax = std::numeric_limits<double>::lowest();
    m_worldBounds.yMin = std::numeric_limits<double>::max();
    m_worldBounds.yMax = std::numeric_limits<double>::lowest();

    for (const auto& p : result.points) {
        m_worldBounds.xMin = qMin(m_worldBounds.xMin, p.x);
        m_worldBounds.xMax = qMax(m_worldBounds.xMax, p.x);
        m_worldBounds.yMin = qMin(m_worldBounds.yMin, p.y);
        m_worldBounds.yMax = qMax(m_worldBounds.yMax, p.y);
    }

    // 添加10%边距
    double dx = m_worldBounds.xMax - m_worldBounds.xMin;
    double dy = m_worldBounds.yMax - m_worldBounds.yMin;
    if (dx < 1.0) dx = 1.0;
    if (dy < 1.0) dy = 1.0;
    m_worldBounds.xMin -= dx * 0.1;
    m_worldBounds.xMax += dx * 0.1;
    m_worldBounds.yMin -= dy * 0.1;
    m_worldBounds.yMax += dy * 0.1;

    updateViewTransform();

    // 预计算参考轨迹的屏幕坐标 —— 降采样到 ~2000 个点
    // 再多在屏幕上也只是重叠在相同像素上，浪费 CPU
    constexpr int MAX_PATH_POINTS = 2000;
    const int stride = qMax(1, totalPts / MAX_PATH_POINTS);
    m_trajectoryPath.clear();
    m_trajectoryPath.reserve(totalPts / stride + 2);

    for (int i = 0; i < totalPts; i += stride) {
        m_trajectoryPath.append(worldToScreen(result.points[i].x, result.points[i].y));
    }
    // 确保终点包含
    if ((totalPts - 1) % stride != 0) {
        m_trajectoryPath.append(worldToScreen(result.points.last().x, result.points.last().y));
    }

    // 保存原始轨迹数据用于仿真时的精确时间查找（二分查找 O(log n)，不需要降采样）
    m_trajectoryPoints = result.points;
    m_totalDuration = result.duration;
    m_dof = result.dof;             // 记录 DOF，3-DOF 不显示航向角

    m_flownPath.clear();
    m_currentHeading = 0;
    m_hasData = true;
    update();
}

void FlightDisplayWidget::setTime(double time)
{
    if (!m_hasData || m_trajectoryPoints.isEmpty()) return;

    // 边界处理
    if (time <= 0) {
        updateAircraftState(m_trajectoryPoints.first());
        return;
    }
    if (time >= m_totalDuration) {
        updateAircraftState(m_trajectoryPoints.last());
        return;
    }

    // 二分查找最近的时间点
    int lo = 0, hi = m_trajectoryPoints.size() - 1;
    while (lo < hi - 1) {
        int mid = (lo + hi) / 2;
        if (m_trajectoryPoints[mid].time <= time)
            lo = mid;
        else
            hi = mid;
    }

    const auto& p1 = m_trajectoryPoints[lo];
    const auto& p2 = m_trajectoryPoints[hi];
    double alpha = (time - p1.time) / (p2.time - p1.time);

    // 线性插值
    core::TrajectoryPoint pt;
    pt.time = time;
    pt.x = p1.x + (p2.x - p1.x) * alpha;
    pt.y = p1.y + (p2.y - p1.y) * alpha;
    pt.z = p1.z + (p2.z - p1.z) * alpha;
    pt.vx = p1.vx + (p2.vx - p1.vx) * alpha;
    pt.vy = p1.vy + (p2.vy - p1.vy) * alpha;
    pt.vz = p1.vz + (p2.vz - p1.vz) * alpha;
    pt.yaw = p1.yaw + (p2.yaw - p1.yaw) * alpha;

    updateAircraftState(pt);
}

double FlightDisplayWidget::trajectoryDuration() const
{
    return m_totalDuration;
}

void FlightDisplayWidget::updateAircraftState(const core::TrajectoryPoint& state)
{
    if (!m_hasData) return;

    m_currentX = state.x;
    m_currentY = state.y;

    // 航向角：仅 6-DOF 模式使用 Ruckig 计算的真实 yaw 角旋转飞机
    // 3-DOF/1-DOF 不旋转，飞机保持 0°（机头朝右）
    if (m_dof == 6) {
        m_currentHeading = state.yaw;
    } else {
        m_currentHeading = 0.0;
    }

    // 追加已飞路径
    QPointF screenPt = worldToScreen(m_currentX, m_currentY);
    if (m_flownPath.isEmpty() || m_flownPath.last() != screenPt) {
        m_flownPath.append(screenPt);
    }

    update();
}

void FlightDisplayWidget::clear()
{
    m_trajectoryPath.clear();
    m_flownPath.clear();
    m_hasData = false;
    m_currentHeading = 0;
    update();
}

void FlightDisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // 背景
    painter.fillRect(rect(), QColor("#061424"));

    if (!m_hasData) {
        painter.setPen(QColor("#3a5a7a"));
        painter.setFont(QFont("Share Tech Mono", 11));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("飞行轨迹俯视图\n生成轨迹后显示"));
        return;
    }

    // ---- 网格 ----
    painter.setPen(QPen(QColor(0, 100, 160, 30), 0.5));
    int gridStep = 40;
    for (int x = m_margin; x < w - m_margin; x += gridStep) {
        painter.drawLine(x, m_margin, x, h - m_margin);
    }
    for (int y = m_margin; y < h - m_margin; y += gridStep) {
        painter.drawLine(m_margin, y, w - m_margin, y);
    }

    // ---- 坐标轴 ----
    painter.setPen(QPen(QColor(0, 100, 160, 80), 1));
    int axisY = h - m_margin;  // X轴在底部
    int axisX = m_margin;       // Y轴在左边
    painter.drawLine(m_margin, axisY, w - m_margin, axisY);  // X轴
    painter.drawLine(axisX, m_margin, axisX, axisY);          // Y轴

    // 坐标轴标签
    painter.setPen(QColor("#5a8aaa"));
    QFont labelFont("Share Tech Mono", 7);
    painter.setFont(labelFont);

    // X轴刻度
    for (int i = 0; i <= 4; ++i) {
        double val = m_worldBounds.xMin + (m_worldBounds.xMax - m_worldBounds.xMin) * i / 4.0;
        double sx = m_margin + (w - 2 * m_margin) * i / 4.0;
        painter.drawLine(QPointF(sx, axisY), QPointF(sx, axisY + 4));
        painter.drawText(QRectF(sx - 20, axisY + 4, 40, 12), Qt::AlignHCenter | Qt::AlignTop,
                         QString::number(val, 'f', 0) + "m");
    }

    // Y轴刻度
    for (int i = 0; i <= 4; ++i) {
        double val = m_worldBounds.yMin + (m_worldBounds.yMax - m_worldBounds.yMin) * i / 4.0;
        double sy = axisY - (h - 2 * m_margin) * i / 4.0;
        painter.drawLine(QPointF(axisX - 4, sy), QPointF(axisX, sy));
        painter.drawText(QRectF(2, sy - 6, axisX - 8, 12), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(val, 'f', 0));
    }

    // ---- 已飞轨迹（亮线） ----
    if (m_flownPath.size() >= 2) {
        QPen flownPen(QColor(0, 255, 159, 180), 1.5);
        painter.setPen(flownPen);
        for (int i = 0; i < m_flownPath.size() - 1; ++i) {
            painter.drawLine(m_flownPath[i], m_flownPath[i + 1]);
        }
    }

    // ---- 飞机图标 ----
    QPointF aircraftScreen = worldToScreen(m_currentX, m_currentY);
    double iconSize = qMin(w, h) * 0.04;
    iconSize = qBound(8.0, iconSize, 20.0);

    painter.save();
    painter.translate(aircraftScreen);
    painter.rotate(m_currentHeading);

    // 绘制飞机形状
    QPainterPath aircraft = createAircraftShape(iconSize);
    painter.setBrush(QColor(0, 255, 159));       // 亮绿色填充
    painter.setPen(QPen(QColor("#00d4ff"), 1.2)); // 青色边框
    painter.drawPath(aircraft);

    // 速度方向指示线
    painter.setPen(QPen(QColor(0, 255, 159, 180), 0.8));
    painter.drawLine(QPointF(iconSize * 0.6, 0), QPointF(iconSize * 1.8, 0));

    painter.restore();

    // ---- 当前坐标标注 ----
    painter.setPen(QColor("#00d4ff"));
    QFont coordFont("Share Tech Mono", 8);
    painter.setFont(coordFont);
    // 3-DOF 不显示航向角，只有 6-DOF 才显示 H
    QString coordText;
    if (m_dof == 6) {
        coordText = QString("X:%1  Y:%2  H:%3°")
            .arg(m_currentX, 0, 'f', 1)
            .arg(m_currentY, 0, 'f', 1)
            .arg(m_currentHeading, 0, 'f', 1);
    } else {
        coordText = QString("X:%1  Y:%2")
            .arg(m_currentX, 0, 'f', 1)
            .arg(m_currentY, 0, 'f', 1);
    }
    painter.drawText(QPointF(m_margin, m_margin - 6), coordText);

    // ---- 图例 ----
    painter.setPen(QColor("#5a8aaa"));
    QFont legendFont("Share Tech Mono", 7);
    painter.setFont(legendFont);

    int legendX = w - m_margin - 130;
    painter.setPen(QPen(QColor(0, 255, 159, 180), 1.5));
    painter.drawLine(legendX, m_margin + 8, legendX + 20, m_margin + 8);
    painter.setPen(QColor("#5a8aaa"));
    painter.drawText(legendX + 25, m_margin + 12, QString::fromUtf8("飞行轨迹"));
}

void FlightDisplayWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateViewTransform();
    // 重新计算屏幕坐标
    if (m_hasData && !m_trajectoryPath.isEmpty()) {
        // 保持简单：resize时重绘，坐标转换按新的widget尺寸计算
    }
}

QPointF FlightDisplayWidget::worldToScreen(double x, double y) const
{
    int w = width() - 2 * m_margin;
    int h = height() - 2 * m_margin;

    double sx = m_margin + (x - m_worldBounds.xMin) / (m_worldBounds.xMax - m_worldBounds.xMin) * w;
    // Y轴翻转（屏幕Y向下，世界Y向上）
    double sy = m_margin + h - (y - m_worldBounds.yMin) / (m_worldBounds.yMax - m_worldBounds.yMin) * h;

    return QPointF(sx, sy);
}

void FlightDisplayWidget::updateViewTransform()
{
    // 缩放因子在worldToScreen中动态计算
}

QPainterPath FlightDisplayWidget::createAircraftShape(double size)
{
    QPainterPath path;
    // 飞机三角形：机头朝右(0°方向)
    double s = size;
    path.moveTo(s, 0);                    // 机头
    path.lineTo(-s * 0.5, -s * 0.35);    // 右翼尖
    path.lineTo(-s * 0.2, 0);            // 机尾凹陷
    path.lineTo(-s * 0.5, s * 0.35);     // 左翼尖
    path.closeSubpath();
    return path;
}
