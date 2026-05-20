#include "ChartWidget.h"

// 基础Qt头文件
#include <QVBoxLayout>
#include <QColor>
#include <QPen>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <limits>
#include <QDebug>
#include <QtMath>

// Qt Charts头文件
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>

ChartWidget::ChartWidget(QWidget* container, QObject* parent)
    : QObject(parent)
    , m_chart(new QChart)
    , m_chartView(new QChartView(m_chart))
    , m_seriesX(nullptr)
    , m_seriesY(nullptr)
    , m_seriesZ(nullptr)
    , m_axisX(new QValueAxis)
    , m_axisY(new QValueAxis)
    , m_container(container)
{
    qDebug() << "ChartWidget构造函数，容器:" << container;
    // 1. 设置图表主题和背景
    m_chart->setTheme(QChart::ChartThemeDark);
    m_chart->setBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setBackgroundVisible(false);

    // 2. 设置边距为0，让图表完全填充
    m_chart->setMargins(QMargins(0, 0, 0, 0));

    // 3. 设置绘图区域（plot area）边距
    m_chart->setPlotAreaBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setPlotAreaBackgroundVisible(false);

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet("background: transparent; border: none;");

    //设置容器布局
    if (container) {
        // 确保容器有布局
        if (!container->layout()) {
            qDebug() << "容器无布局，创建新布局";
            auto* layout = new QVBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
        }
        container->layout()->addWidget(m_chartView);
    }

    m_axisX->setLabelsColor(QColor("#7aa8cc"));
    m_axisY->setLabelsColor(QColor("#7aa8cc"));
    m_axisX->setGridLineColor(QColor(0,100,160,40));
    m_axisY->setGridLineColor(QColor(0,100,160,40));
    m_axisX->setTitleFont(QFont("Share Tech Mono", 8));
    m_axisY->setTitleFont(QFont("Share Tech Mono", 8));
    m_axisX->setTitleBrush(QColor("#7aa8cc"));
    m_axisY->setTitleBrush(QColor("#7aa8cc"));

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
}

ChartWidget::~ChartWidget()
{
    m_chart->removeAllSeries();
    if (m_seriesX) {
        delete m_seriesX;
        m_seriesX = nullptr;
    }
    if (m_seriesY) {
        delete m_seriesY;
        m_seriesY = nullptr;
    }
    if (m_seriesZ) {
        delete m_seriesZ;
        m_seriesZ = nullptr;
    }

    // 注意：m_chartView 会在 m_container 销毁时自动清理
    // 但m_chart是m_chartView的子对象，也会被自动清理

    qDebug() << "ChartWidget析构函数";
}

