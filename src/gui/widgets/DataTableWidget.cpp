#include "DataTableWidget.h"

#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QFont>
#include <QScrollBar>
#include <QLabel>
#include <cmath>

#include "TrajectoryGenerator.h"

// Qt6兼容
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringConverter>
#endif

DataTableWidget::DataTableWidget(QTableWidget* tableWidget, QObject* parent)
    : QObject(parent)
    , m_tableWidget(tableWidget)
    , m_currentRow(-1)
    , m_currentTime(0.0)
    , m_trajectoryData(nullptr)
{
    qDebug() << "DataTableWidget构造函数，使用UI表格:" << tableWidget;

    if (!m_tableWidget) {
        qCritical() << "错误：传入的表格控件为空指针！";
        return;
    }

    // 初始化列信息
    initColumnInfo();

    // 初始化表格
    initTable();
}

DataTableWidget::~DataTableWidget()
{
    qDebug() << "DataTableWidget析构函数";
}

void DataTableWidget::initColumnInfo()
{
    m_columnInfos.resize(COL_COUNT);

    // 时间
    m_columnInfos[COL_TIME] = {"时间", "s", true};

    // 位置
    m_columnInfos[COL_POS_X] = {"位置 X", "m", true};
    m_columnInfos[COL_POS_Y] = {"位置 Y", "m", true};
    m_columnInfos[COL_POS_Z] = {"位置 Z", "m", true};

    // 速度
    m_columnInfos[COL_VEL_X] = {"速度 Vx", "m/s", true};
    m_columnInfos[COL_VEL_Y] = {"速度 Vy", "m/s", true};
    m_columnInfos[COL_VEL_Z] = {"速度 Vz", "m/s", true};

    // 加速度
    m_columnInfos[COL_ACC_X] = {"加速度 Ax", "m/s²", false};
    m_columnInfos[COL_ACC_Y] = {"加速度 Ay", "m/s²", false};
    m_columnInfos[COL_ACC_Z] = {"加速度 Az", "m/s²", false};

    // 加加速度
    m_columnInfos[COL_JERK_X] = {"加加速度 Jx", "m/s³", false};
    m_columnInfos[COL_JERK_Y] = {"加加速度 Jy", "m/s³", false};
    m_columnInfos[COL_JERK_Z] = {"加加速度 Jz", "m/s³", false};

    // 统计信息
    m_columnInfos[COL_TOTAL_DISTANCE] = {"总距离", "m", false};
    m_columnInfos[COL_TOTAL_SPEED] = {"总速度", "m/s", false};
    m_columnInfos[COL_TOTAL_ACC] = {"总加速度", "m/s²", false};
}

void DataTableWidget::initTable()
{
    if (!m_tableWidget) return;

    // 设置表格属性
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSortingEnabled(true);

    // 设置字体
    QFont tableFont("Share Tech Mono", 9);
    m_tableWidget->setFont(tableFont);

    // 设置样式 - 使用您提供的样式
    m_tableWidget->setStyleSheet(R"(
        QTableWidget {
            background-color: #040f1c;
            border: 1px solid rgba(0, 100, 160, 0.3);
            font-family: "Share Tech Mono", monospace;
            font-size: 10px;
            gridline-color: rgba(0, 100, 160, 0.15);
        }
        QTableWidget::item {
            color: #7aa8cc;
            padding: 4px 8px;
        }
        QTableWidget::item:selected {
            background-color: rgba(0, 212, 255, 0.15);
        }
        QHeaderView::section {
            background-color: #061424;
            color:  rgb(0, 212, 255);
            font-family: "Share Tech Mono", monospace;
            font-size: 9px;
            padding: 0px;
            border: none;
            border-bottom: 1px solid rgba(0, 212, 255, 0.3);
        }
        /* 表格滚动条 */
        /* 全局滚动条样式 - 垂直 */
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

        /* 全局滚动条样式 - 水平 */
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
    )");

    // 更新表格列
    updateTableColumns();
}

