#include "TrajectoryTableModel.h"
#include <cmath>
#include <QColor>

TrajectoryTableModel::TrajectoryTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // 初始化列定义
    m_columns.resize(COL_COUNT);

    m_columns[COL_TIME]           = {QStringLiteral("时间"),       QStringLiteral("s"),     true};
    m_columns[COL_POS_X]          = {QStringLiteral("位置 X"),     QStringLiteral("m"),     true};
    m_columns[COL_POS_Y]          = {QStringLiteral("位置 Y"),     QStringLiteral("m"),     true};
    m_columns[COL_POS_Z]          = {QStringLiteral("位置 Z"),     QStringLiteral("m"),     true};
    m_columns[COL_VEL_X]          = {QStringLiteral("速度 Vx"),    QStringLiteral("m/s"),   true};
    m_columns[COL_VEL_Y]          = {QStringLiteral("速度 Vy"),    QStringLiteral("m/s"),   true};
    m_columns[COL_VEL_Z]          = {QStringLiteral("速度 Vz"),    QStringLiteral("m/s"),   true};
    m_columns[COL_ACC_X]          = {QStringLiteral("加速度 Ax"),  QStringLiteral("m/s²"),  false};
    m_columns[COL_ACC_Y]          = {QStringLiteral("加速度 Ay"),  QStringLiteral("m/s²"),  false};
    m_columns[COL_ACC_Z]          = {QStringLiteral("加速度 Az"),  QStringLiteral("m/s²"),  false};
    m_columns[COL_JERK_X]         = {QStringLiteral("加加速度 Jx"),QStringLiteral("m/s³"),  false};
    m_columns[COL_JERK_Y]         = {QStringLiteral("加加速度 Jy"),QStringLiteral("m/s³"),  false};
    m_columns[COL_JERK_Z]         = {QStringLiteral("加加速度 Jz"),QStringLiteral("m/s³"),  false};
    m_columns[COL_TOTAL_DISTANCE] = {QStringLiteral("总距离"),     QStringLiteral("m"),     false};
    m_columns[COL_TOTAL_SPEED]    = {QStringLiteral("总速度"),     QStringLiteral("m/s"),   false};
    m_columns[COL_TOTAL_ACC]      = {QStringLiteral("总加速度"),   QStringLiteral("m/s²"),  false};

    rebuildColumnMapping();
}
//根据列的可见性，建立「可见列索引 ↔ 实际列ID」的双向映射。
void TrajectoryTableModel::rebuildColumnMapping()
{
    m_sectionToColId.clear();// 清空映射向量
    m_colIdToSection.fill(-1, COL_COUNT); // 把所有列ID对应的可见索引设为-1（隐藏）

    for (int colId = 0; colId < COL_COUNT; ++colId) {
        if (m_columns[colId].visible) {
            m_colIdToSection[colId] = m_sectionToColId.size();
            m_sectionToColId.append(colId);
        }
    }
}

// 接收轨迹数据，预计算统计值（最大速度、最大加速度、总距离），通知View刷新。
void TrajectoryTableModel::setTrajectoryData(const core::TrajectoryResult& data)
{
    beginResetModel();

    // QVector 使用隐式共享（COW），此赋值只增加引用计数，不会深拷贝
    m_data = data;

    // 预计算统计值
    m_maxSpeed      = 0.0;
    m_maxAcc        = 0.0;
    m_totalDistance = 0.0;

    if (data.success && !data.points.isEmpty()) {
        for (int i = 0; i < data.points.size(); ++i) {
            const auto& p = data.points[i];

            double speed = std::sqrt(p.vx * p.vx + p.vy * p.vy + p.vz * p.vz);
            if (speed > m_maxSpeed) m_maxSpeed = speed;

            double acc = std::sqrt(p.ax * p.ax + p.ay * p.ay + p.az * p.az);
            if (acc > m_maxAcc) m_maxAcc = acc;

            if (i > 0) {
                const auto& prev = data.points[i - 1];
                double dx = p.x - prev.x;
                double dy = p.y - prev.y;
                double dz = p.z - prev.z;
                m_totalDistance += std::sqrt(dx * dx + dy * dy + dz * dz);
            }
        }
    }

    endResetModel();
}

void TrajectoryTableModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_maxSpeed      = 0.0;
    m_maxAcc        = 0.0;
    m_totalDistance = 0.0;
    endResetModel();
}

// ---------- QAbstractTableModel 接口 ----------
int TrajectoryTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || m_data.points.isEmpty()) return 0;
    return m_data.points.size();
}

int TrajectoryTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_sectionToColId.size();
}

