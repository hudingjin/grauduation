#ifndef DATATABLEWIDGET_H
#define DATATABLEWIDGET_H

#include <QObject>
#include <QTableView>
#include <QDebug>
#include "TrajectoryTableModel.h"

namespace core {
    struct TrajectoryResult;
}

class DataTableWidget : public QObject
{
    Q_OBJECT

public:
    // 构造函数接受 QTableView*（由 UI 文件或代码创建）
    explicit DataTableWidget(QTableView* tableView, QObject* parent = nullptr);
    ~DataTableWidget();

    // 设置轨迹数据（Model 内部只存指针，不拷贝全量数据）
    void setTrajectoryData(const core::TrajectoryResult& data);

    // 更新当前选中行（仿真播放时高亮当前时间点对应的行）
    void updateCurrentRow(double time);

    // 清空表格
    void clearTable();

    // 获取当前选中的行 / 总行数
    int getCurrentRow() const;
    int getRowCount() const;

    // 导出功能（遍历 Model 数据，不需要复制 WidgetItem）
    bool exportToCSV(const QString& filename);
    bool exportToExcel(const QString& filename);

    // 列选择对话框
    void showColumnSelectorDialog();

    // 暴露 Model 和 View 给外部（用于创建缩放视图等）
    TrajectoryTableModel* model() const { return m_model; }
    QTableView*           view()  const { return m_tableView; }

private:
    void initView();

    // 二分查找时间对应的行
    int findRowByTime(double time) const;

    // ---------- 成员变量 ----------
    QTableView*            m_tableView  = nullptr;
    TrajectoryTableModel*  m_model      = nullptr;

    int    m_currentRow  = -1;
    double m_currentTime = 0.0;
};

#endif // DATATABLEWIDGET_H
