// dialogs/AttitudeDialog.h
#ifndef ATTITUDEDIALOG_H
#define ATTITUDEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QDoubleValidator>
#include "../core/FlightParams.h"

class AttitudeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttitudeDialog(QWidget *parent = nullptr);
    ~AttitudeDialog();

    // 获取姿态参数
    core::AttitudeParams getAttitudeParams() const;

    // 设置姿态参数
    void setAttitudeParams(const core::AttitudeParams& params);

    // 验证输入
    bool validateInputs() const;

private slots:
    void onTextChanged(const QString& text);
    void onBtnOkClicked();
    void onBtnCancelClicked();
    void onBtnResetClicked();

private:
    // UI控件
    QTabWidget* m_tabWidget;

    // 初始姿态标签页
    QWidget* m_tabInitial;
    QGroupBox* m_groupInitialAngles;
    QLineEdit* m_editInitialRoll;
    QLineEdit* m_editInitialPitch;
    QLineEdit* m_editInitialYaw;

    QGroupBox* m_groupInitialRates;
    QLineEdit* m_editInitialRollRate;
    QLineEdit* m_editInitialPitchRate;
    QLineEdit* m_editInitialYawRate;

    // 目标姿态标签页
    QWidget* m_tabTarget;
    QGroupBox* m_groupTargetAngles;
    QLineEdit* m_editTargetRoll;
    QLineEdit* m_editTargetPitch;
    QLineEdit* m_editTargetYaw;

    QGroupBox* m_groupTargetRates;
    QLineEdit* m_editTargetRollRate;
    QLineEdit* m_editTargetPitchRate;
    QLineEdit* m_editTargetYawRate;

    // 约束标签页
    QWidget* m_tabConstraints;
    QGroupBox* m_groupRateConstraints;
    QLineEdit* m_editMaxRollRate;
    QLineEdit* m_editMaxPitchRate;
    QLineEdit* m_editMaxYawRate;

    QGroupBox* m_groupAccelConstraints;
    QLineEdit* m_editMaxRollAccel;
    QLineEdit* m_editMaxPitchAccel;
    QLineEdit* m_editMaxYawAccel;

    QGroupBox* m_groupJerkConstraints;
    QLineEdit* m_editMaxRollJerk;
    QLineEdit* m_editMaxPitchJerk;
    QLineEdit* m_editMaxYawJerk;

    // 按钮
    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;
    QPushButton* m_btnReset;

    // 布局
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;

    // 初始化函数
    void setupUI();
    void setupValidators();
    void setupConnections();
    void setupStyles();
    void setDefaultValues();

    // 工具函数
    void markErrorField(QLineEdit* edit) const;
    void clearErrorStates() const;
    QLineEdit* createNumberEdit(const QString& placeholder = "",
                               double min = -999999, double max = 999999,
                               int decimals = 3);
    QGroupBox* createGroupBox(const QString& title, QLayout* layout);
};

#endif // ATTITUDEDIALOG_H