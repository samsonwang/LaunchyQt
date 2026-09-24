
#include "Logger.h"

#include <cstdio>

#include <QDir>
#include <QString>
#include <QDateTime>
#include <QByteArray>

namespace launchy {
namespace log {

static FILE* s_logFile = nullptr;
static QtMsgType s_logLevel = QtWarningMsg;

// QtMsgType values are not ordered by severity (QtInfoMsg was appended
// after QtFatalMsg), so map each type to a monotonic severity rank.
static int severity(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return 0;
    case QtInfoMsg:
        return 1;
    case QtWarningMsg:
        return 2;
    case QtCriticalMsg:
        return 3;
    case QtFatalMsg:
        return 4;
    default:
        return 0;
    }
}

// single-letter severity tag used in log lines
static const char* levelTag(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return "D";
    case QtInfoMsg:
        return "I";
    case QtWarningMsg:
        return "W";
    case QtCriticalMsg:
        return "C";
    case QtFatalMsg:
        return "F";
    default:
        return "U";
    }
}

// installed via qInstallMessageHandler, implementation detail of this module
static void messageHandler(QtMsgType type,
                           const QMessageLogContext& context,
                           const QString& msg) {
    if (s_logFile == nullptr || severity(type) < severity(s_logLevel)) {
        return;
    }

    const QByteArray timeStr = QDateTime::currentDateTime()
        .toString("yyyy-MM-dd hh:mm:ss.zzz")
        .toLocal8Bit();
    const QByteArray localMsg = msg.toLocal8Bit();

    // source context is null in release builds, hide it there
    if (context.file != nullptr && context.function != nullptr) {
        fprintf(s_logFile, "%s [%s] %s (%s:%u, %s)\n",
                timeStr.constData(), levelTag(type), localMsg.constData(),
                context.file, context.line, context.function);
    }
    else {
        fprintf(s_logFile, "%s [%s] %s\n",
                timeStr.constData(), levelTag(type), localMsg.constData());
    }
    fflush(s_logFile);
}

static void setLogLevel(QtMsgType type) {
    s_logLevel = type;
    if (s_logFile == nullptr) {
        QString tempPath = QDir::tempPath() + QString("/Launchy");
        QDir tempDir(tempPath);
        if (!tempDir.exists()) {
            tempDir.mkpath(".");
        }
        QString logFileName = tempPath + QString("/launchy.log");
        s_logFile = fopen(logFileName.toUtf8().constData(), "w");
        if (s_logFile) {
            qInstallMessageHandler(messageHandler);
        }
    }
}

void stopLogging() {
    qInstallMessageHandler(nullptr);
    if (s_logFile) {
        fflush(s_logFile);
        fclose(s_logFile);
        s_logFile = nullptr;
    }
}

void setDebugLogEnabled(bool enabled) {
    if (enabled) {
        setLogLevel(QtDebugMsg);
    }
    else {
        setLogLevel(QtWarningMsg);
    }
}

} // namespace log
} // namespace launchy
