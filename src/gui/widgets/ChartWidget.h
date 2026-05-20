#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QColor>

// 前向声明
class QWidget;
class QGraphicsLineItem;

// 不声明Qt Charts类，在实现中包含
QT_BEGIN_NAMESPACE
class QChartView;
class QChart;
class QSplineSeries;
class QValueAxis;
QT_END_NAMESPACE

class ChartWidget : public QObject
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* container, QObject* parent = nullptr);
    ~ChartWidget() override;

    // 设置数据
    void setData(const QVector<QPointF>& xData,
                 const QVector<QPointF>& yData,
                 const QVector<QPointF>& zData = QVector<QPointF>());

    // 设置坐标轴标签
    void setAxisLabels(const QString& xLabel, const QString& yLabel) const;

    // 设置系列颜色
    void setSeriesColor(int index, const QColor& color) const;

    // 添加参考线
    static void addReferenceLine(double value, const QColor& color, const QString& label = "");

    // 清空
    void clear();
    static QVector<QPointF> downsampleData(const QVector<QPointF>& data, int maxPoints);

    void updateCurrentTime(double time);
    void clearChart() { clear(); }  // 为了向后兼容
    void setDataForDOF(const QVector<QPointF>& xData,
                   const QVector<QPointF>& yData = QVector<QPointF>(),
                   const QVector<QPointF>& zData = QVector<QPointF>(),
                   int dof = 3);

    // 获取系列对象
    [[nodiscard]] QSplineSeries* getSeriesX() const { return m_seriesX; }
    [[nodiscard]] QSplineSeries* getSeriesY() const { return m_seriesY; }
    [[nodiscard]] QSplineSeries* getSeriesZ() const { return m_seriesZ; }

    // 获取图表视图
    [[nodiscard]] QChartView* getChartView() const { return m_chartView; }

private:
    double m_xMin = 0.0;
    double m_xMax = 1.0;
    double m_yMin = 0.0;
    double m_yMax = 1.0;
    QChart* m_chart;
    QChartView* m_chartView;
    QSplineSeries* m_seriesX;
    QSplineSeries* m_seriesY;
    QSplineSeries* m_seriesZ;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;
    QWidget* m_container;
    QGraphicsLineItem* m_timeIndicator = nullptr;
};

#endif // CHARTWIDGET_H