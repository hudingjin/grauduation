#include "mainwindow.h"
#include "cmake-build-debug/aircraftSystem_autogen/include/ui_mainwindow.h"
#include "qchartview.h"
#include "widgets/TitleBarWidget.h"
#include "widgets/ChartWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QDoubleValidator>
#include <QButtonGroup>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QStyle>
#include <QApplication>
#include <QSplineSeries>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>
#include <cmath>
#include <QTableWidget>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QHeaderView>
#include "StatusWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dofGroup(nullptr)
    , m_simManager(nullptr)
    , m_statusLabel(nullptr)
    , m_positionChart(nullptr)
    , m_velocityChart(nullptr)
    , m_attitudeChart(nullptr)
    , m_accelerationChart(nullptr)
    , m_dataTable(nullptr)
    , m_statusWidget(nullptr)
{
    ui->setupUi(this);
    // 查找Qt Designer中已有的组件
    auto* logoLabel = findChild<QLabel*>("logoLabel");
    auto* statusDot = findChild<QLabel*>("statusDot");
    auto* clockLabel = findChild<QLabel*>("clockLabel");
    auto* titleMain = findChild<QLabel*>("titleMain");
    auto* titleSub = findChild<QLabel*>("titleSub");
    if (logoLabel && statusDot && clockLabel && titleMain && titleSub) {
        // 创建TitleBarWidget，传入所有必需的组件
        m_titleBarWidget = new TitleBarWidget(
            logoLabel, statusDot, clockLabel, titleMain, titleSub, this);

        // 添加到titleTextContainer
        if (ui->titleTextContainer) {
            QLayout* layout = ui->titleTextContainer->layout();
            if (!layout) {
                layout = new QVBoxLayout(ui->titleTextContainer);
                layout->setContentsMargins(0, 0, 0, 0);
            }
            layout->addWidget(m_titleBarWidget);
        }

        qDebug() << "TitleBarWidget创建成功";
    } else {
        qCritical() << "未找到所有必需的TitleBar组件，请检查Qt Designer中的objectName";

        // 调试：打印所有可用的label
        QList<QLabel*> allLabels = findChildren<QLabel*>();
        qDebug() << "当前可用的所有QLabel对象:";
        for (QLabel* label : allLabels) {
            qDebug() << "  " << label->objectName() << "(文本:" << label->text() << ")";
        }
    }
    // 创建状态标
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setFixedHeight(20);

    // 添加到状态态栏
    QStatusBar* statusBar = this->statusBar();
    if (statusBar) {
        statusBar->addPermanentWidget(m_statusLabel);
    }

    // 初始化控制面
    initControlPanel();
    setupTableControls();

    // 初始化仿真管理器
    m_simManager = new core::SimulationManager(this);

    // 初始化各模块
    initValidators();
    initButtonGroups();
    setupDOFConnections();
    initConnections();
    initCharts();  // 在initCharts中初始化数据表格
    //在initCharts之后调用initStatusWidget
    initStatusWidget();
    // 初始化UI状态
    updateUIForState(core::SimulationState::Idle);
    updateStatusMessage("系统就绪，请配置参数并生成轨迹", false);

    // 初始化进度条
    ui->progressSimulation->setRange(0, 1000);
    ui->progressSimulation->setValue(0);
    ui->progressSimulation->setFormat("%p%");
    qApp->setStyleSheet(qApp->styleSheet());

    // 检查Ruckig状态
    if (m_simManager && m_simManager->getCurrentTrajectory().dof > 0) {
        // 如果有可用的轨迹生成
        updateStatusMessage("Ruckig轨迹生成器就绪", false);
    } else {
        updateStatusMessage("使用模拟轨迹生成算法", true);
    }
}

MainWindow::~MainWindow()
{
    // 清理资源
    if (m_simManager) {
        delete m_simManager;
        m_simManager = nullptr;
    }
    delete ui;
}

// 初始化状态态显示区
// void MainWindow::initStatusDisplay()
// {
//     // 动创建控
//     m_progressBar = new QProgressBar(this);
//     m_progressBar->setRange(0, 1000);
//     m_progressBar->setValue(0);

//     m_statusLabel = new QLabel("就绪", this);

//     // 添加到状态态栏
//     QStatusBar* statusBar = this->statusBar();
//     if (statusBar) {
//         statusBar->addPermanentWidget(m_progressBar);
//         statusBar->addPermanentWidget(m_statusLabel);
//     }
// }

// 初始化控制面
void MainWindow::initControlPanel()
{
    // 设置按钮的初始启用状态
    ui->btnGenerate->setEnabled(true);    // 生成按钮始终可用
    ui->btnPlay->setEnabled(false);       // 播放按钮初始禁用
    ui->btnStop->setEnabled(false);       // 停止按钮初始禁用
    ui->btnReset->setEnabled(false);      // 重置按钮初始禁用
    ui->btnExport->setEnabled(false);     // 导出按钮初始禁用

    // 初始化进度条
    ui->progressSimulation->setValue(0);
    ui->progressSimulation->setFormat("0%");

    // 设置工具提示
    ui->btnGenerate->setToolTip("生成飞行轨迹");
    ui->btnPlay->setToolTip("开继续仿真");
    ui->btnStop->setToolTip("暂停/停止仿真");
    ui->btnReset->setToolTip("Reset system state");
    ui->btnExport->setToolTip("导出轨迹数据");
}



// 初始化连
void MainWindow::initConnections()
{
    // 连接SimulationManager信号
    if (m_simManager) {
        connect(m_simManager, &core::SimulationManager::trajectoryGenerated,
                this, &MainWindow::onTrajectoryGenerated);
        connect(m_simManager, &core::SimulationManager::trajectoryGenerationStarted,
                this, &MainWindow::onTrajectoryGenerationStarted);
        connect(m_simManager, &core::SimulationManager::trajectoryGenerationFinished,
                this, &MainWindow::onTrajectoryGenerationFinished);
        connect(m_simManager, &core::SimulationManager::stateChanged,
                this, &MainWindow::onSimulationStateChanged);
        connect(m_simManager, &core::SimulationManager::simulationTimeUpdated,
                this, &MainWindow::onSimulationTimeUpdated);
        connect(m_simManager, &core::SimulationManager::realtimeStateUpdated,
                this, &MainWindow::onRealtimeStateUpdated);
        connect(m_simManager, &core::SimulationManager::progressUpdated,
                this, &MainWindow::onProgressUpdated);
        connect(m_simManager, &core::SimulationManager::errorOccurred,
                this, &MainWindow::onErrorOccurred);
    }

    // 遍历所有 QLineEdit，将它们的 textChanged 信号连接到同一个槽函数
    QList<QLineEdit*> lineEdits = this->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        if (edit->isReadOnly() || !edit->isEnabled()) continue;
        connect(edit, &QLineEdit::textChanged, this, &MainWindow::onInputTextChanged);
    }
}