void DataTableWidget::setTrajectoryData(const core::TrajectoryResult& data)
{
    m_trajectoryData = &data;
    clearTable();

    if (!data.success || data.points.isEmpty()) {
        qWarning() << "轨迹数据无效，无法显示表格";
        return;
    }

    qDebug() << "开始设置表格数据，点数:" << data.points.size();

    // 计算统计信息
    double maxSpeed = 0.0;
    double maxAcc = 0.0;
    double totalDistance = 0.0;

    // 设置行数
    m_tableWidget->setRowCount(data.points.size());

    for (int i = 0; i < data.points.size(); ++i) {
        const auto& point = data.points[i];

        // 计算统计量
        double speed = sqrt(point.vx * point.vx + point.vy * point.vy + point.vz * point.vz);
        maxSpeed = qMax(maxSpeed, speed);

        double acc = sqrt(point.ax * point.ax + point.ay * point.ay + point.az * point.az);
        maxAcc = qMax(maxAcc, acc);

        if (i > 0) {
            const auto& prevPoint = data.points[i-1];
            double dx = point.x - prevPoint.x;
            double dy = point.y - prevPoint.y;
            double dz = point.z - prevPoint.z;
            totalDistance += sqrt(dx*dx + dy*dy + dz*dz);
        }

        // 填充数据到可见列
        int tableCol = 0;
        for (int colId = 0; colId < COL_COUNT; ++colId) {
            if (!isColumnVisible(colId)) continue;

            QString value = "";
            QTableWidgetItem* item = nullptr;

            switch (colId) {
            case COL_TIME:
                value = formatValue(point.time, 4);
                item = new QTableWidgetItem(value);
                item->setData(Qt::UserRole, point.time);
                break;
            case COL_POS_X: value = formatValue(point.x); break;
            case COL_POS_Y: value = formatValue(point.y); break;
            case COL_POS_Z: value = formatValue(point.z); break;
            case COL_VEL_X: value = formatValue(point.vx); break;
            case COL_VEL_Y: value = formatValue(point.vy); break;
            case COL_VEL_Z: value = formatValue(point.vz); break;
            case COL_ACC_X: value = formatValue(point.ax); break;
            case COL_ACC_Y: value = formatValue(point.ay); break;
            case COL_ACC_Z: value = formatValue(point.az); break;
            case COL_JERK_X: value = formatValue(point.jx); break;
            case COL_JERK_Y: value = formatValue(point.jy); break;
            case COL_JERK_Z: value = formatValue(point.jz); break;
            case COL_TOTAL_DISTANCE: value = formatValue(totalDistance); break;
            case COL_TOTAL_SPEED: value = formatValue(speed); break;
            case COL_TOTAL_ACC: value = formatValue(acc); break;
            }

            if (!item) {
                item = new QTableWidgetItem(value);
            }

            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

            // 为速度和加速度添加颜色编码
            if (colId == COL_TOTAL_SPEED && maxSpeed > 0) {
                double ratio = speed / maxSpeed;
                item->setBackground(getColorForValue(ratio, 0, 1));
            } else if (colId == COL_TOTAL_ACC && maxAcc > 0) {
                double ratio = acc / maxAcc;
                QColor color = getColorForValue(ratio, 0, 1);
                color.setAlpha(128);
                item->setBackground(color);
            }

            m_tableWidget->setItem(i, tableCol, item);
            tableCol++;
        }

        // 进度更新
        if (i % 100 == 0) {
            QApplication::processEvents();
        }
    }

    // 重置当前行
    m_currentRow = -1;
    m_currentTime = 0.0;

    // 如果有数据，选中第一行
    if (data.points.size() > 0) {
        updateCurrentRow(data.points.first().time);
    }

    qDebug() << "表格数据设置完成，行数:" << data.points.size();
    qDebug() << "统计信息 - 最大速度:" << maxSpeed << "m/s, 最大加速度:" << maxAcc << "m/s², 总距离:" << totalDistance << "m";
}

