#include "FlightConfigRepository.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace core {

FlightConfigRepository::FlightConfigRepository(QObject* parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("flight_config_repository"))
{
}

bool FlightConfigRepository::initialize()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QCoreApplication::applicationDirPath();
    }

    QDir dir(dataDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        m_lastError = QStringLiteral("无法创建数据库目录: %1").arg(dataDir);
        return false;
    }

    m_databasePath = dir.filePath(QStringLiteral("flight_configs.sqlite"));
    return ensureSchema();
}

QString FlightConfigRepository::databasePath() const
{
    return m_databasePath;
}

QString FlightConfigRepository::lastError() const
{
    return m_lastError;
}

QList<FlightConfigSummary> FlightConfigRepository::loadSummaries() const
{
    QList<FlightConfigSummary> summaries;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen()) {
        return summaries;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, name, dof, created_at "
        "FROM flight_configs "
        "ORDER BY datetime(created_at) DESC, id DESC"));

    if (!query.exec()) {
        return summaries;
    }

    while (query.next()) {
        FlightConfigSummary summary;
        summary.id = query.value(0).toInt();
        summary.name = query.value(1).toString();
        summary.dof = query.value(2).toInt();
        summary.createdAt = query.value(3).toString();
        summaries.push_back(summary);
    }

    return summaries;
}

bool FlightConfigRepository::loadConfig(int id, FlightParams* params) const
{
    if (!params) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM flight_configs WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec() || !query.next()) {
        return false;
    }

    RuckigConfig config;
    KinematicLimits limits;
    StatePoint start;
    StatePoint target;
    AttitudeParams attitude;

    config.dof = query.value(QStringLiteral("dof")).toInt();
    config.syncMode = query.value(QStringLiteral("sync_mode")).toString();
    config.deltaT = query.value(QStringLiteral("delta_t")).toDouble() / 1000.0;

    start.x = query.value(QStringLiteral("start_x")).toDouble();
    start.y = query.value(QStringLiteral("start_y")).toDouble();
    start.z = query.value(QStringLiteral("start_z")).toDouble();
    start.vx = query.value(QStringLiteral("start_vx")).toDouble();
    start.vy = query.value(QStringLiteral("start_vy")).toDouble();
    start.vz = query.value(QStringLiteral("start_vz")).toDouble();

    target.x = query.value(QStringLiteral("target_x")).toDouble();
    target.y = query.value(QStringLiteral("target_y")).toDouble();
    target.z = query.value(QStringLiteral("target_z")).toDouble();
    target.vx = query.value(QStringLiteral("target_vx")).toDouble();
    target.vy = query.value(QStringLiteral("target_vy")).toDouble();
    target.vz = query.value(QStringLiteral("target_vz")).toDouble();

    limits.velX = query.value(QStringLiteral("max_vel_x")).toDouble();
    limits.velY = query.value(QStringLiteral("max_vel_y")).toDouble();
    limits.velZ = query.value(QStringLiteral("max_vel_z")).toDouble();
    limits.accX = query.value(QStringLiteral("max_acc_x")).toDouble();
    limits.accY = query.value(QStringLiteral("max_acc_y")).toDouble();
    limits.accZ = query.value(QStringLiteral("max_acc_z")).toDouble();
    limits.jerkX = query.value(QStringLiteral("max_jerk_x")).toDouble();
    limits.jerkY = query.value(QStringLiteral("max_jerk_y")).toDouble();
    limits.jerkZ = query.value(QStringLiteral("max_jerk_z")).toDouble();

    attitude.roll0 = query.value(QStringLiteral("att_roll0")).toDouble();
    attitude.pitch0 = query.value(QStringLiteral("att_pitch0")).toDouble();
    attitude.yaw0 = query.value(QStringLiteral("att_yaw0")).toDouble();
    attitude.rollf = query.value(QStringLiteral("att_roll_f")).toDouble();
    attitude.pitchf = query.value(QStringLiteral("att_pitch_f")).toDouble();
    attitude.yawf = query.value(QStringLiteral("att_yaw_f")).toDouble();
    attitude.maxRollVel = query.value(QStringLiteral("att_max_roll_vel")).toDouble();
    attitude.maxPitchVel = query.value(QStringLiteral("att_max_pitch_vel")).toDouble();
    attitude.maxYawVel = query.value(QStringLiteral("att_max_yaw_vel")).toDouble();
    attitude.maxRollAcc = query.value(QStringLiteral("att_max_roll_acc")).toDouble();
    attitude.maxPitchAcc = query.value(QStringLiteral("att_max_pitch_acc")).toDouble();
    attitude.maxYawAcc = query.value(QStringLiteral("att_max_yaw_acc")).toDouble();
    attitude.maxRollJerk = query.value(QStringLiteral("att_max_roll_jerk")).toDouble();
    attitude.maxPitchJerk = query.value(QStringLiteral("att_max_pitch_jerk")).toDouble();
    attitude.maxYawJerk = query.value(QStringLiteral("att_max_yaw_jerk")).toDouble();

    params->setConfig(config);
    params->setLimits(limits);
    params->setStartState(start);
    params->setTargetState(target);
    params->setIs6DOF(config.dof == 6);
    if (config.dof == 6) {
        params->setAttitudeParams(attitude);
    }

    return true;
}