// 根据仿真状态更新UI
void MainWindow::updateUIForState(core::SimulationState state)
{
    switch (state) {
    case core::SimulationState::Idle:
        ui->btnGenerate->setEnabled(true);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(false);
        ui->btnExport->setEnabled(false);
        updateStatusMessage("空闲状态，可以生成轨迹迹", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Generating:
        ui->btnGenerate->setEnabled(false);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(false);
        ui->btnExport->setEnabled(false);
        updateStatusMessage("正在生成轨迹迹...", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Ready:
        ui->btnGenerate->setEnabled(false);
        ui->btnPlay->setEnabled(true);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(true);
        ui->btnExport->setEnabled(true);
        updateStatusMessage("轨迹就绪，可以开始仿真", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Running:
        ui->btnGenerate->setEnabled(false);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(true);
        ui->btnReset->setEnabled(false);
        ui->btnExport->setEnabled(false);
        updateStatusMessage("仿真运行..", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Paused:
        ui->btnGenerate->setEnabled(false);
        ui->btnPlay->setEnabled(true);
        ui->btnStop->setEnabled(true);
        ui->btnReset->setEnabled(false);
        ui->btnExport->setEnabled(true);
        updateStatusMessage("仿真已暂停", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Stopped:
        ui->btnGenerate->setEnabled(false);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(true);
        ui->btnExport->setEnabled(true);
        updateStatusMessage("仿真已停止", false);
        updatePlayButtonState(state);
        break;

    case core::SimulationState::Error:
        ui->btnGenerate->setEnabled(true);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(true);
        ui->btnExport->setEnabled(false);
        updateStatusMessage("系统错误，请检查参数", true);
        updatePlayButtonState(state);
        break;
    }
}

// 更新播放按钮状态
void MainWindow::updatePlayButtonState(core::SimulationState state) const{
    // 根据状态更新播放按钮的文本
    switch (state) {
    case core::SimulationState::Ready:
        ui->btnPlay->setText("开始");
        ui->btnStop->setText("停止");
        break;

    case core::SimulationState::Running:
        ui->btnPlay->setText("继续");
        ui->btnStop->setText("暂停");
        break;

    case core::SimulationState::Paused:
        ui->btnPlay->setText("继续");
        ui->btnStop->setText("停止");
        break;

    default:
        ui->btnPlay->setText("开始");
        ui->btnStop->setText("停止");
        break;
    }
}

// 更新状态消
void MainWindow::updateStatusMessage(const QString& message, bool isError) const{
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        QString style = isError ?
            "QLabel { color: #ff3366; padding: 2px 8px; font-weight: bold; }"
            : "QLabel { color: #7aa8cc; padding: 2px 8px; }";
        m_statusLabel->setStyleSheet(style);
    }

    qDebug() << "状态更" << message;
}



// 初始化辑
void MainWindow::initValidators()
{
    // 设置浮点数验证器 (范围 -9999 9999，小数点
    auto *doubleValidator = new QDoubleValidator(-10000.0, 10000.0, 4, this);
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    // 设置整数验证(用于 DOF ID 输入)
    auto *intValidator = new QIntValidator(0, 10000, this);
    // --- 应用到运动学约束输入---
    ui->VXlineEdit->setValidator(doubleValidator);
    ui->VYlineEdit->setValidator(doubleValidator);
    ui->VZlineEdit->setValidator(doubleValidator);

    ui->accXlineEdit->setValidator(doubleValidator);
    ui->accYlineEdit->setValidator(doubleValidator);
    ui->accZlineEdit->setValidator(doubleValidator);

    ui->JXlineEdit->setValidator(doubleValidator);
    ui->JYlineEdit->setValidator(doubleValidator);
    ui->JZlineEdit->setValidator(doubleValidator);
    // --- 应用到起目标点输入框 ---
    ui->editP0X->setValidator(doubleValidator);
    ui->editP0Y->setValidator(doubleValidator);
    ui->editP0Z->setValidator(doubleValidator);
    ui->editV0X->setValidator(doubleValidator);
    ui->editV0Y->setValidator(doubleValidator);
    ui->editV0Z->setValidator(doubleValidator);
    ui->editPfX->setValidator(doubleValidator);
    ui->editPfY->setValidator(doubleValidator);
    ui->editPfZ->setValidator(doubleValidator);
    ui->editVfX->setValidator(doubleValidator);
    ui->editVfY->setValidator(doubleValidator);
    ui->editVfZ->setValidator(doubleValidator);
    // --- 应用到控制周---
    ui->cycleLineEdit->setValidator(doubleValidator);
    auto* cycleValidator = new QDoubleValidator(0.1, 1000.0, 3, this);
    cycleValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->cycleLineEdit->setValidator(cycleValidator);
    ui->cycleLineEdit->setPlaceholderText("控制周期(ms)");
    ui->cycleLineEdit->setToolTip("请输入控制周期，范围.1 - 1000 ms");

    // 设置默认
    if (ui->cycleLineEdit->text().isEmpty()) {
        ui->cycleLineEdit->setText("10.0");  // 默认10毫秒
    }
}

void MainWindow::initButtonGroups()
{
    // 创建DOF互斥逻辑
    m_dofGroup = new QButtonGroup(this);
    m_dofGroup->setExclusive(true); // 关键：设置互

    // 确保所有按钮都是可选的
    ui->btn1Dof->setCheckable(true);
    ui->btn3Dof->setCheckable(true);
    ui->btn6Dof->setCheckable(true);

    // 将按钮添加到按钮组，设置不同的ID
    m_dofGroup->addButton(ui->btn1Dof, 1);
    m_dofGroup->addButton(ui->btn3Dof, 3);
    m_dofGroup->addButton(ui->btn6Dof, 6);
    // 默认选中DOF 3

    ui->btn3Dof->setChecked(true);

    // 调试输出当前选中状态
    qDebug() << "DOF初始化完成，当前选中ID:" << m_dofGroup->checkedId();
}


// 2. 清除错误状态（恢复默认蓝框
void MainWindow::onInputTextChanged(const QString &text) const{
    auto *edit = qobject_cast<QLineEdit*>(sender());
    if (edit) {
        // 如果当前是错误状态态，用户输入了内容，则恢复正
        if (edit->property("error") == "true") {
            edit->setProperty("error", "false");
            // 强制刷新样式，恢复为默认的深蓝背
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
        }
    }
}

// 核心校验函数
// MainWindow.cpp - 修改validateInputs()函数
bool MainWindow::validateInputs()
{
    // 清除所有错误状态
    clearAllErrorStates();

    // int dof = m_dofGroup ? m_dofGroup->checkedId() : 3;  // 默认3-DOF
    // QList<QLineEdit*> allEdits = this->findChildren<QLineEdit*>();
    // for (QLineEdit* edit : allEdits) {
    //     if (edit->property("error") == "true") {
    //         edit->setProperty("error", "false");
    //         edit->style()->unpolish(edit);
    //         edit->style()->polish(edit);
    //     }
    // }
    // 添加详细的调试信
    qDebug() << "=== 开始输入验证===";
    qDebug() << "当前DOF模式(m_currentDOF):" << m_currentDOF;
    qDebug() << "DOF按钮组中ID:" << (m_dofGroup ? m_dofGroup->checkedId() : -1);
    switch (m_currentDOF) {
    case 1:
        qDebug() << "调用1-DOF验证";
        return validate1DOFInputs();
    case 3:
        qDebug() << "调用3-DOF验证";
        return validate3DOFInputs();
    case 6:
        qDebug() << "调用6-DOF验证";
        return validate6DOFInputs();  // 确保调用6-DOF验证
    default:
        QMessageBox::warning(this, "错误", "未知的DOF模式: " + QString::number(m_currentDOF));
        return false;
    }
}
// 安全读取数
double MainWindow::safeReadDouble(QLineEdit* edit)
{
    // 既然已经做了校验，这里直接转 double 即可，Trimmed() 去除空格
    if (edit->text().trimmed().isEmpty()) {
        qDebug() << "严重错误: 尝试读取空输入框:" << edit->objectName();
        return 0.0; // 返回 0 只是防止程序崩溃，但正常流程不该走到这里
    }
    return edit->text().trimmed().toDouble();
}

 void MainWindow::on_btnGenerate_clicked()
{
    qDebug() << "=== 开始生成轨迹===";
    qDebug() << "当前DOF模式:" << m_currentDOF;
    if (!validateInputs()) {
        qDebug() << "参数验证失败";
        return;
    }

    // 1. 检查仿真管理器
    if (!m_simManager) {
        qCritical() << "Error: SimulationManager is null";
        QMessageBox::critical(this, "System Error", "Simulation manager is not initialized.");
        return;
    }

    // 2. 检查当前状态
    core::SimulationState currentState = m_simManager->getState();
    qDebug() << "当前仿真状态" << static_cast<int>(currentState);
    // Do not allow a second generation while a trajectory exists or simulation is active.
    if (currentState == core::SimulationState::Generating ||
        currentState == core::SimulationState::Ready ||
        currentState == core::SimulationState::Running ||
        currentState == core::SimulationState::Paused) {
        qDebug() << "已有轨迹或仿真正在进行，请先复位后再生成";
        updateStatusMessage("请先复位，再生成新的轨迹", true);
        return;
    }

    // 4. 如果处于错误状态，先重
    if (currentState == core::SimulationState::Error) {
        qDebug() << "检测到错误状态，正在重置...";
        m_simManager->resetSimulation();
    }

    // 5. 禁用生成按钮，防止重复点
    ui->btnGenerate->setEnabled(false);
    updateStatusMessage("正在生成轨迹迹...", false);

    // 1. 定义临时变量
    core::RuckigConfig config;
    core::KinematicLimits limits;
    core::StatePoint start;
    core::StatePoint target;
    // Ruckig 顶层配置
    // 判断自由(根据按钮选中状态
    // 确保这三个按钮是 QPushButton，且设置checkable = true
    int dof = m_dofGroup->checkedId();
    config.dof = dof;
    if (dof == 1 || dof == 3 || dof == 6) {
        config.dof = dof;
    } else {
        // 如果获取失败，回退到按钮状态态检
        qWarning() << "Failed to read DOF from button group, falling back to button state";
        if (ui->btn1Dof->isChecked()) {
            config.dof = 1;
        } else if (ui->btn6Dof->isChecked()) {
            config.dof = 6;
        } else {
            config.dof = 3; // 默认3-DOF
        }
    }
    // 读取同步模式
    // currentText() 获取下拉框当前显示的文本
    QString modeText = ui->syncComboBox->currentText();
    if (modeText.contains("TIME")) config.syncMode = "TIME";
    else if (modeText.contains("PHASE")) config.syncMode = "PHASE";
    else config.syncMode = "NONE";
    // 读取控制周期
    double cycleTime = ui->cycleLineEdit->text().toDouble();

    // 添加输入验证证
    if (cycleTime <= 0) {
        QMessageBox::warning(this, "参数错误", "控制周期必须大于0");
        return;
    }
    if (cycleTime > 1000) {  // 如果大于1000，单位错
        QMessageBox::warning(this, "参数错误", "控制周期单位应为毫秒(ms)，请确认输入");
        return;
    }
    if (cycleTime < 0.1) {  // 如果小于0.1毫秒，可能太
        QMessageBox::warning(this, "参数警告", "控制周期过小，可能导致计算不稳定");
        ui->btnGenerate->setEnabled(true);
        return;
    }

    // 转换为秒（Ruckig需要秒为单位）
    config.deltaT = cycleTime / 1000.0;  // 毫秒转秒

    qDebug() << "控制周期: 输入" << cycleTime << "ms -> 转换" << config.deltaT << "s";
    // ------------------------------------------------
    // 2. 读取运动学约(同之
    // ------------------------------------------------
    limits.velX = safeReadDouble(ui->VXlineEdit);
    limits.velY = safeReadDouble(ui->VYlineEdit);
    limits.velZ = safeReadDouble(ui->VZlineEdit);

    limits.accX = safeReadDouble(ui->accXlineEdit);
    limits.accY = safeReadDouble(ui->accYlineEdit);
    limits.accZ = safeReadDouble(ui->accZlineEdit);
    limits.jerkX = safeReadDouble(ui->JXlineEdit);
    limits.jerkY = safeReadDouble(ui->JYlineEdit);
    limits.jerkZ = safeReadDouble(ui->JZlineEdit);
    // ------------------------------------------------
    // 3. 读取起始和目标状态(同之
    // ------------------------------------------------
    start.x = safeReadDouble(ui->editP0X);
    start.y = safeReadDouble(ui->editP0Y);
    start.z = safeReadDouble(ui->editP0Z);
    start.vx = safeReadDouble(ui->editV0X); // 假设你有起始速度输入
    start.vy = safeReadDouble(ui->editV0Y);
    start.vz = safeReadDouble(ui->editV0Z);
    target.x = safeReadDouble(ui->editPfX);
    target.y = safeReadDouble(ui->editPfY);
    target.z = safeReadDouble(ui->editPfZ);
    target.vx = safeReadDouble(ui->editVfX); // 假设你有目标速度输入
    target.vy = safeReadDouble(ui->editVfY);
    target.vz = safeReadDouble(ui->editVfZ);
    // ------------------------------------------------
    // 4. 更新核心数据对象
    // ------------------------------------------------
    m_flightParams.setConfig(config);
    m_flightParams.setLimits(limits);
    m_flightParams.setStartState(start);
    m_flightParams.setTargetState(target);
    m_flightParams.setIs6DOF(config.dof == 6);
    if (config.dof == 6) {
        m_flightParams.setAttitudeParams(m_attitudeParams);
    }
    // ------------------------------------------------
    // 7. 验证打印与后续操
    // ==========================================
    // 调用类内部的打印函数
    m_flightParams.printDebug();
    // 【修正这里不能直接访dof，因m_flightParams 是类对象，dof 是其私有成员结构体内的数
    // 必须通过 getConfig() 获取
    qDebug() << "UI数据采集完成，准备传入算法核.."
             << m_flightParams.getConfig().dof
             << "??VelX:" << m_flightParams.getLimits().velX;
    // ?? QtConcurrent ??????????? GUI ??
    if (m_simManager) {
        // ?????????????????
        core::FlightParams paramsCopy = m_flightParams;
        core::SimulationManager* mgr = m_simManager;
        auto future = QtConcurrent::run([mgr, paramsCopy]() {
            mgr->generateTrajectory(paramsCopy);
        });
        Q_UNUSED(future);
        ui->btnGenerate->setEnabled(true);
        return;
    }else {
        QMessageBox::critical(this, "System Error", "Simulation manager is not initialized.");
    }

}

void MainWindow::on_btnPlay_clicked()
{
    if (!m_simManager) {
        QMessageBox::critical(this, "System Error", "Simulation manager is not initialized.");
        return;
    }

    switch (core::SimulationState currentState = m_simManager->getState()) {
    case core::SimulationState::Ready:
        // 从就绪状态态开始仿
        m_simManager->startSimulation();
        m_simulationRunning = true;
        // 禁用所有输入控
        disableAllInputsDuringSimulation();
        break;

    case core::SimulationState::Paused:
        // 从暂停状态态继续仿
        m_simManager->resumeSimulation();
        m_simulationRunning = true;
        break;

    default:
        qWarning() << "当前状态无法开继续仿真，状态" << static_cast<int>(currentState);
        break;
    }
}


void MainWindow::on_btnStop_clicked()
{
    if (!m_simManager) {
        QMessageBox::critical(this, "System Error", "Simulation manager is not initialized.");
        return;
    }

    switch (core::SimulationState currentState = m_simManager->getState()) {
    case core::SimulationState::Running:
        // 运行状态时点击暂停
            m_simManager->pauseSimulation();
        ui->btnStop->setText("停止");
        break;

    case core::SimulationState::Paused:
        // 暂停状态时点击停止
            m_simManager->stopSimulation();
        break;

    default:
        // 其他状态直接停
            m_simManager->stopSimulation();
        break;
    }
}


void MainWindow::on_btnReset_clicked()
{
    if (m_simManager) {
        m_simManager->resetSimulation();
    }
    m_simulationRunning = false;
    enableAllInputsAfterReset();
    // 重置进度
    ui->progressSimulation->setValue(0);
    ui->progressSimulation->setFormat("0%");
    //重置StatusWidget
    if (m_statusWidget) {
        qDebug() << "正在重置StatusWidget...";
        m_statusWidget->clearStatus();
    } else {
        qWarning() << "StatusWidget is null, cannot reset";
    }
    // 清空图表和数
    if (m_positionChart) m_positionChart->clear();
    if (m_velocityChart) m_velocityChart->clear();
    if (m_attitudeChart) m_attitudeChart->clear();
    if (m_accelerationChart) m_accelerationChart->clear();
    if (m_dataTable) m_dataTable->clearTable();
    // 清空数据表格
    if (m_dataTable) m_dataTable->clearTable();
    clearAllErrorStates();
    updateStatusMessage("复位完成，请重新生成轨迹", false);
    ui->centralwidget->adjustSize();
}


void MainWindow::on_btnExport_clicked(){
    if (!m_dataTable || m_dataTable->getRowCount() == 0) {
        QMessageBox::warning(this, "导出错误", "没有可导出的轨迹数据");
        return;
    }

    // 弹出文件保存对话
    QString fileName = QFileDialog::getSaveFileName(this,
        "导出轨迹数据",
        QDir::homePath() + "/trajectory_data.csv",
        "CSV文件 (*.csv);;所有文件 (*.*)");

    if (fileName.isEmpty()) {
        return;  // 用户取消了保
    }

    // 导出数据
    bool success = false;
    if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        success = m_dataTable->exportToCSV(fileName);
    } else {
        // 如果不是CSV扩展名，自动添加
        if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName += ".csv";
        }
        success = m_dataTable->exportToCSV(fileName);
    }

    if (success) {
        QMessageBox::information(this, "导出成功",
            QString("轨迹数据已成功导出到:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "导出错误", "导出失败");
    }
}


// ================== SimulationManager信号==================

void MainWindow::onTrajectoryGenerated(const core::TrajectoryResult& result)
{
    // Keep Generate disabled after a successful trajectory; Reset enables it again.
    ui->btnGenerate->setEnabled(!result.success);

    if (result.success) {
        // 检查轨迹点
        if (result.points.size() < 10) {
            qWarning() << "轨迹点数过少:" << result.points.size()
                       << "，可能时间步长过大或轨迹时长过短";
            updateStatusMessage(
                QString("Trajectory generated, but only %1 points. Consider a smaller cycle time or a longer target distance.")
                .arg(result.points.size()), true);
        } else {
            updateStatusMessage(
                QString("轨迹生成完成，点 %1，时 %2s")
                .arg(result.points.size())
                .arg(result.duration, 0, 'f', 2), false);
        }

        // 调试输出轨迹信息
        qDebug() << "轨迹信息:";
        qDebug() << "  点数:" << result.points.size();
        qDebug() << "  时长:" << result.duration << "s";
        if (!result.points.isEmpty()) {
            qDebug() << "  起始 t=" << result.points.first().time
                     << ", pos=(" << result.points.first().x
                     << "," << result.points.first().y
                     << "," << result.points.first().z << ")";
            qDebug() << "  结束 t=" << result.points.last().time
                     << ", pos=(" << result.points.last().x
                     << "," << result.points.last().y
                     << "," << result.points.last().z << ")";
        }

        //更新StatusWidget
        if (m_statusWidget) {
            // 更新状态显
            m_statusWidget->updateStatus("轨迹生成成功");
            m_statusWidget->updateRuckigStatus("成功");

            // 更新总时
            if (!result.points.isEmpty()) {
                double totalTime = result.points.last().time;
                m_statusWidget->updateTime(0.0, totalTime);

                // 更新起始位置
                const auto& firstPoint = result.points.first();
                m_statusWidget->updatePosition(firstPoint.x, firstPoint.y, firstPoint.z);

                // 更新起始速度
                m_statusWidget->updateSpeed(firstPoint.vx, firstPoint.vy, firstPoint.vz);

                // 更新起始姿
                double roll = qRadiansToDegrees(atan2(firstPoint.vy, firstPoint.vx));
                double pitch = qRadiansToDegrees(atan2(firstPoint.vz,
                    sqrt(firstPoint.vx*firstPoint.vx + firstPoint.vy*firstPoint.vy)));
                double yaw = 0.0;  // 简化的航向角
                if (qAbs(firstPoint.x) > 1e-6 || qAbs(firstPoint.y) > 1e-6) {
                    yaw = qRadiansToDegrees(atan2(firstPoint.y, firstPoint.x + 1e-6));
                }
                if (result.dof == 6) {
                    roll = firstPoint.roll;
                    pitch = firstPoint.pitch;
                    yaw = firstPoint.yaw;
                }
                m_statusWidget->updateAttitude(roll, pitch, yaw);
            }

            // 更新仿真进度
            m_statusWidget->updateSimulationProgress(0);
        }

        // 更新图表
        updateChartsWithTrajectory(result);

        // 更新数据表格
        if (m_dataTable) {
            m_dataTable->setTrajectoryData(result);
        }

        disableAllInputsDuringSimulation();
        ui->btnReset->setEnabled(true);
        ui->btnPlay->setEnabled(true);
        ui->btnExport->setEnabled(true);

    } else {
        updateStatusMessage("轨迹生成失败: " + result.errorMessage, true);

        // 失败时也更新StatusWidget
        if (m_statusWidget) {
            m_statusWidget->updateStatus("轨迹生成失败", true);
            m_statusWidget->updateRuckigStatus("失败", true);
        }
    }
}
void MainWindow::onTrajectoryGenerationStarted()
{
    updateStatusMessage("开始生成轨迹..", false);
    ui->progressSimulation->setValue(0);
    ui->progressSimulation->setFormat("0%");
    ui->btnGenerate->setEnabled(false);
}

void MainWindow::onTrajectoryGenerationFinished(bool success)
{
    if (success) {
        updateStatusMessage("轨迹生成完成，可以开始仿真", false);
    } else {
        updateStatusMessage("轨迹生成失败", true);
        ui->btnGenerate->setEnabled(true);  // 失败时重新启用生成按
    }
}

void MainWindow::onSimulationStateChanged(core::SimulationState newState)
{
    updateUIForState(newState);
}

void MainWindow::onSimulationTimeUpdated(double time)
{
    // 这里可以更新时间显示
    Q_UNUSED(time);
}

void MainWindow::onRealtimeStateUpdated(const core::TrajectoryPoint& state)
{
    // 实时更新图表指示
    if (m_positionChart) m_positionChart->updateCurrentTime(state.time);
    if (m_velocityChart) m_velocityChart->updateCurrentTime(state.time);
    if (m_accelerationChart) m_accelerationChart->updateCurrentTime(state.time);
    if (m_attitudeChart) m_attitudeChart->updateCurrentTime(state.time);

    // 更新数据表格选中
    if (m_dataTable) m_dataTable->updateCurrentRow(state.time);

    // 更新状态显
    if (m_statusWidget) {
        // 更新时间
        double totalTime = 0.0;
        if (m_simManager) {
            totalTime = m_simManager->getTrajectoryDuration();
        }
        m_statusWidget->updateTime(state.time, totalTime);

        // 更新位置
        m_statusWidget->updatePosition(state.x, state.y, state.z);

        // 更新速度
        m_statusWidget->updateSpeed(state.vx, state.vy, state.vz);

        // 修复：使用与图表相同的Yaw计算逻辑
        double roll = qRadiansToDegrees(atan2(state.vy, state.vx));
        double pitch = qRadiansToDegrees(atan2(state.vz, sqrt(state.vx*state.vx + state.vy*state.vy)));

        // 修复：从位置计算Yaw（与updateChartsWithTrajectory中的逻辑保持一致）
        double yaw = 0.0;
        if (qAbs(state.x) > 1e-6 || qAbs(state.y) > 1e-6) {
            yaw = qRadiansToDegrees(atan2(state.y, state.x + 1e-6));
        }
        if (m_currentDOF == 6) {
            roll = state.roll;
            pitch = state.pitch;
            yaw = state.yaw;
        }

        m_statusWidget->updateAttitude(roll, pitch, yaw);
    }
}
void MainWindow::onProgressUpdated(double progress)
{
    int value = static_cast<int>(progress * 1000);
    ui->progressSimulation->setValue(value);
    ui->progressSimulation->setFormat(QString("%1%").arg(progress * 100, 0, 'f', 1));
}

void MainWindow::onErrorOccurred(const QString& error)
{
    updateStatusMessage("错误: " + error, true);
    QMessageBox::warning(this, "仿真错误", error);
}

void MainWindow::initCharts()
{
    // 1. 创建图表控件，连接到UI中的容器Widget
    m_positionChart = new ChartWidget(ui->positionChartArea, this);
    m_velocityChart = new ChartWidget(ui->velocityChartArea, this);
    m_attitudeChart = new ChartWidget(ui->attitudeChartArea, this);
    m_accelerationChart = new ChartWidget(ui->AttitudeChartLegend, this);  // 注意：这里是AttitudeChartLegend

    // 2. 设置坐标轴标
    m_positionChart->setAxisLabels("时间 (s)", "位置 (m)");
    m_velocityChart->setAxisLabels("时间 (s)", "速度 (m/s)");
    m_attitudeChart->setAxisLabels("时间 (s)", "角度 (°)");
    m_accelerationChart->setAxisLabels("时间 (s)", "加度 (m/s²)");

    // 3. 设置样式
    // 位置- X:#ff3366, Y:#55ddff, Z:#00ff9f
    m_positionChart->setSeriesColor(0, QColor("#ff3366"));
    m_positionChart->setSeriesColor(1, QColor("#55ddff"));
    m_positionChart->setSeriesColor(2, QColor("#00ff9f"));

    // 速度- X:#ff3366, Y:#00ff9f, Z:#0099ff
    m_velocityChart->setSeriesColor(0, QColor("#ff3366"));
    m_velocityChart->setSeriesColor(1, QColor("#00ff9f"));
    m_velocityChart->setSeriesColor(2, QColor("#0099ff"));

    // 姿图 - Roll:#ff6699, Pitch:#aa55ff, Yaw:#55ddff
    m_attitudeChart->setSeriesColor(0, QColor("#ff6699"));
    m_attitudeChart->setSeriesColor(1, QColor("#aa55ff"));
    m_attitudeChart->setSeriesColor(2, QColor("#55ddff"));

    // 加度- AccX:#ff3366, AccY:#00ff9f, Jerk:#ffb300
    m_accelerationChart->setSeriesColor(0, QColor("#ff3366"));
    m_accelerationChart->setSeriesColor(1, QColor("#00ff9f"));
    m_accelerationChart->setSeriesColor(2, QColor("#ffb300"));

    qDebug() << "Charts initialized";

    // 初始化数据表
    if (ui->tableTrajectory && !m_dataTable) {
        m_dataTable = new DataTableWidget(ui->tableTrajectory, this);
        qDebug() << "Data table initialized";
    } else if (!ui->tableTrajectory) {
        qCritical() << "错误：ui->tableTrajectory 未找到！";
    } else if (m_dataTable) {
        qDebug() << "Data table already initialized, skipping";
    }
}



void MainWindow::updateChartsWithTrajectory(const core::TrajectoryResult& result)
{
    int dof = m_dofGroup ? m_dofGroup->checkedId() : 3;
    qDebug() << "更新图表，DOF =" << dof << "，轨迹点=" << result.points.size();

    if (m_positionChart) {
        // 位置图数
        QVector<QPointF> posX, posY, posZ;
        for (const auto& point : result.points) {
            posX.append(QPointF(point.time, point.x));
            if (dof >= 3) {
                posY.append(QPointF(point.time, point.y));
                posZ.append(QPointF(point.time, point.z));
            }
        }
        m_positionChart->setDataForDOF(posX, posY, posZ, dof);
    }

    if (m_velocityChart) {
        // 速度图数
        QVector<QPointF> velX, velY, velZ;
        for (const auto& point : result.points) {
            velX.append(QPointF(point.time, point.vx));
            if (dof >= 3) {
                velY.append(QPointF(point.time, point.vy));
                velZ.append(QPointF(point.time, point.vz));
            }
        }
        m_velocityChart->setDataForDOF(velX, velY, velZ, dof);
    }

    if (m_accelerationChart) {
        // 加度图数
        QVector<QPointF> accX, accY, jerk;
        for (const auto& point : result.points) {
            accX.append(QPointF(point.time, point.ax));
            if (dof >= 3) {
                accY.append(QPointF(point.time, point.ay));
            }
            jerk.append(QPointF(point.time, 0)); // 加加速度占位
        }
        m_accelerationChart->setDataForDOF(accX, accY, jerk, dof);
    }

    if (m_attitudeChart && dof >= 3) {
        QVector<QPointF> roll, pitch, yaw;
        for (const auto& point : result.points) {
            double rollAngle = point.roll;
            double pitchAngle = point.pitch;
            double yawAngle = point.yaw;

            if (dof != 6) {
                // 3-DOF does not carry real attitude state, so keep the old derived display.
                rollAngle = atan2(point.vy, point.vx) * 180.0 / M_PI;
                pitchAngle = asin(point.vz /
                    sqrt(point.vx*point.vx + point.vy*point.vy + point.vz*point.vz + 1e-6)) * 180.0 / M_PI;
                yawAngle = atan2(point.y, point.x + 1e-6) * 180.0 / M_PI;
            }

            roll.append(QPointF(point.time, rollAngle));
            pitch.append(QPointF(point.time, pitchAngle));
            yaw.append(QPointF(point.time, yawAngle));
        }
        m_attitudeChart->setDataForDOF(roll, pitch, yaw, dof);
    }}

void MainWindow::setupTableControls()
{
    if (!ui->tableTrajectory) {
        qWarning() << "Table widget not found";
        return;
    }

    // 1. 查找velocityHeader_3和tableTrajectory的父容器
    QWidget* velocityHeader = ui->velocityHeader_3;  // AUTO REFRESH所在的widget
    if (!velocityHeader) {
        qWarning() << "velocityHeader_3 not found";
        return;
    }

    QWidget* tableTrajectory = ui->tableTrajectory;
    if (!tableTrajectory) {
        qWarning() << "tableTrajectory not found";
        return;
    }

    // 2. 找到它们的共同父容器
    QWidget* commonParent = velocityHeader->parentWidget();
    if (!commonParent) {
        qWarning() << "未找到共同父容器";
        return;
    }

    // 3. 获取容器的布局
    QLayout* parentLayout = commonParent->layout();
    if (!parentLayout) {
        qWarning() << "父容器无布局";
        return;
    }

    qDebug() << "开始设置表格控制按钮组（两按钮版）";

    // 4. 创建控制按钮组widget
    QWidget* controlWidget = new QWidget(commonParent);
    controlWidget->setObjectName("tableControlWidget");

    // 水平布局
    QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(5, 5, 5, 5);
    controlLayout->setSpacing(4);

    // 修改：只保留两个按钮
    QPushButton* btnSelect列设置 = new QPushButton("列设置", controlWidget);
    btnSelect列设置->setObjectName("btnSelect列设置");
    btnSelect列设置->setToolTip("选择要显示的列");
    btnSelect列设置->setFixedWidth(80);
    btnSelect列设置->setFixedHeight(24);

    connect(btnSelect列设置, &QPushButton::clicked, this, [this]() {
        if (m_dataTable) {
            m_dataTable->showColumnSelectorDialog();
        }
    });

    QPushButton* btnZoomTable = new QPushButton("放大", controlWidget);
    btnZoomTable->setObjectName("btnZoomTable");
    btnZoomTable->setToolTip("放大表格查看");
    btnZoomTable->setFixedWidth(80);
    btnZoomTable->setFixedHeight(24);

    connect(btnZoomTable, &QPushButton::clicked, this, &MainWindow::onZoomTableClicked);

    // 只添加两个控
    controlLayout->addStretch();  // 中间伸缩
    controlLayout->addWidget(btnSelect列设置);
    controlLayout->addStretch();  // 中间伸缩
    controlLayout->addWidget(btnZoomTable);
    controlLayout->addStretch();  // 右侧伸缩

    // 设置样式
    controlWidget->setStyleSheet(R"(
        QWidget#tableControlWidget {
            background-color: #040f1c;
            border: 1px solid rgba(0, 100, 160, 0.3);
            border-radius: 0px;
        }
        QPushButton#btnSelect列设置, QPushButton#btnZoomTable {
            background-color: #061424;
            color: rgb(0, 212, 255);
            font-family: "Share Tech Mono", monospace;
            font-size: 9px;
            border: 1px solid rgba(0, 100, 160, 0.3);
            border-radius: 0px;
            padding: 4px 6px;
        }
        QPushButton#btnSelect列设置:hover, QPushButton#btnZoomTable:hover {
            background-color: #0a1a2f;
            border-color: rgba(0, 212, 255, 0.5);
        }
        QPushButton#btnSelect列设置:pressed, QPushButton#btnZoomTable:pressed {
            background-color: rgba(0, 212, 255, 0.15);
            padding: 5px 6px 3px 6px;
        }
        QPushButton#btnSelect列设置:disabled, QPushButton#btnZoomTable:disabled {
            background-color: #1c3a5a;
            color: #5a7a9c;
        }
    )");

    // 设置字体
    QFont font;
    font.setFamily("Share Tech Mono");
    font.setPointSize(9);
    font.setWeight(QFont::Normal);

    btnSelect列设置->setFont(font);
    btnZoomTable->setFont(font);

    // 在布局中找到velocityHeader_3和tableTrajectory的位
    int velocityIndex = -1;
    int tableIndex = -1;

    for (int i = 0; i < parentLayout->count(); ++i) {
        QLayoutItem* item = parentLayout->itemAt(i);
        if (item && item->widget()) {
            if (item->widget() == velocityHeader) {
                velocityIndex = i;
                qDebug() << "找到velocityHeader_3，索" << velocityIndex;
            } else if (item->widget() == tableTrajectory) {
                tableIndex = i;
                qDebug() << "找到tableTrajectory，索" << tableIndex;
            }
        }
    }

    // 将控制按钮组插入到两者之
    if (velocityIndex >= 0 && tableIndex >= 0 && velocityIndex < tableIndex) {
        if (QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>(parentLayout)) {
            boxLayout->insertWidget(velocityIndex + 1, controlWidget);
        } else {
            parentLayout->addWidget(controlWidget);
        }
        qDebug() << "控制按钮组已插入到velocityHeader_3和tableTrajectory之间";
    } else {
        if (QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>(parentLayout)) {
            if (tableIndex > 0) {
                boxLayout->insertWidget(tableIndex, controlWidget);
            } else {
                boxLayout->insertWidget(0, controlWidget);
            }
        } else {
            parentLayout->addWidget(controlWidget);
        }
        qDebug() << "控制按钮组已插入到tableTrajectory之前";
    }

    // 调整尺寸
    controlWidget->adjustSize();
        qDebug() << "Table controls inserted";
}
QWidget* MainWindow::createWidgetFromLayout(QLayout* layout)
{
    QWidget* widget = new QWidget(this);
    widget->setLayout(layout);
    return widget;
}

