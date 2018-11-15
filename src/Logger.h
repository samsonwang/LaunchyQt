
#pragma once
#include <QtGlobal>
#include <cstdio>
namespace launchy {
class Logger {
public:
    static void stopLogging();
    static void setLogLevel(int index);
    static void setLogLevel(QtMsgType type);
    static void messageHandler(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg);
private:
    Logger();
    Q_DISABLE_COPY(Logger)

private:
    static FILE* s_logFile;
    static QtMsgType s_logLevel;
};
}
