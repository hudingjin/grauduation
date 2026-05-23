    #ifndef MAINWINDOW_H
    #define MAINWINDOW_H
    #include "../core/FlightParams.h"
    #include "../core/SimulationManager.h"
    #include"dialogs/AttitudeDialog.h"
    #include "widgets/DataTableWidget.h"
    #include"TitleBarWidget.h"
    #include <QMainWindow>
    #include <QLineEdit>
    #include <QDebug>
    #include <QMessageBox>
    #include <QButtonGroup>
    #include <QProgressBar>
    #include <QLabel>
    #include <QDialog>

    QT_BEGIN_NAMESPACE
    namespace Ui {
    class MainWindow;
    }
    QT_END_NAMESPACE


    // 前向声明图表控件
    class ChartWidget;
    class DataTableWidget;
    class StatusWidget;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override ;

    private slots:
        void on_btnGenerate_clicked();

        void onZoomTableClicked();

        // 用于实现：用户一旦输入，红色边框消失
        void onInputTextChanged(const QString &text) const;
        void on_btnPlay_clicked();

        void on_btnStop_clicked();

        void on_btnReset_clicked();

        void on_btnExport_clicked();


        // SimulationManager信号槽
        void onTrajectoryGenerated(const core::TrajectoryResult& result);
        void onTrajectoryGenerationStarted();
        void onTrajectoryGenerationFinished(bool success);
        void onSimulationStateChanged(core::SimulationState newState);
        void onSimulationTimeUpdated(double time);
        void onRealtimeStateUpdated(const core::TrajectoryPoint& state);
        void onProgressUpdated(double progress);
        void onErrorOccurred(const QString& error);
        void updatePlayButtonState(core::SimulationState state) const;
    private:
        Ui::MainWindow *ui;
        QButtonGroup* m_dofGroup;  // DOF按钮组成员变量
        int m_currentDOF = 3;
        bool m_simulationRunning = false;  // 标记仿真是否运行
        // 核心数据对象
        core::FlightParams m_flightParams;
        core::SimulationManager* m_simManager;  // 仿真管理器指针

        // UI控件
       // QProgressBar* m_progressBar;  // 进度条
        QLabel* m_statusLabel;        // 状态标签

        // 图表控件
        ChartWidget* m_positionChart = nullptr;
        ChartWidget* m_velocityChart = nullptr;
        ChartWidget* m_accelerationChart = nullptr;
        ChartWidget* m_attitudeChart = nullptr;
        // 数据表格控件
        DataTableWidget* m_dataTable = nullptr;
        // 创建放大表格
        QTableWidget* createZoomedTable();
        // 状态标签指针
        StatusWidget* m_statusWidget = nullptr;
        QLabel* m_labelRollValue = nullptr;
        QLabel* m_labelPitchValue = nullptr;
        QLabel* m_labelYawValue = nullptr;
        QLabel* m_valueTimeCurrent = nullptr;
        QLabel* m_valueTimeTotal = nullptr;
        QLabel* m_valuePosition = nullptr;
        QLabel* m_valueSpeed = nullptr;
        QLabel* m_valueStatus = nullptr;
        QLabel* m_valueRuckig = nullptr;
        TitleBarWidget* m_titleBarWidget = nullptr;
        AttitudeDialog* m_attitudeDialog = nullptr;  // 姿态对话框
        core::AttitudeParams m_attitudeParams;  // 当前姿态参数


        // 初始化函数
        void initConnections();      // 初始化信号槽连接
        void initValidators();       // 专门负责输入限制
        void initButtonGroups();     // 专门负责按钮互斥逻辑
        void initControlPanel();     // 初始化控制面板
        void initCharts();           // 初始化图表
        void initStatusWidget();
        void updateUIForState(core::SimulationState state);  // 根据状态更新UI
        void updateChartsWithTrajectory(const core::TrajectoryResult& trajectory);

        // 工具函数
        bool validateInputs();                         // 检查输入是否为空并标记
        static double safeReadDouble(QLineEdit* edit); // 安全读取数值
        void updateStatusMessage(const QString& message, bool isError = false) const;
        void updateUIForDOF(int dof);
        void setupDOFConnections();
        void resetDOFSpecificInputs(int dof);
        void showDOFHelp(int dof);
        void setDefaultValuesForDOF(int dof);
        void setupTableControls();
        // 放大表格对话框
        void showZoomedTableDialog();
        void exportTableToCSV(QTableWidget* table, const QString& filename);
        QWidget* createWidgetFromLayout(QLayout* layout);
        QString getDisplayNameForField(const QString& chars);
        void clearAllErrorStates() const;
        bool validate1DOFInputs();
        bool validate3DOFInputs();
        bool validate6DOFInputs();
        void disableAllInputsDuringSimulation(); // 仿真期间禁用所有输入
        void enableAllInputsAfterReset(); // 复位后重新启用输入
    };
    #endif // MAINWINDOW_H