void MainWindow::onZoomTableClicked()
{
    if (!m_dataTable || m_dataTable->getRowCount() == 0) {
        QMessageBox::information(this, "无数据", "当前没有可查看的轨迹表格数据");
        return;
    }

    showZoomedTableDialog();
}

void MainWindow::showZoomedTableDialog()
{
    // 创建对话
    QDialog* zoomDialog = new QDialog(this);
    zoomDialog->setObjectName("zoomedTableDialog");
    zoomDialog->setWindowTitle("放大表格 - 轨迹数据");
    zoomDialog->setMinimumSize(1200, 800);
    zoomDialog->setWindowFlags(zoomDialog->windowFlags() | Qt::WindowMinMaxButtonsHint);  // 添加最小化最大化按钮

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(zoomDialog);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // 1. 创建工具
    QToolBar* toolbar = new QToolBar(zoomDialog);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // 导出按钮
    QAction* actExport = toolbar->addAction("💾 导出数据");
    actExport->setToolTip("导出当前表格数据");
    connect(actExport, &QAction::triggered, zoomDialog, [this, zoomDialog]() {
        QString fileName = QFileDialog::getSaveFileName(
            zoomDialog, "导出数据",
            QDir::homePath() + "/trajectory_zoomed.csv",
            "CSV文件 (*.csv);;所有文件 (*.*)");

        if (!fileName.isEmpty()) {
            if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
                fileName += ".csv";
            }

            QTableWidget* table = zoomDialog->findChild<QTableWidget*>("zoomedTable");
            if (table) {
                exportTableToCSV(table, fileName);
            }
        }
    });

    // 打印按钮
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);

    // 修改：删除关闭按钮相关代
    // 不再添加关闭按钮，用户可以使用窗口的关闭按钮（右上角X

    mainLayout->addWidget(toolbar);

    // 2. 创建状态栏
    QStatusBar* statusBar = new QStatusBar(zoomDialog);
    QLabel* statusLabel = new QLabel("就绪", statusBar);
    statusBar->addPermanentWidget(statusLabel);
    mainLayout->addWidget(statusBar);

    // 3. 创建放大的表
    QTableWidget* zoomedTable = createZoomedTable();
    zoomedTable->setObjectName("zoomedTable");
    mainLayout->addWidget(zoomedTable);

    // 4. 设置对话框样
    zoomDialog->setStyleSheet(R"(
        QDialog#zoomedTableDialog {
            background-color: #041f3c;
        }
        QToolBar {
            background-color: #0a2b4a;
            border-bottom: 1px solid #0d3a5c;
            spacing: 5px;
        }
        QToolBar QToolButton {
            padding: 3px 8px;
            color: #ffffff;
            background-color: #1a5b8c;
            border: 1px solid #0d3a5c;
            border-radius: 3px;
        }
        QToolBar QToolButton:hover {
            background-color: #2a8bcc;
        }
        QStatusBar {
            background-color: #0a2b4a;
            color: #7aa8cc;
            border-top: 1px solid #0d3a5c;
        }
    )");

    // 5. 显示对话
    zoomDialog->setAttribute(Qt::WA_DeleteOnClose);
    zoomDialog->exec();
}