bool FlightConfigRepository::saveSuccessfulConfig(const FlightParams& params, const QString& name)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen()) {
        m_lastError = QStringLiteral("数据库未打开");
        return false;
    }

    const auto& config = params.getConfig();
    const auto& limits = params.getLimits();
    const auto& start = params.getStartState();
    const auto& target = params.getTargetState();
    const auto& attitude = params.getAttitudeParams();

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO flight_configs ("
        "name, dof, start_x, start_y, start_z, start_vx, start_vy, start_vz, "
        "target_x, target_y, target_z, target_vx, target_vy, target_vz, "
        "max_vel_x, max_vel_y, max_vel_z, max_acc_x, max_acc_y, max_acc_z, "
        "max_jerk_x, max_jerk_y, max_jerk_z, "
        "att_roll0, att_pitch0, att_yaw0, att_roll_f, att_pitch_f, att_yaw_f, "
        "att_max_roll_vel, att_max_pitch_vel, att_max_yaw_vel, "
        "att_max_roll_acc, att_max_pitch_acc, att_max_yaw_acc, "
        "att_max_roll_jerk, att_max_pitch_jerk, att_max_yaw_jerk, "
        "sync_mode, delta_t"
        ") VALUES ("
        ":name, :dof, :start_x, :start_y, :start_z, :start_vx, :start_vy, :start_vz, "
        ":target_x, :target_y, :target_z, :target_vx, :target_vy, :target_vz, "
        ":max_vel_x, :max_vel_y, :max_vel_z, :max_acc_x, :max_acc_y, :max_acc_z, "
        ":max_jerk_x, :max_jerk_y, :max_jerk_z, "
        ":att_roll0, :att_pitch0, :att_yaw0, :att_roll_f, :att_pitch_f, :att_yaw_f, "
        ":att_max_roll_vel, :att_max_pitch_vel, :att_max_yaw_vel, "
        ":att_max_roll_acc, :att_max_pitch_acc, :att_max_yaw_acc, "
        ":att_max_roll_jerk, :att_max_pitch_jerk, :att_max_yaw_jerk, "
        ":sync_mode, :delta_t)"));

    query.bindValue(QStringLiteral(":name"), name.trimmed().isEmpty() ? makeDefaultName(params) : name.trimmed());
    query.bindValue(QStringLiteral(":dof"), config.dof);
    query.bindValue(QStringLiteral(":start_x"), start.x);
    query.bindValue(QStringLiteral(":start_y"), start.y);
    query.bindValue(QStringLiteral(":start_z"), start.z);
    query.bindValue(QStringLiteral(":start_vx"), start.vx);
    query.bindValue(QStringLiteral(":start_vy"), start.vy);
    query.bindValue(QStringLiteral(":start_vz"), start.vz);
    query.bindValue(QStringLiteral(":target_x"), target.x);
    query.bindValue(QStringLiteral(":target_y"), target.y);
    query.bindValue(QStringLiteral(":target_z"), target.z);
    query.bindValue(QStringLiteral(":target_vx"), target.vx);
    query.bindValue(QStringLiteral(":target_vy"), target.vy);
    query.bindValue(QStringLiteral(":target_vz"), target.vz);
    query.bindValue(QStringLiteral(":max_vel_x"), limits.velX);
    query.bindValue(QStringLiteral(":max_vel_y"), limits.velY);
    query.bindValue(QStringLiteral(":max_vel_z"), limits.velZ);
    query.bindValue(QStringLiteral(":max_acc_x"), limits.accX);
    query.bindValue(QStringLiteral(":max_acc_y"), limits.accY);
    query.bindValue(QStringLiteral(":max_acc_z"), limits.accZ);
    query.bindValue(QStringLiteral(":max_jerk_x"), limits.jerkX);
    query.bindValue(QStringLiteral(":max_jerk_y"), limits.jerkY);
    query.bindValue(QStringLiteral(":max_jerk_z"), limits.jerkZ);

    const bool is6Dof = config.dof == 6;
    query.bindValue(QStringLiteral(":att_roll0"), is6Dof ? attitude.roll0 : QVariant());
    query.bindValue(QStringLiteral(":att_pitch0"), is6Dof ? attitude.pitch0 : QVariant());
    query.bindValue(QStringLiteral(":att_yaw0"), is6Dof ? attitude.yaw0 : QVariant());
    query.bindValue(QStringLiteral(":att_roll_f"), is6Dof ? attitude.rollf : QVariant());
    query.bindValue(QStringLiteral(":att_pitch_f"), is6Dof ? attitude.pitchf : QVariant());
    query.bindValue(QStringLiteral(":att_yaw_f"), is6Dof ? attitude.yawf : QVariant());
    query.bindValue(QStringLiteral(":att_max_roll_vel"), is6Dof ? attitude.maxRollVel : QVariant());
    query.bindValue(QStringLiteral(":att_max_pitch_vel"), is6Dof ? attitude.maxPitchVel : QVariant());
    query.bindValue(QStringLiteral(":att_max_yaw_vel"), is6Dof ? attitude.maxYawVel : QVariant());
    query.bindValue(QStringLiteral(":att_max_roll_acc"), is6Dof ? attitude.maxRollAcc : QVariant());
    query.bindValue(QStringLiteral(":att_max_pitch_acc"), is6Dof ? attitude.maxPitchAcc : QVariant());
    query.bindValue(QStringLiteral(":att_max_yaw_acc"), is6Dof ? attitude.maxYawAcc : QVariant());
    query.bindValue(QStringLiteral(":att_max_roll_jerk"), is6Dof ? attitude.maxRollJerk : QVariant());
    query.bindValue(QStringLiteral(":att_max_pitch_jerk"), is6Dof ? attitude.maxPitchJerk : QVariant());
    query.bindValue(QStringLiteral(":att_max_yaw_jerk"), is6Dof ? attitude.maxYawJerk : QVariant());
    query.bindValue(QStringLiteral(":sync_mode"), config.syncMode);
    query.bindValue(QStringLiteral(":delta_t"), config.deltaT * 1000.0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QString FlightConfigRepository::makeDefaultName(const FlightParams& params) const
{
    return QStringLiteral("%1-DOF 成功参数 %2")
        .arg(params.getConfig().dof)
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
}

bool FlightConfigRepository::ensureSchema()
{
    if (m_databasePath.isEmpty()) {
        m_lastError = QStringLiteral("数据库路径为空");
        return false;
    }

    QSqlDatabase db;
    if (QSqlDatabase::contains(m_connectionName)) {
        db = QSqlDatabase::database(m_connectionName);
    } else {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    const QString schema = QStringLiteral(R"SQL(
CREATE TABLE IF NOT EXISTS flight_configs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL,
    dof         INTEGER NOT NULL DEFAULT 3,
    created_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
    start_x     REAL    NOT NULL DEFAULT 0.0,
    start_y     REAL    NOT NULL DEFAULT 0.0,
    start_z     REAL    NOT NULL DEFAULT 1000.0,
    start_vx    REAL    NOT NULL DEFAULT 0.0,
    start_vy    REAL    NOT NULL DEFAULT 0.0,
    start_vz    REAL    NOT NULL DEFAULT 0.0,
    target_x    REAL    NOT NULL DEFAULT 500.0,
    target_y    REAL    NOT NULL DEFAULT 300.0,
    target_z    REAL    NOT NULL DEFAULT 800.0,
    target_vx   REAL    NOT NULL DEFAULT 0.0,
    target_vy   REAL    NOT NULL DEFAULT 0.0,
    target_vz   REAL    NOT NULL DEFAULT 0.0,
    max_vel_x   REAL    NOT NULL DEFAULT 80.0,
    max_vel_y   REAL    NOT NULL DEFAULT 80.0,
    max_vel_z   REAL    NOT NULL DEFAULT 30.0,
    max_acc_x   REAL    NOT NULL DEFAULT 15.0,
    max_acc_y   REAL    NOT NULL DEFAULT 15.0,
    max_acc_z   REAL    NOT NULL DEFAULT 8.0,
    max_jerk_x  REAL    NOT NULL DEFAULT 50.0,
    max_jerk_y  REAL    NOT NULL DEFAULT 50.0,
    max_jerk_z  REAL    NOT NULL DEFAULT 20.0,
    att_roll0     REAL,
    att_pitch0    REAL,
    att_yaw0      REAL,
    att_roll_f    REAL,
    att_pitch_f   REAL,
    att_yaw_f     REAL,
    att_max_roll_vel   REAL,
    att_max_pitch_vel  REAL,
    att_max_yaw_vel    REAL,
    att_max_roll_acc   REAL,
    att_max_pitch_acc  REAL,
    att_max_yaw_acc    REAL,
    att_max_roll_jerk  REAL,
    att_max_pitch_jerk REAL,
    att_max_yaw_jerk   REAL,
    sync_mode   TEXT    NOT NULL DEFAULT 'TIME',
    delta_t     REAL    NOT NULL DEFAULT 10.0
)
)SQL");

    if (!query.exec(schema)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

} // namespace core
