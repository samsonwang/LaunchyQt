
#pragma once
#include <QtGlobal>

class QLogger {
public:
    static void setLogLevel(QtMsgType type);
    static void messageHandler(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg);
private:
    QLogger();
    Q_DISABLE_COPY(QLogger)

private:
    static FILE* s_logFile;
    static QtMsgType s_logLevel;
};