QTableWidget* MainWindow::createZoomedTable()
{
    if (!m_dataTable || !m_dataTable->getRowCount()) {
        return new QTableWidget();
    }

    // 获取原始表格
    QTableWidget* originalTable = ui->tableTrajectory;
    if (!originalTable) {
        return new QTableWidget();
    }

    // 创建放大表格
    QTableWidget* zoomedTable = new QTableWidget();

    // 复制表头
    int columnCount = originalTable->columnCount();
    int rowCount = originalTable->rowCount();

    zoomedTable->setColumnCount(columnCount);
    zoomedTable->setRowCount(rowCount);

    // 复制表头文本
    for (int col = 0; col < columnCount; ++col) {
        QTableWidgetItem* headerItem = originalTable->horizontalHeaderItem(col);
        if (headerItem) {
            zoomedTable->setHorizontalHeaderItem(col, headerItem->clone());
        }
    }

    // 复制数据
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem* item = originalTable->item(row, col);
            if (item) {
                zoomedTable->setItem(row, col, item->clone());
            }
        }
    }

    // 设置表格属
    zoomedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    zoomedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    zoomedTable->setSelectionMode(QAbstractItemView::SingleSelection);
    zoomedTable->setAlternatingRowColors(true);
    zoomedTable->setSortingEnabled(true);

    // 设置字体
    QFont tableFont("Consolas", 10);
    zoomedTable->setFont(tableFont);

    // 设置列宽
    for (int col = 0; col < columnCount; ++col) {
        zoomedTable->setColumnWidth(col, 120);
    }

    // 设置样式
    zoomedTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: #041f3c;"
        "   color: #7aa8cc;"
        "   gridline-color: #0d3a5c;"
        "   border: 1px solid #0d3a5c;"
        "   font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QTableWidget::item {"
        "   padding: 4px;"
        "}"
        "QTableWidget::item:selected {"
        "   background-color: #0d3a5c;"
        "   color: #ffffff;"
        "}"
        "QHeaderView::section {"
        "   background-color: #0a2b4a;"
        "   color: #7aa8cc;"
        "   padding: 6px;"
        "   border: 1px solid #0d3a5c;"
        "   font-weight: bold;"
        "   font-size: 11px;"
        "}"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: #0a2b4a;"
        "   width: 16px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #1a5b8c;"
        "   border-radius: 8px;"
        "}"
        "QScrollBar:horizontal {"
        "   border: none;"
        "   background: #0a2b4a;"
        "   height: 16px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "   background: #1a5b8c;"
        "   border-radius: 8px;"
        "}"
        );

    // 设置表头
    QFont headerFont("Segoe UI", 10, QFont::Bold);
    zoomedTable->horizontalHeader()->setFont(headerFont);
    zoomedTable->horizontalHeader()->setStretchLastSection(true);
    zoomedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // 设置垂直表头
    zoomedTable->verticalHeader()->setDefaultSectionSize(24);
    zoomedTable->verticalHeader()->setFont(tableFont);

    qDebug() << "Created zoomed table:" << columnCount << "columns," << rowCount << "rows";
    return zoomedTable;
}

