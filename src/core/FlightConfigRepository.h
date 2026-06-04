#ifndef FLIGHTCONFIGREPOSITORY_H
#define FLIGHTCONFIGREPOSITORY_H

#include "FlightParams.h"

#include <QList>
#include <QObject>
#include <QString>

namespace core {

struct FlightConfigSummary {
    int id = 0;
    QString name;
    int dof = 3;
    QString createdAt;
};

class FlightConfigRepository : public QObject
{
    Q_OBJECT

public:
    explicit FlightConfigRepository(QObject* parent = nullptr);

    bool initialize();
    [[nodiscard]] QString databasePath() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QList<FlightConfigSummary> loadSummaries() const;
    [[nodiscard]] bool loadConfig(int id, FlightParams* params) const;
    bool saveSuccessfulConfig(const FlightParams& params, const QString& name = QString());

private:
    QString m_connectionName;
    QString m_databasePath;
    QString m_lastError;

    [[nodiscard]] QString makeDefaultName(const FlightParams& params) const;
    bool ensureSchema();
};

} // namespace core

#endif // FLIGHTCONFIGREPOSITORY_H