void ChartWidget::setData(const QVector<QPointF>& xData,
                         const QVector<QPointF>& yData,
                         const QVector<QPointF>& zData)
{
    qDebug() << "ChartWidget::setData 被调用";
    qDebug() << "xData点数:" << xData.size();
    qDebug() << "yData点数:" << yData.size();
    qDebug() << "zData点数:" << zData.size();

    if (xData.isEmpty() || yData.isEmpty()) {
        qWarning() << "数据为空，无法设置图表";
        return;
    }

    // 清空现有系列
    m_chart->removeAllSeries();

    // 创建新系列
    m_seriesX = new QSplineSeries();
    m_seriesY = new QSplineSeries();

    // 设置颜色
    m_seriesX->setColor(QColor("#ff3366"));
    m_seriesY->setColor(QColor("#55ddff"));

    // 设置线宽
    QPen penX = m_seriesX->pen();
    penX.setWidthF(1.5);
    m_seriesX->setPen(penX);

    QPen penY = m_seriesY->pen();
    penY.setWidthF(1.5);
    m_seriesY->setPen(penY);

    // 添加数据
    m_seriesX->replace(xData);
    m_seriesY->replace(yData);

    // 添加到图表
    m_chart->addSeries(m_seriesX);
    m_chart->addSeries(m_seriesY);

    // 必须附加到坐标轴！
    m_seriesX->attachAxis(m_axisX);
    m_seriesX->attachAxis(m_axisY);
    m_seriesY->attachAxis(m_axisX);
    m_seriesY->attachAxis(m_axisY);

    // 计算数据范围
    if (!xData.isEmpty()) {
        m_xMin = xData.first().x();
        m_xMax = xData.last().x();

        // 计算Y轴范围
        m_yMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::lowest();

        for (const auto& point : xData) {
            m_yMin = qMin(m_yMin, point.y());
            m_yMax = qMax(m_yMax, point.y());
        }
        for (const auto& point : yData) {
            m_yMin = qMin(m_yMin, point.y());
            m_yMax = qMax(m_yMax, point.y());
        }

        // 添加10%的边距
        double yRange = m_yMax - m_yMin;
        if (yRange < 1e-6) yRange = 1.0;
        m_yMin -= yRange * 0.1;
        m_yMax += yRange * 0.1;
    }

    // 更新坐标轴范围
    if (m_axisX) {
        m_axisX->setRange(m_xMin, m_xMax);
        qDebug() << "X轴范围:" << m_xMin << "->" << m_xMax;
    }
    if (m_axisY) {
        m_axisY->setRange(m_yMin, m_yMax);
        qDebug() << "Y轴范围:" << m_yMin << "->" << m_yMax;
    }

    // 处理Z数据
    if (!zData.isEmpty()) {
        m_seriesZ = new QSplineSeries();
        m_seriesZ->setColor(QColor("#00ff9f"));
        QPen penZ = m_seriesZ->pen();
        penZ.setWidthF(1.5);
        m_seriesZ->setPen(penZ);
        m_seriesZ->replace(zData);
        m_chart->addSeries(m_seriesZ);
        m_seriesZ->attachAxis(m_axisX);
        m_seriesZ->attachAxis(m_axisY);
    } else {
        m_seriesZ = nullptr;
    }
    qDebug() << "图表数据设置完成";
}

void ChartWidget::updateCurrentTime(double time)
{
    if (!m_chart || !m_seriesX || m_seriesX->count() == 0) {
        return;
    }

    // 清理旧的指示器
    if (m_timeIndicator) {
        m_chart->scene()->removeItem(m_timeIndicator);
        delete m_timeIndicator;
        m_timeIndicator = nullptr;
    }

    // 添加除零保护
    if (m_xMax <= 0.0) {
        m_xMax = 1.0;
    }
    if (m_xMin >= m_xMax) {
        m_xMax = m_xMin + 1.0;
    }

    // 计算X轴位置
    QRectF plotArea = m_chart->plotArea();
    qreal xPos = plotArea.left() + ((time - m_xMin) / (m_xMax - m_xMin)) * plotArea.width();

    m_timeIndicator = new QGraphicsLineItem();
    m_timeIndicator->setPen(QPen(QColor(0, 212, 255, 128), 1, Qt::DashLine));
    m_timeIndicator->setLine(xPos, plotArea.top(), xPos, plotArea.bottom());

    m_chart->scene()->addItem(m_timeIndicator);
}
void ChartWidget::setAxisLabels(const QString& xLabel, const QString& yLabel) const{
    m_axisX->setTitleText(xLabel);
    m_axisY->setTitleText(yLabel);
}

void ChartWidget::setSeriesColor(int index, const QColor& color) const{
    QSplineSeries* series = nullptr;

    switch (index) {
    case 0: series = m_seriesX; break;
    case 1: series = m_seriesY; break;
    case 2: series = m_seriesZ; break;
    }

    if (series) {
        series->setColor(color);
    }
}

void ChartWidget::addReferenceLine(double value, const QColor& color, const QString& label)
{
    // 实现参考线添加
    Q_UNUSED(value);
    Q_UNUSED(color);
    Q_UNUSED(label);
    // 这里可以扩展实现参考线功能
}