void MainWindow::exportTableToCSV(QTableWidget* table, const QString& filename)
{
    if (!table || table->rowCount() == 0) {
        QMessageBox::warning(this, "导出错误", "没有可导出的表格数据");
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出错误", "无法创建文件: " + filename);
        return;
    }

    QTextStream stream(&file);

    // Qt6兼容
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    // 写入表头
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (headerItem) {
            stream << "\"" << headerItem->text() << "\"";
        }
        if (col < table->columnCount() - 1) {
            stream << ",";
        }
    }
    stream << "\n";

    // 写入数据
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                stream << "\"" << item->text() << "\"";
            }
            if (col < table->columnCount() - 1) {
                stream << ",";
            }
        }
        stream << "\n";
    }

    file.close();

    QMessageBox::information(this, "导出成功",
                             QString("数据已导出到：\n%1\n\n行数：%2")
                                 .arg(filename)
                                 .arg(table->rowCount()));
}
void MainWindow::initStatusWidget()
{
    qDebug() << "=== 开始初始化StatusWidget ===";

    // 获取UI中的标签组件
    m_labelRollValue = findChild<QLabel*>("labelRollValue");
    m_labelPitchValue = findChild<QLabel*>("labelPitchValue");
    m_labelYawValue = findChild<QLabel*>("labelYawValue");
    m_valueTimeCurrent = findChild<QLabel*>("valueTimeCurrent");
    m_valueTimeTotal = findChild<QLabel*>("valueTimeTotal");
    m_valuePosition = findChild<QLabel*>("valuePosition");
    m_valueSpeed = findChild<QLabel*>("valueSpeed");
    m_valueStatus = findChild<QLabel*>("valueStatus");
    m_valueRuckig = findChild<QLabel*>("valueRuckig");

    // 调试输出找到的组
    qDebug() << "找到labelRollValue:" << (m_labelRollValue != nullptr);
    qDebug() << "找到labelPitchValue:" << (m_labelPitchValue != nullptr);
    qDebug() << "找到labelYawValue:" << (m_labelYawValue != nullptr);
    qDebug() << "找到valueTimeCurrent:" << (m_valueTimeCurrent != nullptr);
    qDebug() << "找到valueTimeTotal:" << (m_valueTimeTotal != nullptr);
    qDebug() << "找到valuePosition:" << (m_valuePosition != nullptr);
    qDebug() << "找到valueSpeed:" << (m_valueSpeed != nullptr);
    qDebug() << "找到valueStatus:" << (m_valueStatus != nullptr);
    qDebug() << "找到valueRuckig:" << (m_valueRuckig != nullptr);

    // 检查组件是否找
    bool allFound = true;
    if (!m_labelRollValue) { qWarning() << "未找labelRollValue"; allFound = false; }
    if (!m_labelPitchValue) { qWarning() << "未找labelPitchValue"; allFound = false; }
    if (!m_labelYawValue) { qWarning() << "未找labelYawValue"; allFound = false; }
    if (!m_valueTimeCurrent) { qWarning() << "未找valueTimeCurrent"; allFound = false; }
    if (!m_valueTimeTotal) { qWarning() << "未找valueTimeTotal"; allFound = false; }
    if (!m_valuePosition) { qWarning() << "未找valuePosition"; allFound = false; }
    if (!m_valueSpeed) { qWarning() << "未找valueSpeed"; allFound = false; }
    if (!m_valueStatus) { qWarning() << "未找valueStatus"; allFound = false; }
    if (!m_valueRuckig) { qWarning() << "未找valueRuckig"; allFound = false; }

    if (allFound) {
        // 创建StatusWidget
        m_statusWidget = new StatusWidget(this);

        // 设置UI组件
        m_statusWidget->setupUI(
            m_labelRollValue,
            m_labelPitchValue,
            m_labelYawValue,
            m_valueTimeCurrent,
            m_valueTimeTotal,
            m_valuePosition,
            m_valueSpeed,
            m_valueStatus,
            m_valueRuckig,
            ui->progressSimulation
        );
        qDebug() << "StatusWidget initialized";
    } else {
        qCritical() << "状态显示组件初始化失败，部分组件未找到";

        // 调试：打印所有可用的label名称
        QList<QLabel*> allLabels = findChildren<QLabel*>();
        qDebug() << "当前可用的所有QLabel对象:";
        for (QLabel* label : allLabels) {
            qDebug() << "  " << label->objectName();
        }
    }

    qDebug() << "=== StatusWidget初始化结===";
}

