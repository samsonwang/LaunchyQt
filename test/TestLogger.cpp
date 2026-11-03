/*
LaunchyQt: file logger unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QFile>
#include <QString>

#include "Core/Logger.h"

#include "PluginInterface/Runtime.h"

using namespace launchy;

namespace {

// Logger owns exactly one file: %TEMP%/Launchy/launchy.log, the one the app
// writes when tracing is on. The test reads it back instead of asking the
// module for its state, because "the line reached the file" is the contract.
// It truncates that file on first use -- only a debug artefact, and the only
// way to exercise the real handler.
QString logFilePath() {
    return getAppTempPath(false) + QStringLiteral("/launchy.log");
}

QString logContent() {
    QFile file(logFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

bool logHasLine(const QString& tagAndText) {
    return logContent().contains(tagAndText);
}

} // namespace

class TestLogger : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // --- threshold: warnings and above at the default level ---
    void warningIsWritten();
    void criticalIsWritten();
    void debugIsDroppedAtTheWarningLevel();
    void infoIsDroppedAtTheWarningLevel();

    // --- threshold: everything once debug logging is on ---
    void debugIsWrittenWhenEnabled();
    void infoIsWrittenWhenEnabled();

    // --- shutdown ---
    void stopLoggingStopsWriting();
    void stopLoggingIsIdempotent();
};

void TestLogger::initTestCase() {
    // Opens the file and installs the handler at the default threshold
    log::setDebugLogEnabled(false);
    QVERIFY2(QFile::exists(logFilePath()), qPrintable(logFilePath()));
}

void TestLogger::warningIsWritten() {
    qWarning("%s", "logger-test-warning");
    QVERIFY(logHasLine("[W] logger-test-warning"));
}

void TestLogger::criticalIsWritten() {
    qCritical("%s", "logger-test-critical");
    QVERIFY(logHasLine("[C] logger-test-critical"));
}

// QtMsgType is not ordered by severity (QtInfoMsg was appended after
// QtFatalMsg), which is why Logger maps each type to a rank. This case and the
// next one fail if the comparison ever goes back to the raw enum values.
void TestLogger::debugIsDroppedAtTheWarningLevel() {
    qDebug("%s", "logger-test-debug-dropped");
    QVERIFY(!logHasLine("logger-test-debug-dropped"));
}

void TestLogger::infoIsDroppedAtTheWarningLevel() {
    qInfo("%s", "logger-test-info-dropped");
    QVERIFY(!logHasLine("logger-test-info-dropped"));
}

void TestLogger::debugIsWrittenWhenEnabled() {
    log::setDebugLogEnabled(true);

    qDebug("%s", "logger-test-debug-kept");
    QVERIFY(logHasLine("[D] logger-test-debug-kept"));
}

void TestLogger::infoIsWrittenWhenEnabled() {
    qInfo("%s", "logger-test-info-kept");
    QVERIFY(logHasLine("[I] logger-test-info-kept"));

    // Back to the default threshold for the shutdown cases
    log::setDebugLogEnabled(false);
}

void TestLogger::stopLoggingStopsWriting() {
    const QString contentBefore = logContent();

    log::stopLogging();

    qWarning("%s", "logger-test-after-stop");
    QVERIFY(!logHasLine("logger-test-after-stop"));
    // The file is closed, not truncated: what was already written stays
    QCOMPARE(logContent(), contentBefore);
}

void TestLogger::stopLoggingIsIdempotent() {
    // The application calls it from a quit handler and again from the destructor
    log::stopLogging();
    log::stopLogging();
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(TestLogger)

#include "TestLogger.moc"
