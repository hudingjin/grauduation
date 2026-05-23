// dialogs/AttitudeDialog.cpp
#include "AttitudeDialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QFrame>
#include <QStyle>

AttitudeDialog::AttitudeDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUI();
    setupValidators();
    setupConnections();
    setupStyles();
    setDefaultValues();

    setWindowTitle("姿参数设- 6-DOF模式");
    setFixedSize(850, 700);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
}

AttitudeDialog::~AttitudeDialog()
{
    // 所有QWidget对象都会自动删除
}

void AttitudeDialog::setupUI()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);

    // 创建标题
    QLabel* titleLabel = new QLabel("6-DOF 姿态参数设置", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #00d4ff; font-size: 16px; font-weight: bold; padding: 10px;");
    m_mainLayout->addWidget(titleLabel);

    // 创建标签
    m_tabWidget = new QTabWidget(this);

    // ========== 初始姿标签页 ==========
    m_tabInitial = new QWidget(this);
    QGridLayout* initialLayout = new QGridLayout(m_tabInitial);
    initialLayout->setSpacing(20);

    // 初始姿角度组
    m_groupInitialAngles = new QGroupBox("初始姿角 (°)", m_tabInitial);
    QGridLayout* anglesLayout = new QGridLayout(m_groupInitialAngles);

    m_editInitialRoll = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);
    m_editInitialPitch = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);
    m_editInitialYaw = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);

    anglesLayout->addWidget(new QLabel("Roll (滚转:"), 0, 0);
    anglesLayout->addWidget(m_editInitialRoll, 0, 1);
    anglesLayout->addWidget(new QLabel("Pitch (俯仰:"), 1, 0);
    anglesLayout->addWidget(m_editInitialPitch, 1, 1);
    anglesLayout->addWidget(new QLabel("Yaw (偏航:"), 2, 0);
    anglesLayout->addWidget(m_editInitialYaw, 2, 1);

    m_groupInitialAngles->setLayout(anglesLayout);

    // 初始角度
    m_groupInitialRates = new QGroupBox("初始角度 (°/s)", m_tabInitial);
    QGridLayout* ratesLayout = new QGridLayout(m_groupInitialRates);

    m_editInitialRollRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);
    m_editInitialPitchRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);
    m_editInitialYawRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);

    ratesLayout->addWidget(new QLabel("Roll Rate:"), 0, 0);
    ratesLayout->addWidget(m_editInitialRollRate, 0, 1);
    ratesLayout->addWidget(new QLabel("Pitch Rate:"), 1, 0);
    ratesLayout->addWidget(m_editInitialPitchRate, 1, 1);
    ratesLayout->addWidget(new QLabel("Yaw Rate:"), 2, 0);
    ratesLayout->addWidget(m_editInitialYawRate, 2, 1);

    m_groupInitialRates->setLayout(ratesLayout);

    initialLayout->addWidget(m_groupInitialAngles, 0, 0);
    initialLayout->addWidget(m_groupInitialRates, 0, 1);
    initialLayout->setColumnStretch(0, 1);
    initialLayout->setColumnStretch(1, 1);

    m_tabInitial->setLayout(initialLayout);

    // ========== 目标姿标签页 ==========
    m_tabTarget = new QWidget(this);
    QGridLayout* targetLayout = new QGridLayout(m_tabTarget);
    targetLayout->setSpacing(20);

    // 目标姿角度组
    m_groupTargetAngles = new QGroupBox("目标姿角 (°)", m_tabTarget);
    QGridLayout* targetAnglesLayout = new QGridLayout(m_groupTargetAngles);

    m_editTargetRoll = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);
    m_editTargetPitch = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);
    m_editTargetYaw = createNumberEdit("-180.0 ~ 180.0", -180.0, 180.0, 2);

    targetAnglesLayout->addWidget(new QLabel("Roll (滚转:"), 0, 0);
    targetAnglesLayout->addWidget(m_editTargetRoll, 0, 1);
    targetAnglesLayout->addWidget(new QLabel("Pitch (俯仰:"), 1, 0);
    targetAnglesLayout->addWidget(m_editTargetPitch, 1, 1);
    targetAnglesLayout->addWidget(new QLabel("Yaw (偏航:"), 2, 0);
    targetAnglesLayout->addWidget(m_editTargetYaw, 2, 1);

    m_groupTargetAngles->setLayout(targetAnglesLayout);

    // 目标角度
    m_groupTargetRates = new QGroupBox("目标角度 (°/s)", m_tabTarget);
    QGridLayout* targetRatesLayout = new QGridLayout(m_groupTargetRates);

    m_editTargetRollRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);
    m_editTargetPitchRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);
    m_editTargetYawRate = createNumberEdit("-360.0 ~ 360.0", -360.0, 360.0, 2);

    targetRatesLayout->addWidget(new QLabel("Roll Rate:"), 0, 0);
    targetRatesLayout->addWidget(m_editTargetRollRate, 0, 1);
    targetRatesLayout->addWidget(new QLabel("Pitch Rate:"), 1, 0);
    targetRatesLayout->addWidget(m_editTargetPitchRate, 1, 1);
    targetRatesLayout->addWidget(new QLabel("Yaw Rate:"), 2, 0);
    targetRatesLayout->addWidget(m_editTargetYawRate, 2, 1);

    m_groupTargetRates->setLayout(targetRatesLayout);

    targetLayout->addWidget(m_groupTargetAngles, 0, 0);
    targetLayout->addWidget(m_groupTargetRates, 0, 1);
    targetLayout->setColumnStretch(0, 1);
    targetLayout->setColumnStretch(1, 1);

    m_tabTarget->setLayout(targetLayout);

    // ========== 约束标签==========
    m_tabConstraints = new QWidget(this);
    QGridLayout* constraintsLayout = new QGridLayout(m_tabConstraints);
    constraintsLayout->setSpacing(20);

    // 角度约束
    m_groupRateConstraints = new QGroupBox("最大角速度约束 (°/s)", m_tabConstraints);
    QGridLayout* rateLayout = new QGridLayout(m_groupRateConstraints);

    m_editMaxRollRate = createNumberEdit("0.1 ~ 360.0", 0.1, 360.0, 2);
    m_editMaxPitchRate = createNumberEdit("0.1 ~ 360.0", 0.1, 360.0, 2);
    m_editMaxYawRate = createNumberEdit("0.1 ~ 360.0", 0.1, 360.0, 2);

    rateLayout->addWidget(new QLabel("Roll Rate:"), 0, 0);
    rateLayout->addWidget(m_editMaxRollRate, 0, 1);
    rateLayout->addWidget(new QLabel("Pitch Rate:"), 1, 0);
    rateLayout->addWidget(m_editMaxPitchRate, 1, 1);
    rateLayout->addWidget(new QLabel("Yaw Rate:"), 2, 0);
    rateLayout->addWidget(m_editMaxYawRate, 2, 1);

    m_groupRateConstraints->setLayout(rateLayout);

    // 角加速度约束
    m_groupAccelConstraints = new QGroupBox("最大角加速度约束 (°/s²)", m_tabConstraints);
    auto* accelLayout = new QGridLayout(m_groupAccelConstraints);

    m_editMaxRollAccel = createNumberEdit("0.1 ~ 720.0", 0.1, 720.0, 2);
    m_editMaxPitchAccel = createNumberEdit("0.1 ~ 720.0", 0.1, 720.0, 2);
    m_editMaxYawAccel = createNumberEdit("0.1 ~ 720.0", 0.1, 720.0, 2);

    accelLayout->addWidget(new QLabel("Roll Acceleration:"), 0, 0);
    accelLayout->addWidget(m_editMaxRollAccel, 0, 1);
    accelLayout->addWidget(new QLabel("Pitch Acceleration:"), 1, 0);
    accelLayout->addWidget(m_editMaxPitchAccel, 1, 1);
    accelLayout->addWidget(new QLabel("Yaw Acceleration:"), 2, 0);
    accelLayout->addWidget(m_editMaxYawAccel, 2, 1);

    m_groupAccelConstraints->setLayout(accelLayout);

    // 加加速度约束
    m_groupJerkConstraints = new QGroupBox("最大加加速度约束 (°/s³)", m_tabConstraints);
    QGridLayout* jerkLayout = new QGridLayout(m_groupJerkConstraints);

    m_editMaxRollJerk = createNumberEdit("0.1 ~ 2000.0", 0.1, 2000.0, 2);
    m_editMaxPitchJerk = createNumberEdit("0.1 ~ 2000.0", 0.1, 2000.0, 2);
    m_editMaxYawJerk = createNumberEdit("0.1 ~ 2000.0", 0.1, 2000.0, 2);

    jerkLayout->addWidget(new QLabel("Roll Jerk:"), 0, 0);
    jerkLayout->addWidget(m_editMaxRollJerk, 0, 1);
    jerkLayout->addWidget(new QLabel("Pitch Jerk:"), 1, 0);
    jerkLayout->addWidget(m_editMaxPitchJerk, 1, 1);
    jerkLayout->addWidget(new QLabel("Yaw Jerk:"), 2, 0);
    jerkLayout->addWidget(m_editMaxYawJerk, 2, 1);

    m_groupJerkConstraints->setLayout(jerkLayout);

    constraintsLayout->addWidget(m_groupRateConstraints, 0, 0);
    constraintsLayout->addWidget(m_groupAccelConstraints, 0, 1);
    constraintsLayout->addWidget(m_groupJerkConstraints, 0, 2);
    constraintsLayout->setColumnStretch(0, 1);
    constraintsLayout->setColumnStretch(1, 1);
    constraintsLayout->setColumnStretch(2, 1);

    m_tabConstraints->setLayout(constraintsLayout);

    // 添加标签
    m_tabWidget->addTab(m_tabInitial, "初始姿态");
    m_tabWidget->addTab(m_tabTarget, "目标姿态");
    m_tabWidget->addTab(m_tabConstraints, "运动约束");

    m_mainLayout->addWidget(m_tabWidget);

    // 添加分隔
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #0d3a5c; min-height: 2px; max-height: 2px; margin: 10px 0;");
    m_mainLayout->addWidget(line);

    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(20);

    m_btnReset = new QPushButton("重置", this);
    m_btnReset->setObjectName("btnReset");
    m_btnReset->setFixedSize(100, 30);

    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_btnReset);

    m_btnCancel = new QPushButton("取消", this);
    m_btnCancel->setFixedSize(100, 30);
    m_buttonLayout->addWidget(m_btnCancel);

    m_btnOk = new QPushButton("确定", this);
    m_btnOk->setFixedSize(100, 30);
    m_buttonLayout->addWidget(m_btnOk);

    m_mainLayout->addLayout(m_buttonLayout);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);

    setLayout(m_mainLayout);
}