// ================== DOF相关函数实现 ==================

// 1. 设置DOF连接
// setupDOFConnections() 函数中修改DOF按钮连接
void MainWindow::setupDOFConnections()
{
    if (!m_dofGroup) {
        qWarning() << "DOF button group is not initialized";
        return;
    }

    // 连接DOF按钮组信
    connect(m_dofGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton* button) {
                // 添加调试信息
                qDebug() << "=== DOF按钮点击信号开===";
                qDebug() << "被点击按" << button->text();

                // 检查仿真是否运行中
                if (m_simulationRunning) {
                    QMessageBox::warning(this, "Simulation Running",
                                         "仿真进行中，无法切换DOF模式。\n"
                                         "Please click Reset before switching DOF mode.");

                    // 恢复之前的中状态
                    if (m_dofGroup) {
                        QAbstractButton* prevBtn = m_dofGroup->button(m_currentDOF);
                        if (prevBtn) {
                            prevBtn->setChecked(true);
                        }
                    }
                    return;
                }

                int newDOF = m_dofGroup->id(button);
                qDebug() << "新DOF:" << newDOF << "当前DOF:" << m_currentDOF;

                // 如果和当前DOF相同，不处理
                if (newDOF == m_currentDOF) {
                    qDebug() << "DOF相同，不处理";
                    return;
                }

                // 保存旧DOF
                int oldDOF = m_currentDOF;

                // 特殊处理6-DOF模式
                if (newDOF == 6) {
                    qDebug() << "切换-DOF模式，弹出对话框";

                    // 先更新当前DOF
                    m_currentDOF = 6;

                    // 创建或获取对话框
                    if (!m_attitudeDialog) {
                        m_attitudeDialog = new AttitudeDialog(this);
                    }

                    // 显示对话
                    int result = m_attitudeDialog->exec();

                    if (result == QDialog::Accepted) {
                        // 用户点击了确
                        m_attitudeParams = m_attitudeDialog->getAttitudeParams();

                        // 确保m_currentDOF
                        m_currentDOF = 6;

                        // 更新UI
                        updateUIForDOF(6);
                        showDOFHelp(6);
                        updateStatusMessage("6-DOF模式已激活，姿参数已设置", false);

                        qDebug() << "6-DOF模式激活成功，当前DOF:" << m_currentDOF;
                        qDebug() << "姿参"
                                 << "roll0=" << m_attitudeParams.roll0
                                 << "pitch0=" << m_attitudeParams.pitch0
                                 << "yaw0=" << m_attitudeParams.yaw0;
                    } else {
                        // 用户取消，恢复之前的DOF
                        m_currentDOF = oldDOF;
                        if (m_dofGroup) {
                            QAbstractButton* prevBtn = m_dofGroup->button(oldDOF);
                            if (prevBtn) {
                                prevBtn->setChecked(true);
                            }
                        }
                        updateUIForDOF(oldDOF);
                        updateStatusMessage("姿参数设置已取消", false);
                        qDebug() << "6-DOF切换取消，恢复到DOF:" << oldDOF;
                    }
                } else {
                    // 1-DOF-DOF切换
                    m_currentDOF = newDOF;
                    updateUIForDOF(newDOF);
                    showDOFHelp(newDOF);
                    setDefaultValuesForDOF(newDOF);

                    qDebug() << "Switched to" << newDOF << "-DOF mode";
                }

                qDebug() << "=== DOF按钮点击信号结束 ===";
            });

    // 初始化显示当前DOF的UI
    int currentDOF = m_dofGroup->checkedId();
    if (currentDOF > 0) {
        m_currentDOF = currentDOF;
        updateUIForDOF(currentDOF);
    }
}