QVariant TrajectoryTableModel::data(const QModelIndex& index, int role) const
{
    if (m_data.points.isEmpty() || !index.isValid()) return {};

    int row   = index.row();
    int colId = columnIdForSection(index.column());

    if (row < 0 || row >= m_data.points.size() || colId < 0)
        return {};

    const auto& point = m_data.points[row];

    // ----- 显示角色 -----
    if (role == Qt::DisplayRole) {
        switch (colId) {
        case COL_TIME:           return formatValue(point.time, 4);
        case COL_POS_X:          return formatValue(point.x);
        case COL_POS_Y:          return formatValue(point.y);
        case COL_POS_Z:          return formatValue(point.z);
        case COL_VEL_X:          return formatValue(point.vx);
        case COL_VEL_Y:          return formatValue(point.vy);
        case COL_VEL_Z:          return formatValue(point.vz);
        case COL_ACC_X:          return formatValue(point.ax);
        case COL_ACC_Y:          return formatValue(point.ay);
        case COL_ACC_Z:          return formatValue(point.az);
        case COL_JERK_X:         return formatValue(point.jx);
        case COL_JERK_Y:         return formatValue(point.jy);
        case COL_JERK_Z:         return formatValue(point.jz);
        case COL_TOTAL_DISTANCE: return formatValue(m_totalDistance);
        case COL_TOTAL_SPEED:
            return formatValue(std::sqrt(point.vx * point.vx +
                                         point.vy * point.vy +
                                         point.vz * point.vz));
        case COL_TOTAL_ACC:
            return formatValue(std::sqrt(point.ax * point.ax +
                                         point.ay * point.ay +
                                         point.az * point.az));
        default: return {};
        }
    }

    // ----- 前景色 -----
    if (role == Qt::ForegroundRole) {
        return QColor("#7aa8cc");
    }

    // ----- 背景色（速度和加速度的颜色编码） -----
    if (role == Qt::BackgroundRole) {
        if (colId == COL_TOTAL_SPEED && m_maxSpeed > 0) {
            double speed = std::sqrt(point.vx * point.vx +
                                     point.vy * point.vy +
                                     point.vz * point.vz);
            return getColorForValue(speed / m_maxSpeed, 0.0, 1.0);
        }
        if (colId == COL_TOTAL_ACC && m_maxAcc > 0) {
            double acc = std::sqrt(point.ax * point.ax +
                                   point.ay * point.ay +
                                   point.az * point.az);
            QColor c = getColorForValue(acc / m_maxAcc, 0.0, 1.0);
            c.setAlpha(128);
            return c;
        }
    }

    // ----- 对齐 -----
    if (role == Qt::TextAlignmentRole) {
        return int(Qt::AlignRight | Qt::AlignVCenter);
    }

    // ----- 用户数据（供 CSV 导出等使用）-----
    if (role == Qt::UserRole) {
        if (colId == COL_TIME) return point.time;
    }

    return {};
}

QVariant TrajectoryTableModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    int colId = columnIdForSection(section);
    if (colId < 0 || colId >= COL_COUNT) return {};

    return m_columns[colId].name + QStringLiteral(" (") + m_columns[colId].unit + QStringLiteral(")");
}

// ---------- 列可见性 ----------
void TrajectoryTableModel::setColumnVisible(int colId, bool visible)
{
    if (colId < 0 || colId >= COL_COUNT) return;
    if (m_columns[colId].visible == visible) return;

    // begin/endResetModel 会让 View 完全重建列
    beginResetModel();
    m_columns[colId].visible = visible;
    rebuildColumnMapping();
    endResetModel();
}

bool TrajectoryTableModel::isColumnVisible(int colId) const
{
    if (colId < 0 || colId >= COL_COUNT) return false;
    return m_columns[colId].visible;
}

int TrajectoryTableModel::visibleColumnCount() const
{
    return m_sectionToColId.size();
}

int TrajectoryTableModel::columnIdForSection(int section) const
{
    if (section < 0 || section >= m_sectionToColId.size()) return -1;
    return m_sectionToColId[section];
}

int TrajectoryTableModel::sectionForColumnId(int colId) const
{
    if (colId < 0 || colId >= COL_COUNT) return -1;
    return m_colIdToSection[colId];
}

// ---------- 工具函数 ----------
QString TrajectoryTableModel::formatValue(double value, int precision)
{
    if (std::isnan(value) || std::isinf(value)) return QStringLiteral("NaN");
    if (std::abs(value) < 1e-10)               return QStringLiteral("0.000");
    return QString::number(value, 'f', precision);
}

QColor TrajectoryTableModel::getColorForValue(double value, double min, double max)
{
    if (value < min) value = min;
    if (value > max) value = max;

    double normalized = (max > min) ? (value - min) / (max - min) : 0.0;

    int r = 0, g = 0, b = 0;
    if (normalized < 0.25) {
        double t = normalized / 0.25;
        g = static_cast<int>(t * 255);
        b = 255;
    } else if (normalized < 0.5) {
        double t = (normalized - 0.25) / 0.25;
        r = 0;
        g = 255;
        b = static_cast<int>((1 - t) * 255);
    } else if (normalized < 0.75) {
        double t = (normalized - 0.5) / 0.25;
        r = static_cast<int>(t * 255);
        g = 255;
        b = 0;
    } else {
        double t = (normalized - 0.75) / 0.25;
        r = 255;
        g = static_cast<int>((1 - t) * 255);
        b = 0;
    }
    return QColor(r, g, b, 50);
}