void AttitudeDialog::setupValidators()
{
    // 使用QLineEdit的验证器，已经在createNumberEdit中设置了范围
    // 这里可以为某些特殊的输入添加额外验证
}

void AttitudeDialog::setupConnections()
{
    // 连接所有输入框的文本变化信号
    QList<QLineEdit*> allEdits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : allEdits) {
        connect(edit, &QLineEdit::textChanged, this, &AttitudeDialog::onTextChanged);
    }

    // 连接按钮信号
    connect(m_btnOk, &QPushButton::clicked, this, &AttitudeDialog::onBtnOkClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &AttitudeDialog::onBtnCancelClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &AttitudeDialog::onBtnResetClicked);
}

void AttitudeDialog::setupStyles()
{
    // 设置窗口样式，与主界面保持一
    setStyleSheet(R"(
        QDialog {
            background-color: #041f3c;
            color: #7aa8cc;
            font-family: 'Segoe UI', Arial, sans-serif;
        }

        QTabWidget::pane {
            border: 1px solid #0d3a5c;
            background-color: #0a2b4a;
        }

        QTabBar::tab {
            background-color: #0d3a5c;
            color: #7aa8cc;
            padding: 8px 20px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }

        QTabBar::tab:selected {
            background-color: #1a5b8c;
            color: #ffffff;
            border-bottom: 2px solid #00d4ff;
        }

        QTabBar::tab:hover:!selected {
            background-color: #2a7bcc;
        }

        QGroupBox {
            font-weight: bold;
            border: 2px solid #0d3a5c;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
            background-color: #0a2b4a;
            color: #ffffff;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #00d4ff;
        }

        QLabel {
            color: #7aa8cc;
            font-size: 11px;
            font-weight: normal;
        }

        QLineEdit {
            background-color: #061424;
            border: 1px solid #0d3a5c;
            border-radius: 3px;
            padding: 4px 6px;
            color: #ffffff;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 11px;
            selection-background-color: #1a5b8c;
        }

        QLineEdit[error="true"] {
            border: 1px solid #ff3366;
            background-color: rgba(255, 51, 102, 0.1);
        }

        QLineEdit:focus {
            border: 1px solid #00d4ff;
            background-color: #0a1a2f;
        }

        QPushButton {
            background-color: #1a5b8c;
            color: #ffffff;
            border: 1px solid #0d3a5c;
            border-radius: 3px;
            padding: 6px 12px;
            font-weight: bold;
            font-size: 12px;
        }

        QPushButton:hover {
            background-color: #2a8bcc;
            border-color: #00d4ff;
        }

        QPushButton:pressed {
            background-color: #0d3a5c;
            padding: 7px 12px 5px 12px;
        }

        QPushButton#btnReset {
            background-color: #5c5c5c;
            color: #cccccc;
        }

        QPushButton#btnReset:hover {
            background-color: #7a7a7a;
            border-color: #aaaaaa;
        }
    )");
}