// 2. 根据DOF更新UI
void MainWindow::updateUIForDOF(int dof)
{
    qDebug() << "正在更新UI：" << dof << "-DOF mode";
    clearAllErrorStates();
    // 记录当前选中的DOF
    m_currentDOF = dof;

    // 1. 先获取所有输入框列表
    QVector<QLineEdit*> allAxisEdits = {
        // 位置
        ui->editP0X, ui->editP0Y, ui->editP0Z,
        ui->editPfX, ui->editPfY, ui->editPfZ,
        // 速度
        ui->editV0X, ui->editV0Y, ui->editV0Z,
        ui->editVfX, ui->editVfY, ui->editVfZ,
        // 运动学约
        ui->VXlineEdit, ui->VYlineEdit, ui->VZlineEdit,
        ui->accXlineEdit, ui->accYlineEdit, ui->accZlineEdit,
        ui->JXlineEdit, ui->JYlineEdit, ui->JZlineEdit
    };

    // 2. 清除所有输入框的错误状态
    for (QLineEdit* edit : allAxisEdits) {
        if (edit) {
            edit->setProperty("error", "false");
            // 强制样式刷新
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
        }
    }

    // 3. 根据DOF模式处理
    switch (dof) {
    case 1:  // 1-DOF
    {
        // 启用X轴输入框
        ui->labelP0X->setEnabled(true);
        ui->editP0X->setEnabled(true);
        ui->labelPfX->setEnabled(true);
        ui->editPfX->setEnabled(true);
        ui->labelV0X->setEnabled(true);
        ui->editV0X->setEnabled(true);
        ui->labelVfX->setEnabled(true);
        ui->editVfX->setEnabled(true);
        ui->VXlabel->setEnabled(true);
        ui->VXlineEdit->setEnabled(true);
        ui->AXlabel->setEnabled(true);
        ui->accXlineEdit->setEnabled(true);
        ui->JXlabel->setEnabled(true);
        ui->JXlineEdit->setEnabled(true);

        // 禁用Y/Z轴输入框
        QVector<QLineEdit*> disabledEditsY = {
            ui->editP0Y, ui->editP0Z,
            ui->editPfY, ui->editPfZ,
            ui->editV0Y, ui->editV0Z,
            ui->editVfY, ui->editVfZ,
            ui->VYlineEdit, ui->VZlineEdit,
            ui->accYlineEdit, ui->accZlineEdit,
            ui->JYlineEdit, ui->JZlineEdit
        };

        for (QLineEdit* edit : disabledEditsY) {
            if (edit) {
                // 1. 清除错误状态
                edit->setProperty("error", "false");
                edit->style()->unpolish(edit);
                edit->style()->polish(edit);

                // 2. 清空数
                edit->clear();

                // 3. 禁用控件
                edit->setEnabled(false);
            }
        }

        // 禁用对应的标
        QVector<QLabel*> disabledLabelsY = {
            ui->labelP0Y, ui->labelP0Z,
            ui->labelPfY, ui->labelPfZ,
            ui->labelV0Y, ui->labelV0Z,
            ui->labelVfY, ui->labelVfZ,
            ui->VYlabel, ui->VZlabel,
            ui->AYlabel, ui->AZlabel,
            ui->JYlabel, ui->JZlabel
        };

        for (QLabel* label : disabledLabelsY) {
            if (label) {
                label->setEnabled(false);
            }
        }

        break;
    }

    case 3:  // 3-DOF
    {
        // 启用所有控
        for (QLineEdit* edit : allAxisEdits) {
            if (edit) {
                edit->setEnabled(true);

                // 清除可能的错误状态
                edit->setProperty("error", "false");
                edit->style()->unpolish(edit);
                edit->style()->polish(edit);
            }
        }

        // 启用所有标
        QVector<QLabel*> allAxisLabels = {
            ui->labelP0X, ui->labelP0Y, ui->labelP0Z,
            ui->labelPfX, ui->labelPfY, ui->labelPfZ,
            ui->labelV0X, ui->labelV0Y, ui->labelV0Z,
            ui->labelVfX, ui->labelVfY, ui->labelVfZ,
            ui->VXlabel, ui->VYlabel, ui->VZlabel,
            ui->AXlabel, ui->AYlabel, ui->AZlabel,
            ui->JXlabel, ui->JYlabel, ui->JZlabel
        };

        for (QLabel* label : allAxisLabels) {
            if (label) {
                label->setEnabled(true);
            }
        }
        break;
    }

    case 6:  // 6-DOF
    {
        qDebug() << "6-DOF mode: enabling all axis controls";

        for (QLineEdit* edit : allAxisEdits) {
            if (edit) {
                edit->setEnabled(true);
                edit->setProperty("error", "false");
                edit->style()->unpolish(edit);
                edit->style()->polish(edit);
            }
        }

        QVector<QLabel*> allAxisLabels = {
            ui->labelP0X, ui->labelP0Y, ui->labelP0Z,
            ui->labelPfX, ui->labelPfY, ui->labelPfZ,
            ui->labelV0X, ui->labelV0Y, ui->labelV0Z,
            ui->labelVfX, ui->labelVfY, ui->labelVfZ,
            ui->VXlabel, ui->VYlabel, ui->VZlabel,
            ui->AXlabel, ui->AYlabel, ui->AZlabel,
            ui->JXlabel, ui->JYlabel, ui->JZlabel
        };

        for (QLabel* label : allAxisLabels) {
            if (label) {
                label->setEnabled(true);
            }
        }

        qDebug() << "6-DOF mode setup complete, m_currentDOF =" << m_currentDOF;
        break;
    }
}

    // 强制刷新布局
    ui->centralwidget->adjustSize();
    qDebug() << "DOF UI更新完成，模式：" << dof << "，m_currentDOF =" << m_currentDOF;
}

// 3. 重置DOF特定输入
void MainWindow::resetDOFSpecificInputs(int dof)
{
    Q_UNUSED(dof);
    // 这里可以清空隐藏的输入框，避免残留数
    // 例如：if (dof == 1) { ui->editP0Y->clear(); ... }
}

// 4. 显示DOF帮助信息
void MainWindow::showDOFHelp(int dof)
{
    QString helpText;
    switch (dof) {
    case 1:
        helpText = "1-DOF模式：单自由度直线运动，仅需输入X轴的运动学约束和位置信息";
        break;
    case 3:
        helpText = "3-DOF mode: input X/Y/Z kinematic limits and state values";
        break;
    case 6:
        helpText = "6-DOF模式：完整空间位姿控制，需要位置和姿态信息（当前仅支持位置控制）";
        break;
    default:
        helpText = "未知DOF模式";
        break;
    }
    updateStatusMessage(helpText, false);
}

// 5. 设置DOF默认
void MainWindow::setDefaultValuesForDOF(int dof)
{
    switch (dof) {
    case 1:  // 1-DOF默认
        if (ui->VXlineEdit->text().isEmpty()) ui->VXlineEdit->setText("2.0");
        if (ui->accXlineEdit->text().isEmpty()) ui->accXlineEdit->setText("1.0");
        if (ui->JXlineEdit->text().isEmpty()) ui->JXlineEdit->setText("5.0");
        if (ui->editP0X->text().isEmpty()) ui->editP0X->setText("0.0");
        if (ui->editPfX->text().isEmpty()) ui->editPfX->setText("10.0");
        if (ui->editV0X->text().isEmpty()) ui->editV0X->setText("0.0");
        if (ui->editVfX->text().isEmpty()) ui->editVfX->setText("0.0");
        break;

    case 3:  // 3-DOF默认
        if (ui->VXlineEdit->text().isEmpty()) ui->VXlineEdit->setText("2.0");
        if (ui->VYlineEdit->text().isEmpty()) ui->VYlineEdit->setText("2.0");
        if (ui->VZlineEdit->text().isEmpty()) ui->VZlineEdit->setText("1.0");
        if (ui->accXlineEdit->text().isEmpty()) ui->accXlineEdit->setText("1.0");
        if (ui->accYlineEdit->text().isEmpty()) ui->accYlineEdit->setText("1.0");
        if (ui->accZlineEdit->text().isEmpty()) ui->accZlineEdit->setText("0.5");
        if (ui->JXlineEdit->text().isEmpty()) ui->JXlineEdit->setText("5.0");
        if (ui->JYlineEdit->text().isEmpty()) ui->JYlineEdit->setText("5.0");
        if (ui->JZlineEdit->text().isEmpty()) ui->JZlineEdit->setText("3.0");
        if (ui->editP0X->text().isEmpty()) ui->editP0X->setText("0.0");
        if (ui->editP0Y->text().isEmpty()) ui->editP0Y->setText("0.0");
        if (ui->editP0Z->text().isEmpty()) ui->editP0Z->setText("0.0");
        if (ui->editPfX->text().isEmpty()) ui->editPfX->setText("10.0");
        if (ui->editPfY->text().isEmpty()) ui->editPfY->setText("5.0");
        if (ui->editPfZ->text().isEmpty()) ui->editPfZ->setText("3.0");
        if (ui->editV0X->text().isEmpty()) ui->editV0X->setText("0.0");
        if (ui->editV0Y->text().isEmpty()) ui->editV0Y->setText("0.0");
        if (ui->editV0Z->text().isEmpty()) ui->editV0Z->setText("0.0");
        if (ui->editVfX->text().isEmpty()) ui->editVfX->setText("0.0");
        if (ui->editVfY->text().isEmpty()) ui->editVfY->setText("0.0");
        if (ui->editVfZ->text().isEmpty()) ui->editVfZ->setText("0.0");
        break;

    case 6:  // 6-DOF默认
        setDefaultValuesForDOF(3);  // 先设-DOF的默认
        // TODO: 如果已添加姿态控件，设置姿默认
        break;
    }
}

// 6. DOF特定的输入验证证函
bool MainWindow::validate1DOFInputs()
{
    bool valid = true;
    QLineEdit* firstEmpty = nullptr;
    QStringList emptyFields;  // 记录空的字段名，用于提示信息

    // 1-DOF模式下只验证X轴控
    QStringList fields1DOF = {
        "VXlineEdit", "accXlineEdit", "JXlineEdit",
        "editP0X", "editPfX", "editV0X", "editVfX"
    };

    for (const QString& fieldName : fields1DOF) {
        QLineEdit* edit = findChild<QLineEdit*>(fieldName);
        if (edit && edit->isVisible() && edit->isEnabled() && edit->text().trimmed().isEmpty()) {
            // 设置错误状态
            edit->setProperty("error", "true");
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);

            // 记录空字
            QString displayName = getDisplayNameForField(fieldName);
            if (!displayName.isEmpty()) {
                emptyFields << displayName;
            }

            if (!firstEmpty) firstEmpty = edit;
            valid = false;
        }
    }

    if (!valid) {
        // 构建详细的错误信
        QString errorMessage = "1-DOF模式下，以下参数未填写：\n\n";
        for (const QString& field : emptyFields) {
            errorMessage += "" + field + "\n";
        }
        errorMessage += "\nPlease fill in all required X-axis parameters.";

        QMessageBox::warning(this, "参数缺失", errorMessage);

        if (firstEmpty) {
            firstEmpty->setFocus();
            qDebug() << "参数验证失败，焦点设置到:" << firstEmpty->objectName();
        }
    } else {
        // 清除所有可能的错误状态
        for (const QString& fieldName : fields1DOF) {
            if (QLineEdit* edit = findChild<QLineEdit*>(fieldName)) {
                if (edit->property("error") == "true") {
                    edit->setProperty("error", "false");
                    edit->style()->unpolish(edit);
                    edit->style()->polish(edit);
                }
            }
        }
    }

    qDebug() << "1-DOF参数验证" << (valid ? "通过" : "失败");
    return valid;
}
bool MainWindow::validate3DOFInputs()
{
    bool valid = true;
    QLineEdit* firstEmpty = nullptr;

    // 检查所有3-DOF需要的输入
    QStringList fields3DOF = {
        "VXlineEdit", "VYlineEdit", "VZlineEdit",
        "accXlineEdit", "accYlineEdit", "accZlineEdit",
        "JXlineEdit", "JYlineEdit", "JZlineEdit",
        "editP0X", "editP0Y", "editP0Z",
        "editPfX", "editPfY", "editPfZ",
        "editV0X", "editV0Y", "editV0Z",
        "editVfX", "editVfY", "editVfZ"
    };

    for (const QString& fieldName : fields3DOF) {
        QLineEdit* edit = findChild<QLineEdit*>(fieldName);
        if (edit && edit->isVisible() && edit->isEnabled() && edit->text().trimmed().isEmpty()) {
            edit->setProperty("error", "true");
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
            if (!firstEmpty) firstEmpty = edit;
            valid = false;
        }
    }

    if (!valid) {
        QMessageBox::warning(this, "参数缺失",
            "3-DOF模式下，请填写所有必需的参数！");
        if (firstEmpty) firstEmpty->setFocus();
    }

    return valid;
}

