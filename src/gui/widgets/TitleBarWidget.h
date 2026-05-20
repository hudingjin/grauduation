#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include <QWidget>

class QLabel;
class QTimer;

class TitleBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBarWidget(QLabel* logoLabel,
                           QLabel* statusDot,
                           QLabel* clockLabel,
                           QLabel* titleMain,
                           QLabel* titleSub,
                           QWidget *parent = nullptr);
    ~TitleBarWidget();

    private slots:
        void updateClock() const;

private:
    void setupStyles();

private:
    QLabel *m_logoLabel = nullptr;
    QLabel *m_statusDot = nullptr;
    QLabel *m_clockLabel = nullptr;
    QLabel *m_titleMain = nullptr;
    QLabel *m_titleSub = nullptr;
    QTimer *clockTimer = nullptr;
};

#endif // TITLEBARWIDGET_H