void DataTableWidget::updateCurrentRow(double time)
{
    if (!m_trajectoryData || m_trajectoryData->points.isEmpty()) {
        return;
    }

    m_currentTime = time;
    int newRow = findRowByTime(time);

    if (newRow < 0 || newRow >= m_trajectoryData->points.size()) {
        return;
    }

    // 清除之前的选中样式
    if (m_currentRow >= 0 && m_currentRow < m_tableWidget->rowCount()) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem* oldItem = m_tableWidget->item(m_currentRow, col);
            if (oldItem) {
                oldItem->setBackground(QBrush(Qt::transparent));
                oldItem->setForeground(QColor("#7aa8cc"));
            }
        }
    }

    // 设置新的选中样式
    m_currentRow = newRow;

    if (m_currentRow < m_tableWidget->rowCount()) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = m_tableWidget->item(m_currentRow, col);
            if (item) {
                item->setBackground(QColor(13, 58, 92));  // 深蓝色背景
                item->setForeground(Qt::white);
            }
        }

        // 滚动到当前行
        m_tableWidget->scrollToItem(m_tableWidget->item(m_currentRow, 0),
                                   QAbstractItemView::PositionAtCenter);
        m_tableWidget->selectRow(m_currentRow);
    }
}

void DataTableWidget::clearTable()
{
    if (m_tableWidget) {
        m_tableWidget->clearContents();
        m_tableWidget->setRowCount(0);
    }

    m_currentRow = -1;
    m_currentTime = 0.0;
    m_trajectoryData = nullptr;

    qDebug() << "数据表格已清空";
}

int DataTableWidget::getCurrentRow() const
{
    return m_currentRow;
}

int DataTableWidget::getRowCount() const
{
    return m_tableWidget ? m_tableWidget->rowCount() : 0;
}

bool DataTableWidget::exportToCSV(const QString& filename)
{
    if (!m_tableWidget || m_tableWidget->rowCount() == 0) {
        qWarning() << "没有数据可导出";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件:" << filename;
        return false;
    }

    QTextStream stream(&file);

    // Qt6兼容性
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    // 写入表头
    for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
        QTableWidgetItem* headerItem = m_tableWidget->horizontalHeaderItem(col);
        if (headerItem) {
            stream << headerItem->text();
        }
        if (col < m_tableWidget->columnCount() - 1) {
            stream << ",";
        }
    }
    stream << "\n";

    // 写入数据
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (item) {
                stream << item->text();
            }
            if (col < m_tableWidget->columnCount() - 1) {
                stream << ",";
            }
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
    if (!csvFile.endsWith(".csv", Qt::CaseInsensitive)) {
        csvFile += ".csv";
    }
    return exportToCSV(csvFile);
}

QString DataTableWidget::formatValue(double value, int precision) const
{
    if (qIsNaN(value) || qIsInf(value)) {
        return "NaN";
    }

    if (qAbs(value) < 1e-10) {
        return "0.000";
    }

    return QString::number(value, 'f', precision);
}