void AttitudeDialog::setDefaultValues()
{
    // 初始姿
    m_editInitialRoll->setText("0.0");
    m_editInitialPitch->setText("0.0");
    m_editInitialYaw->setText("0.0");

    m_editInitialRollRate->setText("0.0");
    m_editInitialPitchRate->setText("0.0");
    m_editInitialYawRate->setText("0.0");

    // 目标姿
    m_editTargetRoll->setText("10.0");
    m_editTargetPitch->setText("5.0");
    m_editTargetYaw->setText("30.0");

    m_editTargetRollRate->setText("0.0");
    m_editTargetPitchRate->setText("0.0");
    m_editTargetYawRate->setText("0.0");

    // 运动学约
    m_editMaxRollRate->setText("30.0");
    m_editMaxPitchRate->setText("30.0");
    m_editMaxYawRate->setText("15.0");

    m_editMaxRollAccel->setText("60.0");
    m_editMaxPitchAccel->setText("60.0");
    m_editMaxYawAccel->setText("30.0");

    m_editMaxRollJerk->setText("200.0");
    m_editMaxPitchJerk->setText("200.0");
    m_editMaxYawJerk->setText("100.0");
}

void AttitudeDialog::onTextChanged(const QString& text)
{
    Q_UNUSED(text);

    QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
    if (edit) {
        // 清除错误状态
        if (edit->property("error") == "true") {
            edit->setProperty("error", "false");
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
        }
    }
}

