#include "DataTableWidget.h"

#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QHeaderView>
#include <QFont>
#include <QScrollBar>
#include <QLabel>
#include <QDateTime>
#include <QCheckBox>
#include <QGroupBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <cmath>

// Qt6 兼容
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringConverter>
#endif

// ===================================================================
// 构造 / 析构
// ===================================================================

DataTableWidget::DataTableWidget(QTableView* tableView, QObject* parent)
    : QObject(parent)
    , m_tableView(tableView)
{
    qDebug() << "DataTableWidget 构造函数，使用 QTableView:" << tableView;

    if (!m_tableView) {
        qCritical() << "错误：传入的表格视图为空指针！";
        return;
    }

    // 创建 Model
    m_model = new TrajectoryTableModel(this);

    // 将 Model 设置到 View
    m_tableView->setModel(m_model);

    initView();
}

DataTableWidget::~DataTableWidget()
{
    qDebug() << "DataTableWidget 析构函数";
}

void DataTableWidget::initView()
{
    if (!m_tableView) return;

    // 基本行为
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(false);   // 轨迹数据按时间有序，不排序
    m_tableView->setShowGrid(true);

    // 字体
    QFont tableFont(QStringLiteral("Share Tech Mono"), 9);
    m_tableView->setFont(tableFont);

    // 样式（与原 QTableWidget 样式一致，但改用 QTableView 选择器）
    m_tableView->setStyleSheet(QStringLiteral(R"(
        QTableView {
            background-color: #040f1c;
            border: 1px solid rgba(0, 100, 160, 0.3);
            font-family: "Share Tech Mono", monospace;
            font-size: 10px;
            gridline-color: rgba(0, 100, 160, 0.15);
            alternate-background-color: #051830;
        }
        QTableView::item {
            color: #7aa8cc;
            padding: 4px 8px;
        }
        QTableView::item:selected {
            background-color: rgba(0, 212, 255, 0.15);
        }
        QHeaderView::section {
            background-color: #061424;
            color: rgb(0, 212, 255);
            font-family: "Share Tech Mono", monospace;
            font-size: 9px;
            padding: 0px;
            border: none;
            border-bottom: 1px solid rgba(0, 212, 255, 0.3);
        }
        QScrollBar:vertical {
            background: #040f1c;
            width: 4px;
            margin: 0px;
            padding: 0px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: rgba(0, 212, 255, 0.5);
            border-radius: 2px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #00d4ff;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
        }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal {
            background: #040f1c;
            height: 4px;
            margin: 0px;
            padding: 0px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background: rgba(0, 212, 255, 0.5);
            border-radius: 2px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #00d4ff;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal {
            width: 0px;
            background: transparent;
        }
        QScrollBar::add-page:horizontal,
        QScrollBar::sub-page:horizontal {
            background: transparent;
        }
    )"));

    // 表头
    QFont headerFont(QStringLiteral("Share Tech Mono"), 9, QFont::Bold);
    m_tableView->horizontalHeader()->setFont(headerFont);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setDefaultSectionSize(100);

    // 行高
    m_tableView->verticalHeader()->setDefaultSectionSize(22);
    m_tableView->verticalHeader()->setFont(tableFont);
}

// ===================================================================
// 数据设置
// ===================================================================

void DataTableWidget::setTrajectoryData(const core::TrajectoryResult& data)
{
    if (!data.success || data.points.isEmpty()) {
        qWarning() << "轨迹数据无效，无法显示表格";
        clearTable();
        return;
    }

    qDebug() << "开始设置表格数据（Model/View 虚拟化），点数:" << data.points.size();

    // Model 只存指针 + 预计算统计值，不拷贝全量数据
    m_model->setTrajectoryData(data);

    m_currentRow  = -1;
    m_currentTime = 0.0;

    // 选中第一行
    if (!data.points.isEmpty()) {
        updateCurrentRow(data.points.first().time);
    }

    qDebug() << "表格数据设置完成，行数:" << data.points.size()
             << "最大速度:" << m_model->maxSpeed() << "m/s"
             << "最大加速度:" << m_model->maxAcceleration() << "m/s²"
             << "总距离:" << m_model->totalDistance() << "m";
}

void DataTableWidget::clearTable()
{
    m_model->clear();
    m_currentRow  = -1;
    m_currentTime = 0.0;
    qDebug() << "数据表格已清空";
}

void DataTableWidget::updateCurrentRow(double time)
{
    const core::TrajectoryResult& data = m_model->trajectoryData();
    if (data.points.isEmpty()) return;

    m_currentTime = time;
    // 二分查找：找到最接近当前时间的行索引
    int newRow = findRowByTime(time);
    if (newRow < 0 || newRow >= data.points.size()) return;

    // 取消旧行选中
    m_tableView->selectionModel()->clearSelection();

    // 选中新行
    QModelIndex idx = m_model->index(newRow, 0);
    m_tableView->selectionModel()->select(
        idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    m_tableView->scrollTo(idx, QAbstractItemView::PositionAtCenter);

    m_currentRow = newRow;
}

int DataTableWidget::getCurrentRow() const
{
    return m_currentRow;
}

int DataTableWidget::getRowCount() const
{
    return m_model->rowCount();
}

// 二分查找时间对应的行

int DataTableWidget::findRowByTime(double time) const
{
    const core::TrajectoryResult& data = m_model->trajectoryData();
    if (data.points.isEmpty()) return -1;

    int left = 0, right = data.points.size() - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        double midTime = data.points[mid].time;
        if (std::abs(midTime - time) < 1e-6) return mid;
        if (midTime < time) left = mid + 1;
        else                right = mid - 1;
    }

    if (right < 0) return 0;
    if (left >= data.points.size()) return data.points.size() - 1;

    return (std::abs(data.points[left].time - time) <
            std::abs(data.points[right].time - time)) ? left : right;
}

// CSV 导出（直接从 Model 遍历数据，不依赖 WidgetItem）

bool DataTableWidget::exportToCSV(const QString& filename)
{
    const core::TrajectoryResult& data = m_model->trajectoryData();
    if (data.points.isEmpty()) {
        qWarning() << "没有数据可导出";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件:" << filename;
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    stream << QStringLiteral("\"导出时间\",\"%1\"\n\n")
                  .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    // 写表头（遍历 Model 可见列）
    int visCols = m_model->visibleColumnCount();
    for (int sec = 0; sec < visCols; ++sec) {
        stream << m_model->headerData(sec, Qt::Horizontal, Qt::DisplayRole).toString();
        if (sec < visCols - 1) stream << ",";
    }
    stream << "\n";

    // 写数据行
    for (int row = 0; row < data.points.size(); ++row) {
        for (int sec = 0; sec < visCols; ++sec) {
            QModelIndex idx = m_model->index(row, sec);
            stream << m_model->data(idx, Qt::DisplayRole).toString();
            if (sec < visCols - 1) stream << ",";
        }
        stream << "\n";
    }

    file.close();
    qDebug() << "数据已导出到CSV:" << filename;
    return true;
}

bool DataTableWidget::exportToExcel(const QString& filename)
{
    QString csvFile = filename;
    if (!csvFile.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive))
        csvFile += QStringLiteral(".csv");
    return exportToCSV(csvFile);
}

// ===================================================================
// 列选择对话框（操作 Model 的列可见性）
// ===================================================================

void DataTableWidget::showColumnSelectorDialog()
{
    if (!m_tableView) return;

    QDialog dialog(m_tableView->window());
    dialog.setWindowTitle(QStringLiteral("选择显示列"));
    dialog.setMinimumSize(400, 500);

    auto* mainLayout = new QVBoxLayout(&dialog);

    auto* scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);

    auto* scrollContent = new QWidget;
    auto* contentLayout = new QVBoxLayout(scrollContent);

    auto* posGroup  = new QGroupBox(QStringLiteral("位置信息"), scrollContent);
    auto* posLayout  = new QVBoxLayout(posGroup);
    auto* velGroup  = new QGroupBox(QStringLiteral("速度信息"), scrollContent);
    auto* velLayout  = new QVBoxLayout(velGroup);
    auto* accGroup  = new QGroupBox(QStringLiteral("加速度信息"), scrollContent);
    auto* accLayout  = new QVBoxLayout(accGroup);
    auto* statGroup = new QGroupBox(QStringLiteral("统计信息"), scrollContent);
    auto* statLayout = new QVBoxLayout(statGroup);

    for (int colId = 0; colId < TrajectoryTableModel::COL_COUNT; ++colId) {
        auto* cb = new QCheckBox(
            m_model->headerData(m_model->sectionForColumnId(colId),
                                Qt::Horizontal).toString(),
            scrollContent);
        cb->setChecked(m_model->isColumnVisible(colId));
        cb->setProperty("columnId", colId);

        if (colId <= TrajectoryTableModel::COL_POS_Z)
            posLayout->addWidget(cb);
        else if (colId <= TrajectoryTableModel::COL_VEL_Z)
            velLayout->addWidget(cb);
        else if (colId <= TrajectoryTableModel::COL_JERK_Z)
            accLayout->addWidget(cb);
        else
            statLayout->addWidget(cb);
    }

    contentLayout->addWidget(posGroup);
    contentLayout->addWidget(velGroup);
    contentLayout->addWidget(accGroup);
    contentLayout->addWidget(statGroup);
    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        const auto& checkboxes = dialog.findChildren<QCheckBox*>();
        for (const auto* cb : checkboxes) {
            int colId = cb->property("columnId").toInt();
            if (colId >= 0 && colId < TrajectoryTableModel::COL_COUNT)
                m_model->setColumnVisible(colId, cb->isChecked());
        }
    }
}