QColor DataTableWidget::getColorForValue(double value, double min, double max) const
{
    if (value < min) value = min;
    if (value > max) value = max;

    double normalized = (value - min) / (max - min);

    int r, g, b;
    if (normalized < 0.25) {
        double t = normalized / 0.25;
        r = 0;
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

int DataTableWidget::findRowByTime(double time) const
{
    if (!m_trajectoryData || m_trajectoryData->points.isEmpty()) {
        return -1;
    }

    int left = 0;
    int right = m_trajectoryData->points.size() - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        double midTime = m_trajectoryData->points[mid].time;

        if (qAbs(midTime - time) < 1e-6) {
            return mid;
        } else if (midTime < time) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    if (right < 0) return 0;
    if (left >= m_trajectoryData->points.size()) return m_trajectoryData->points.size() - 1;

    double leftTime = m_trajectoryData->points[left].time;
    double rightTime = m_trajectoryData->points[right].time;

    if (qAbs(leftTime - time) < qAbs(rightTime - time)) {
        return left;
    } else {
        return right;
    }
}

void DataTableWidget::updateTableColumns()
{
    if (!m_tableWidget) return;

    // 计算可见列数
    int visibleCount = 0;
    for (int i = 0; i < m_columnInfos.size(); ++i) {
        if (m_columnInfos[i].visible) {
            visibleCount++;
        }
    }

    // 设置列数
    m_tableWidget->setColumnCount(visibleCount);

    // 设置表头
    QStringList headers;
    for (int i = 0; i < m_columnInfos.size(); ++i) {
        if (m_columnInfos[i].visible) {
            headers << m_columnInfos[i].name + " (" + m_columnInfos[i].unit + ")";
        }
    }

    m_tableWidget->setHorizontalHeaderLabels(headers);

    // 设置列宽
    for (int i = 0; i < visibleCount; ++i) {
        m_tableWidget->setColumnWidth(i, 100);
    }

    // 设置表头字体
    QFont headerFont("Share Tech Mono", 9, QFont::Bold);
    m_tableWidget->horizontalHeader()->setFont(headerFont);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
}

void DataTableWidget::showColumnSelectorDialog()
{
    if (!m_tableWidget) return;

    QDialog dialog(m_tableWidget->window());
    dialog.setWindowTitle("选择显示列");
    dialog.setMinimumSize(400, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // 创建滚动区域
    QScrollArea* scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);

    QWidget* scrollContent = new QWidget;
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);

    // 按类别分组
    QGroupBox* posGroup = new QGroupBox("位置信息", scrollContent);
    QVBoxLayout* posLayout = new QVBoxLayout(posGroup);

    QGroupBox* velGroup = new QGroupBox("速度信息", scrollContent);
    QVBoxLayout* velLayout = new QVBoxLayout(velGroup);

    QGroupBox* accGroup = new QGroupBox("加速度信息", scrollContent);
    QVBoxLayout* accLayout = new QVBoxLayout(accGroup);

    QGroupBox* statGroup = new QGroupBox("统计信息", scrollContent);
    QVBoxLayout* statLayout = new QVBoxLayout(statGroup);

    // 添加复选框
    for (int i = 0; i < m_columnInfos.size(); ++i) {
        const auto& colInfo = m_columnInfos[i];
        QCheckBox* checkBox = new QCheckBox(colInfo.name + " (" + colInfo.unit + ")", scrollContent);
        checkBox->setChecked(colInfo.visible);
        checkBox->setProperty("columnId", i);

        // 按类别分组
        if (i <= COL_POS_Z) {
            posLayout->addWidget(checkBox);
        } else if (i <= COL_VEL_Z) {
            velLayout->addWidget(checkBox);
        } else if (i <= COL_JERK_Z) {
            accLayout->addWidget(checkBox);
        } else {
            statLayout->addWidget(checkBox);
        }
    }

    // 添加到内容布局
    contentLayout->addWidget(posGroup);
    contentLayout->addWidget(velGroup);
    contentLayout->addWidget(accGroup);
    contentLayout->addWidget(statGroup);
    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // 收集所有复选框
        QList<QCheckBox*> checkboxes = dialog.findChildren<QCheckBox*>();
        for (QCheckBox* checkbox : checkboxes) {
            int colId = checkbox->property("columnId").toInt();
            if (colId >= 0 && colId < m_columnInfos.size()) {
                m_columnInfos[colId].visible = checkbox->isChecked();
            }
        }

        // 更新表格
        updateTableColumns();

        // 重新填充数据
        if (m_trajectoryData) {
            setTrajectoryData(*m_trajectoryData);
        }
    }
}

bool DataTableWidget::isColumnVisible(int column) const
{
    if (column >= 0 && column < m_columnInfos.size()) {
        return m_columnInfos[column].visible;
    }
    return false;
}