void AttitudeDialog::markErrorField(QLineEdit* edit) const
{
    if (edit) {
        edit->setProperty("error", "true");
        edit->style()->unpolish(edit);
        edit->style()->polish(edit);
    }
}

void AttitudeDialog::clearErrorStates() const
{
    QList<QLineEdit*> allEdits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : allEdits) {
        if (edit) {
            edit->setProperty("error", "false");
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
        }
    }
}

QLineEdit* AttitudeDialog::createNumberEdit(const QString& placeholder, double min, double max, int decimals)
{
    QLineEdit* edit = new QLineEdit(this);
    edit->setPlaceholderText(placeholder);
    edit->setValidator(new QDoubleValidator(min, max, decimals, edit));
    edit->setAlignment(Qt::AlignRight);
    edit->setFixedHeight(25);
    return edit;
}

bool AttitudeDialog::validateInputs() const
{
    clearErrorStates();
    bool valid = true;
    QLineEdit* firstEmpty = nullptr;

    // 检查所有必填字段
    QList<QLineEdit*> allEdits = findChildren<QLineEdit*>();

    for (QLineEdit* edit : allEdits) {
        if (edit->text().trimmed().isEmpty()) {
            const_cast<AttitudeDialog*>(this)->markErrorField(edit);
            if (!firstEmpty) firstEmpty = edit;
            valid = false;
        } else {
            // 验证数范
            QString text = edit->text().trimmed();
            bool conversionOk = false;
            double value = text.toDouble(&conversionOk);

            if (!conversionOk) {
                const_cast<AttitudeDialog*>(this)->markErrorField(edit);
                if (!firstEmpty) firstEmpty = edit;
                valid = false;
                continue;
            }

            // 获取验证
            const QDoubleValidator* validator = qobject_cast<const QDoubleValidator*>(edit->validator());
            if (validator) {
                int pos = 0;
                QString tempText = edit->text();
                if (validator->validate(tempText, pos) != QValidator::Acceptable) {
                    const_cast<AttitudeDialog*>(this)->markErrorField(edit);
                    if (!firstEmpty) firstEmpty = edit;
                    valid = false;
                }
            }
        }
    }

    if (!valid) {
        QMessageBox::warning(const_cast<AttitudeDialog*>(this), "输入错误",
            "Please fill in all attitude parameters and keep values in range.");
        if (firstEmpty) {
            firstEmpty->setFocus();
            firstEmpty->selectAll();
        }
    }

    return valid;
}