bool MainWindow::validate6DOFInputs()
{
    // 6-DOF验证应该独立，不调用3-DOF验证
    clearAllErrorStates();  // 清除所有错误状态

    bool valid = true;
    QString errorMessage = "6-DOF模式下，以下参数未填写：\n\n";

    // 检查线运动参数
    QList<QLineEdit*> requiredEdits = {
        findChild<QLineEdit*>("editP0X"),  // 起始位置X
        findChild<QLineEdit*>("editP0Y"),  // 起始位置Y
        findChild<QLineEdit*>("editP0Z"),  // 起始位置Z
        findChild<QLineEdit*>("editPfX"),  // 目标位置X
        findChild<QLineEdit*>("editPfY"),  // 目标位置Y
        findChild<QLineEdit*>("editPfZ"),  // 目标位置Z
        findChild<QLineEdit*>("editV0X"),  // 起始速度X
        findChild<QLineEdit*>("editV0Y"),  // 起始速度Y
        findChild<QLineEdit*>("editV0Z"),  // 起始速度Z
        findChild<QLineEdit*>("editVfX"),  // 目标速度X
        findChild<QLineEdit*>("editVfY"),  // 目标速度Y
        findChild<QLineEdit*>("editVfZ"),  // 目标速度Z
        findChild<QLineEdit*>("VXlineEdit"),  // 最大速度X
        findChild<QLineEdit*>("VYlineEdit"),  // 最大速度Y
        findChild<QLineEdit*>("VZlineEdit"),  // 最大速度Z
        findChild<QLineEdit*>("accXlineEdit"),  // 最大加速度X
        findChild<QLineEdit*>("accYlineEdit"),  // 最大加速度Y
        findChild<QLineEdit*>("accZlineEdit"),  // 最大加速度Z
        findChild<QLineEdit*>("JXlineEdit"),   // 最大加加速度X
        findChild<QLineEdit*>("JYlineEdit"),   // 最大加加速度Y
        findChild<QLineEdit*>("JZlineEdit")    // 最大加加速度Z
    };

    QLineEdit* firstEmpty = nullptr;

    for (QLineEdit* edit : requiredEdits) {
        if (edit && edit->isVisible() && edit->isEnabled()) {
            QString text = edit->text().trimmed();
            if (text.isEmpty() ) {  // 允许0值，不允许空
                edit->setProperty("error", "true");
                edit->style()->unpolish(edit);
                edit->style()->polish(edit);

                if (!firstEmpty) firstEmpty = edit;
                valid = false;

                // 添加错误信息
                QString fieldName = edit->objectName();
                errorMessage += "" + getDisplayNameForField(fieldName) + "\n";
            }

        }
    }

    // 检查姿态参数是否有
    if (!m_attitudeParams.isValid()) {
        QMessageBox::warning(this, "姿参数未设置",
            "6-DOF模式需要设置姿态参数\n"
            "Click the 6-DOF button to set attitude parameters.");
        return false;
    }

    if (!valid) {
        errorMessage += "\nPlease fill in all required 6-DOF parameters.";

        // 显示6-DOF的错误信
        QMessageBox::warning(this, "6-DOF参数缺失", errorMessage);
        if (firstEmpty) {
            firstEmpty->setFocus();
            firstEmpty->selectAll();
        }
    }

    return valid;
}
QString MainWindow::getDisplayNameForField(const QString& fieldName)
{
    static QMap<QString, QString> fieldNameMap = {
        {"editP0X", "起始位置X"},
        {"editPfX", "目标位置X"},
        {"editV0X", "起始速度X"},
        {"editVfX", "目标速度X"},
        {"VXlineEdit", "最大速度X"},
        {"accXlineEdit", "最大加速度X"},
        {"JXlineEdit", "最大加加速度X"},
        {"editP0Y", "起始位置Y"},
        {"editPfY", "目标位置Y"},
        {"editV0Y", "起始速度Y"},
        {"editVfY", "目标速度Y"},
        {"VYlineEdit", "最大速度Y"},
        {"accYlineEdit", "最大加速度Y"},
        {"JYlineEdit", "最大加加速度Y"},
        {"editP0Z", "起始位置Z"},
        {"editPfZ", "目标位置Z"},
        {"editV0Z", "起始速度Z"},
        {"editVfZ", "目标速度Z"},
        {"VZlineEdit", "最大速度Z"},
        {"accZlineEdit", "最大加速度Z"},
        {"JZlineEdit", "最大加加速度Z"}
    };

    return fieldNameMap.value(fieldName, fieldName);
}
void MainWindow::clearAllErrorStates() const{
    qDebug() << "Clearing all input error states";

    QList<QLineEdit*> allEdits = this->findChildren<QLineEdit*>();
    for (QLineEdit* edit : allEdits) {
        if (edit) {
            QString currentError = edit->property("error").toString();
            if (currentError == "true") {
                qDebug() << "  Cleared" << edit->objectName() << "error state";
                edit->setProperty("error", "false");
                edit->style()->unpolish(edit);
                edit->style()->polish(edit);
            }
        }
    }
}

    // 仿真期间禁用所有输
void MainWindow::disableAllInputsDuringSimulation()
{
    qDebug() << "Disabling inputs during simulation";

        // 1. 禁用所有QLineEdit
        QList<QLineEdit*> allLineEdits = findChildren<QLineEdit*>();
        for (QLineEdit* edit : allLineEdits) {
            if (edit && edit->isEnabled()) {
                edit->setEnabled(false);
            }
        }

        // 2. 禁用DOF按钮
        if (m_dofGroup) {
            QList<QAbstractButton*> dofButtons = m_dofGroup->buttons();
            for (QAbstractButton* btn : dofButtons) {
                if (btn) {
                    btn->setEnabled(false);
                }
            }
        }

        // 3. 禁用生成按钮
        ui->btnGenerate->setEnabled(false);

        // 4. 禁用同步模式下拉
        if (ui->syncComboBox) {
            ui->syncComboBox->setEnabled(false);
        }

        // 5. 禁用控制周期输入
        if (ui->cycleLineEdit) {
            ui->cycleLineEdit->setEnabled(false);
        }

        qDebug() << "已禁用" << allLineEdits.size() << "个输入控件";
    }

    // 复位后重新启用输
void MainWindow::enableAllInputsAfterReset()
{
    qDebug() << "复位后重新启用输入控件";

    int currentDOF = m_currentDOF;

    // 1. 先禁用所有输入控
    QList<QLineEdit*> allLineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : allLineEdits) {
        if (edit) {
            edit->setEnabled(false);
        }
    }

    // 2. 根据当前DOF重新启用
    switch (currentDOF) {
    case 1:  // 1-DOF
    {
        // 启用X轴输入框
        QVector<QLineEdit*> xAxisEdits = {
            ui->editP0X, ui->editPfX,
            ui->editV0X, ui->editVfX,
            ui->VXlineEdit, ui->accXlineEdit, ui->JXlineEdit
        };
        for (QLineEdit* edit : xAxisEdits) {
            if (edit) edit->setEnabled(true);
        }
        break;
    }

    case 3:  // 3-DOF
    {
        // 启用所有输入框
        QVector<QLineEdit*> allEdits3DOF = {
            ui->editP0X, ui->editP0Y, ui->editP0Z,
            ui->editPfX, ui->editPfY, ui->editPfZ,
            ui->editV0X, ui->editV0Y, ui->editV0Z,
            ui->editVfX, ui->editVfY, ui->editVfZ,
            ui->VXlineEdit, ui->VYlineEdit, ui->VZlineEdit,
            ui->accXlineEdit, ui->accYlineEdit, ui->accZlineEdit,
            ui->JXlineEdit, ui->JYlineEdit, ui->JZlineEdit
        };
        for (QLineEdit* edit : allEdits3DOF) {
                if (edit) edit->setEnabled(true);
        }
        break;
    }

    case 6:  // 6-DOF
    {
        QVector<QLineEdit*> allEdits6DOF = {
            ui->editP0X, ui->editP0Y, ui->editP0Z,
            ui->editPfX, ui->editPfY, ui->editPfZ,
            ui->editV0X, ui->editV0Y, ui->editV0Z,
            ui->editVfX, ui->editVfY, ui->editVfZ,
            ui->VXlineEdit, ui->VYlineEdit, ui->VZlineEdit,
            ui->accXlineEdit, ui->accYlineEdit, ui->accZlineEdit,
            ui->JXlineEdit, ui->JYlineEdit, ui->JZlineEdit
        };
        for (QLineEdit* edit : allEdits6DOF) {
            if (edit) edit->setEnabled(true);
        }
        break;
    }
    }

    // 3. 重新启用DOF按钮
    if (m_dofGroup) {
        QList<QAbstractButton*> dofButtons = m_dofGroup->buttons();
        for (QAbstractButton* btn : dofButtons) {
            if (btn) {
                btn->setEnabled(true);
            }
        }
    }

    // 4. 重新启用生成按钮
    ui->btnGenerate->setEnabled(true);

    // 5. 重新启用同步模式下拉
    if (ui->syncComboBox) {
        ui->syncComboBox->setEnabled(true);
    }
    // 6. 重新启用控制周期输入
    if (ui->cycleLineEdit) {
        ui->cycleLineEdit->setEnabled(true);
    }

    qDebug() << "Inputs restored for DOF" << currentDOF;
}
