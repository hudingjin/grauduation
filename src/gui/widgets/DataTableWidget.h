#ifndef DATATABLEWIDGET_H
#define DATATABLEWIDGET_H

#include <QObject>
#include <QTableWidget>
#include <QDebug>
#include "../core/FlightParams.h"

namespace core {
    struct TrajectoryResult;
}

class DataTableWidget : public QObject
{
    Q_OBJECT

public:
    explicit DataTableWidget(QTableWidget* tableWidget, QObject* parent = nullptr);
    ~DataTableWidget();

    // 设置轨迹数据
    void setTrajectoryData(const core::TrajectoryResult& data);

    // 更新当前选中行
    void updateCurrentRow(double time);

    // 清空表格
    void clearTable();

    // 获取当前选中的行
    int getCurrentRow() const;

    // 获取总行数
    int getRowCount() const;

    // 导出功能
    bool exportToCSV(const QString& filename);
    bool exportToExcel(const QString& filename);

    // 列管理功能
    void showColumnSelectorDialog();

private:
    // 初始化表格
    void initTable();

    // 更新表格列显示
    void updateTableColumns();

    // 格式化数值显示
    QString formatValue(double value, int precision = 3) const;

    // 颜色渐变
    QColor getColorForValue(double value, double min, double max) const;

    // 获取当前时间点的行索引
    int findRowByTime(double time) const;

    // 获取列是否可见
    bool isColumnVisible(int column) const;

    // 初始化列信息
    void initColumnInfo();

private:
    QTableWidget* m_tableWidget;
    int m_currentRow = -1;
    double m_currentTime = 0.0;
    const core::TrajectoryResult* m_trajectoryData = nullptr;

    // 列信息
    struct ColumnInfo {
        QString name;
        QString unit;
        bool visible;
    };

    QVector<ColumnInfo> m_columnInfos;

    // 列定义
    enum TableColumn {
        COL_TIME = 0,
        COL_POS_X,
        COL_POS_Y,
        COL_POS_Z,
        COL_VEL_X,
        COL_VEL_Y,
        COL_VEL_Z,
        COL_ACC_X,
        COL_ACC_Y,
        COL_ACC_Z,
        COL_JERK_X,
        COL_JERK_Y,
        COL_JERK_Z,
        COL_TOTAL_DISTANCE,
        COL_TOTAL_SPEED,
        COL_TOTAL_ACC,
        COL_COUNT
    };
};

#endif // DATATABLEWIDGET_H