#include "TitleBarWidget.h"
#include <QTimer>
#include <QTime>
#include <QLabel>
#include <QPixmap>
#include <QPainter>

TitleBarWidget::TitleBarWidget(QLabel* logoLabel,
                               QLabel* statusDot,
                               QLabel* clockLabel,
                               QLabel* titleMain,
                               QLabel* titleSub,
                               QWidget *parent)
    : QWidget(parent)
    , m_logoLabel(logoLabel)
    , m_statusDot(statusDot)
    , m_clockLabel(clockLabel)
    , m_titleMain(titleMain)
    , m_titleSub(titleSub)
    , clockTimer(new QTimer(this))
{
    setupStyles();

    // 启动时钟定时器
    updateClock();
    connect(clockTimer, &QTimer::timeout, this, &TitleBarWidget::updateClock);
    clockTimer->start(1000);
}

TitleBarWidget::~TitleBarWidget()
{
    if (clockTimer) {
        clockTimer->stop();
    }
}

void TitleBarWidget::setupStyles()
{
    // 1. 设置Logo样式
    if (m_logoLabel) {
        QPixmap diamondPixmap(20, 20);
        diamondPixmap.fill(Qt::transparent);

        QPainter painter(&diamondPixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);

        QPolygonF outerDiamond;
        outerDiamond << QPointF(10, 0)
                    << QPointF(20, 10)
                    << QPointF(10, 20)
                    << QPointF(0, 10);

        painter.setBrush(QColor(0, 212, 255));
        painter.drawPolygon(outerDiamond);

        QPolygonF innerDiamond;
        innerDiamond << QPointF(10, 4)
                    << QPointF(16, 10)
                    << QPointF(10, 16)
                    << QPointF(4, 10);

        painter.setBrush(QColor(0, 255, 255, 150));
        painter.drawPolygon(innerDiamond);

        m_logoLabel->setPixmap(diamondPixmap);
        m_logoLabel->setFixedSize(20, 20);
        m_logoLabel->setStyleSheet("background: transparent; border: none;");
    }

    // 2. 设置主标题样式
    if (m_titleMain) {
        m_titleMain->setStyleSheet(
            "QLabel {"
            "   color: #00e0ff;"
            "   font-family: 'Microsoft YaHei UI', 'Segoe UI', sans-serif;"
            "   font-size: 20px;"
            "   font-weight: 600;"
            "   background: transparent;"
            "   padding-left: 12px;"
            "   letter-spacing: 0.8px;"
            "}"
        );
        m_titleMain->setText("航迹生成 · 飞行演示系统");
        m_titleMain->setMinimumHeight(24);
    }

    // 3. 设置子标题样式
    if (m_titleSub) {
        m_titleSub->setStyleSheet(
            "QLabel {"
            "   color: rgba(122, 168, 204, 0.7);"
            "   font-family: 'Consolas', 'Courier New', monospace;"
            "   font-size: 10px;"
            "   background: transparent;"
            "   padding-left: 12px;"
            "   letter-spacing: 0.5px;"
            "}"
        );
        m_titleSub->setText("TRAJECTORY  GENERATION  &  FLIGHT  DEMONSTRATION  SYSTEM · RUCKIG  ENGINE");
        m_titleSub->setMinimumHeight(18);
    }

    // 4. 设置状态点样式
    if (m_statusDot) {
        m_statusDot->setFixedSize(6, 6);
        m_statusDot->setStyleSheet(
            "background-color: #00ff9f;"
            "border-radius: 3px;"
        );
    }

    // 5. 设置时钟样式
    if (m_clockLabel) {
        m_clockLabel->setStyleSheet(
            "color: #7aa8cc;"
            "font-family: 'Consolas', 'Courier New', monospace;"
            "font-size: 15px;"
            "letter-spacing: 0.5px;"
        );
    }
}

void TitleBarWidget::updateClock() const{
    if (m_clockLabel) {
        m_clockLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    }
}
