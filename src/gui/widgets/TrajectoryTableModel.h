#ifndef TRAJECTORYTABLEMODEL_H
#define TRAJECTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QColor>
#include "../../core/TrajectoryGenerator.h"

// 自定义 Model：只存储轨迹数据指针，按需提供数据给 View
// View 只会请求可见行的数据，无论轨迹有多少点都不会阻塞 UI
class TrajectoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // 列定义（与 DataTableWidget 的 TableColumn 枚举保持一致）
    enum Column {
        COL_TIME = 0,
        COL_POS_X, COL_POS_Y, COL_POS_Z,
        COL_VEL_X, COL_VEL_Y, COL_VEL_Z,
        COL_ACC_X, COL_ACC_Y, COL_ACC_Z,
        COL_JERK_X, COL_JERK_Y, COL_JERK_Z,
        COL_TOTAL_DISTANCE, COL_TOTAL_SPEED, COL_TOTAL_ACC,
        COL_COUNT
    };
    Q_ENUM(Column)

    explicit TrajectoryTableModel(QObject* parent = nullptr);

    // 设置数据（Model 拥有数据副本，QVector 使用隐式共享 COW，拷贝代价极低）
    void setTrajectoryData(const core::TrajectoryResult& data);
    void clear();
    const core::TrajectoryResult& trajectoryData() const { return m_data; }

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // 列可见性管理
    void setColumnVisible(int colId, bool visible);
    bool isColumnVisible(int colId) const;
    int visibleColumnCount() const;
    int columnIdForSection(int section) const;   // visible section → colId
    int sectionForColumnId(int colId) const;     // colId → visible section (-1 if hidden)

    // 缓存的统计信息
    double maxSpeed()        const { return m_maxSpeed; }
    double maxAcceleration() const { return m_maxAcc; }
    double totalDistance()   const { return m_totalDistance; }

private:
    void rebuildColumnMapping();

    static QString formatValue(double value, int precision = 3);
    static QColor getColorForValue(double value, double min, double max);

    struct ColInfo {
        QString name;
        QString unit;
        bool    visible = true;
    };
    QVector<ColInfo> m_columns;              // 所有列的信息
    QVector<int>     m_sectionToColId;       // 可见 section → colId
    QVector<int>     m_colIdToSection;       // colId → 可见 section（-1 表示隐藏）

    core::TrajectoryResult m_data;           // Model 拥有数据副本（QVector COW，浅拷贝）

    // 缓存统计值（在 setTrajectoryData 时计算一次）
    double m_maxSpeed      = 0.0;
    double m_maxAcc        = 0.0;
    double m_totalDistance = 0.0;
};

#endif // TRAJECTORYTABLEMODEL_H