void AttitudeDialog::onBtnOkClicked()
{
    if (validateInputs()) {
        accept();  // 关闭对话框并返回QDialog::Accepted
    }
}

void AttitudeDialog::onBtnCancelClicked()
{
    reject();  // 关闭对话框并返回QDialog::Rejected
}

void AttitudeDialog::onBtnResetClicked()
{
    clearErrorStates();
    setDefaultValues();
}

core::AttitudeParams AttitudeDialog::getAttitudeParams() const
{
    core::AttitudeParams params;

    // 读取起始姿
    params.roll0 = m_editInitialRoll->text().toDouble();
    params.pitch0 = m_editInitialPitch->text().toDouble();
    params.yaw0 = m_editInitialYaw->text().toDouble();
    params.rollRate0 = m_editInitialRollRate->text().toDouble();
    params.pitchRate0 = m_editInitialPitchRate->text().toDouble();
    params.yawRate0 = m_editInitialYawRate->text().toDouble();

    // 读取目标姿
    params.rollf = m_editTargetRoll->text().toDouble();
    params.pitchf = m_editTargetPitch->text().toDouble();
    params.yawf = m_editTargetYaw->text().toDouble();
    params.rollRatef = m_editTargetRollRate->text().toDouble();
    params.pitchRatef = m_editTargetPitchRate->text().toDouble();
    params.yawRatef = m_editTargetYawRate->text().toDouble();

    // 读取运动学约
    params.maxRollVel = m_editMaxRollRate->text().toDouble();
    params.maxPitchVel = m_editMaxPitchRate->text().toDouble();
    params.maxYawVel = m_editMaxYawRate->text().toDouble();

    params.maxRollAcc = m_editMaxRollAccel->text().toDouble();
    params.maxPitchAcc = m_editMaxPitchAccel->text().toDouble();
    params.maxYawAcc = m_editMaxYawAccel->text().toDouble();

    params.maxRollJerk = m_editMaxRollJerk->text().toDouble();
    params.maxPitchJerk = m_editMaxPitchJerk->text().toDouble();
    params.maxYawJerk = m_editMaxYawJerk->text().toDouble();

    return params;
}

void AttitudeDialog::setAttitudeParams(const core::AttitudeParams& params)
{
    // 设置起始姿
    m_editInitialRoll->setText(QString::number(params.roll0, 'f', 2));
    m_editInitialPitch->setText(QString::number(params.pitch0, 'f', 2));
    m_editInitialYaw->setText(QString::number(params.yaw0, 'f', 2));
    m_editInitialRollRate->setText(QString::number(params.rollRate0, 'f', 2));
    m_editInitialPitchRate->setText(QString::number(params.pitchRate0, 'f', 2));
    m_editInitialYawRate->setText(QString::number(params.yawRate0, 'f', 2));

    // 设置目标姿
    m_editTargetRoll->setText(QString::number(params.rollf, 'f', 2));
    m_editTargetPitch->setText(QString::number(params.pitchf, 'f', 2));
    m_editTargetYaw->setText(QString::number(params.yawf, 'f', 2));
    m_editTargetRollRate->setText(QString::number(params.rollRatef, 'f', 2));
    m_editTargetPitchRate->setText(QString::number(params.pitchRatef, 'f', 2));
    m_editTargetYawRate->setText(QString::number(params.yawRatef, 'f', 2));

    // 设置运动学约
    m_editMaxRollRate->setText(QString::number(params.maxRollVel, 'f', 2));
    m_editMaxPitchRate->setText(QString::number(params.maxPitchVel, 'f', 2));
    m_editMaxYawRate->setText(QString::number(params.maxYawVel, 'f', 2));

    m_editMaxRollAccel->setText(QString::number(params.maxRollAcc, 'f', 2));
    m_editMaxPitchAccel->setText(QString::number(params.maxPitchAcc, 'f', 2));
    m_editMaxYawAccel->setText(QString::number(params.maxYawAcc, 'f', 2));

    m_editMaxRollJerk->setText(QString::number(params.maxRollJerk, 'f', 2));
    m_editMaxPitchJerk->setText(QString::number(params.maxPitchJerk, 'f', 2));
    m_editMaxYawJerk->setText(QString::number(params.maxYawJerk, 'f', 2));
}