void ChartWidget::clear()
{
    if (m_timeIndicator) {
        if (m_chart && m_chart->scene()) {
            m_chart->scene()->removeItem(m_timeIndicator);
        }
        delete m_timeIndicator;
        m_timeIndicator = nullptr;
    }

    if (m_chart) {
        m_chart->removeAllSeries();
    }

    m_seriesX = nullptr;
    m_seriesY = nullptr;
    m_seriesZ = nullptr;
    m_xMin = 0.0;
    m_xMax = 1.0;
    m_yMin = 0.0;
    m_yMax = 1.0;
}

QVector<QPointF> ChartWidget::downsampleData(const QVector<QPointF>& data, int maxPoints)
{
    if (data.size() <= maxPoints) {
        return data;
    }

    QVector<QPointF> result;
    int step = data.size() / maxPoints;
    step = qMax(step, 1);

    for (int i = 0; i < data.size(); i += step) {
        result.append(data[i]);
    }

    // 确保包含最后一个点
    if (!data.isEmpty() && (result.isEmpty() || result.last() != data.last())) {
        result.append(data.last());
    }

    return result;
}
/// ChartWidget.cpp - 在setData函数后添加
void ChartWidget::setDataForDOF(const QVector<QPointF>& xData,
                               const QVector<QPointF>& yData,
                               const QVector<QPointF>& zData,
                               int dof)
{
    // ✅ 修复1：添加详细的调试输出
    qDebug() << "[ChartWidget] setDataForDOF 被调用";
    qDebug() << "  DOF =" << dof;
    qDebug() << "  xData大小:" << xData.size();
    qDebug() << "  yData大小:" << yData.size();
    qDebug() << "  zData大小:" << zData.size();

    if (!xData.isEmpty()) {
        qDebug() << "  第一个点: t=" << xData.first().x() << ", y=" << xData.first().y();
        qDebug() << "  最后一个点: t=" << xData.last().x() << ", y=" << xData.last().y();
    }

    // 1. 输入验证
    if (xData.isEmpty()) {
        qWarning() << "X数据为空，无法设置图表";
        return;
    }

    if (!m_chart) {
        qCritical() << "错误：图表对象为空";
        return;
    }

    if (!m_axisX || !m_axisY) {
        qCritical() << "错误：坐标轴未初始化";
        return;
    }
    m_chart->removeAllSeries();
    // 2. 清理现有系列对象（避免内存泄漏）
    if (m_seriesX) {
        delete m_seriesX;
        m_seriesX = nullptr;
    }
    if (m_seriesY) {
        delete m_seriesY;
        m_seriesY = nullptr;
    }
    if (m_seriesZ) {
        delete m_seriesZ;
        m_seriesZ = nullptr;
    }

    // 3. 从图表中移除所有系列
    m_chart->removeAllSeries();

    // 4. 创建X系列（总是显示）
    m_seriesX = new QSplineSeries();
    m_seriesX->setName("X轴");
    m_seriesX->setColor(QColor("#ff3366"));  // 粉色

    QPen penX = m_seriesX->pen();
    penX.setWidthF(1.5);
    m_seriesX->setPen(penX);
    m_seriesX->replace(xData);
    m_chart->addSeries(m_seriesX);

    // 5. 根据DOF决定是否创建Y系列
    if (dof >= 3 && !yData.isEmpty()) {
        m_seriesY = new QSplineSeries();
        m_seriesY->setName("Y轴");
        m_seriesY->setColor(QColor("#55ddff"));  // 青色

        QPen penY = m_seriesY->pen();
        penY.setWidthF(1.5);
        m_seriesY->setPen(penY);
        m_seriesY->replace(yData);
        m_chart->addSeries(m_seriesY);
    }

    // 6. 根据DOF决定是否创建Z系列
    if (dof >= 3 && !zData.isEmpty()) {
        m_seriesZ = new QSplineSeries();
        m_seriesZ->setName("Z轴");
        m_seriesZ->setColor(QColor("#00ff9f"));  // 绿色

        QPen penZ = m_seriesZ->pen();
        penZ.setWidthF(1.5);
        m_seriesZ->setPen(penZ);
        m_seriesZ->replace(zData);
        m_chart->addSeries(m_seriesZ);
    }

    // 7. 附加系列到坐标轴
    m_seriesX->attachAxis(m_axisX);
    m_seriesX->attachAxis(m_axisY);

    if (m_seriesY) {
        m_seriesY->attachAxis(m_axisX);
        m_seriesY->attachAxis(m_axisY);
    }

    if (m_seriesZ) {
        m_seriesZ->attachAxis(m_axisX);
        m_seriesZ->attachAxis(m_axisY);
    }

    // 8. ✅ 修复2：正确的数据范围计算
    if (!xData.isEmpty()) {
        m_xMin = xData.first().x();
        m_xMax = xData.last().x();

        // 如果只有一个点或所有点时间相同，设置合理的X范围
        if (qAbs(m_xMax - m_xMin) < 1e-6) {
            qDebug() << "  [警告] 所有X值相同，扩展X范围";
            m_xMin -= 0.5;
            m_xMax += 0.5;
        }

        qDebug() << "  X轴计算范围: min=" << m_xMin << ", max=" << m_xMax;

        // 初始化Y轴范围
        m_yMin = std::numeric_limits<double>::max();
        m_yMax = std::numeric_limits<double>::lowest();

        // 计算X数据范围
        for (const auto& point : xData) {
            m_yMin = qMin(m_yMin, point.y());
            m_yMax = qMax(m_yMax, point.y());
        }

        // 如果存在Y数据且DOF>=3，计算Y数据范围
        if (dof >= 3 && !yData.isEmpty()) {
            for (const auto& point : yData) {
                m_yMin = qMin(m_yMin, point.y());
                m_yMax = qMax(m_yMax, point.y());
            }
        }

        // 如果存在Z数据且DOF>=3，计算Z数据范围
        if (dof >= 3 && !zData.isEmpty()) {
            for (const auto& point : zData) {
                m_yMin = qMin(m_yMin, point.y());
                m_yMax = qMax(m_yMax, point.y());
            }
        }

        qDebug() << "  Y轴原始范围: min=" << m_yMin << ", max=" << m_yMax;

        // ✅ 修复3：正确处理Y轴范围
        if (qAbs(m_yMax - m_yMin) < 1e-6) {
            // 如果所有Y值都一样，设置一个合理的范围
            qDebug() << "  [警告] 所有Y值相同，扩展Y范围";
            m_yMin -= 1.0;
            m_yMax += 1.0;
        } else {
            // 添加10%的边距
            double yRange = m_yMax - m_yMin;
            m_yMin -= yRange * 0.1;
            m_yMax += yRange * 0.1;
        }

        qDebug() << "  Y轴最终范围: min=" << m_yMin << ", max=" << m_yMax;
    } else {
        // 如果xData为空（理论上不会到这里），设置默认范围
        m_xMin = 0.0;
        m_xMax = 1.0;
        m_yMin = 0.0;
        m_yMax = 1.0;
    }

    // 9. ✅ 修复4：安全的坐标轴范围设置
    try {
        m_axisX->setRange(m_xMin, m_xMax);
        m_axisY->setRange(m_yMin, m_yMax);
        qDebug() << "  [成功] 坐标轴范围设置完成";
    } catch (const std::exception& e) {
        qCritical() << "  [错误] 设置坐标轴范围时异常:" << e.what();
        // 回退到安全范围
        m_axisX->setRange(0.0, 1.0);
        m_axisY->setRange(0.0, 1.0);
    }

    // 10. 更新图例
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    m_chart->legend()->setLabelBrush(QColor("#7aa8cc"));

    qDebug() << "[ChartWidget] setDataForDOF 完成，显示系列:"
             << (m_seriesX ? "X" : "")
             << (m_seriesY ? "Y" : "")
             << (m_seriesZ ? "Z" : "");
}