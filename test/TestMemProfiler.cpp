/*
LaunchyQt: memory profiler unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QByteArray>

#include "MemProfiler.h"

using namespace launchy;
using namespace launchy::memprof;

// The profiler is a switch plus a delta baseline over an OS counter, so the
// cases here are about the switch and the baseline. The absolute numbers are
// deliberately not asserted: they differ per machine and per allocator.
class TestMemProfiler : public QObject {
    Q_OBJECT

private slots:
    void cleanup();

    void recordIsANoOpWhenDisabled();
    void firstSampleAfterEnableHasNoBaseline();
    void reEnablingStartsANewBaseline();
    void secondSampleReportsTheGrowth();
    void scopedMemIsHarmlessWhenDisabled();
    void scopedMemRunsWhenEnabled();
};

void TestMemProfiler::cleanup() {
    // Leave the module in its default state for the next case
    setEnabled(false);
}

void TestMemProfiler::recordIsANoOpWhenDisabled() {
    setEnabled(false);

    QCOMPARE(record(QStringLiteral("disabled")), qlonglong(-1));
    // Still a no-op on the second call: -1 must not be mistaken for a delta
    QCOMPARE(record(QStringLiteral("disabled")), qlonglong(-1));
}

// The first sample only establishes the baseline, so there is nothing to
// subtract yet. Returning 0 instead would invent a delta of zero.
void TestMemProfiler::firstSampleAfterEnableHasNoBaseline() {
    setEnabled(true);
    QCOMPARE(record(QStringLiteral("first")), qlonglong(-1));
}

// setEnabled(true) resets the baseline: without that, the first delta after a
// long disabled period would report everything allocated in between as if it
// happened in that one stage.
void TestMemProfiler::reEnablingStartsANewBaseline() {
    setEnabled(true);
    QCOMPARE(record(QStringLiteral("baseline")), qlonglong(-1));

    setEnabled(false);
    setEnabled(true);

    QCOMPARE(record(QStringLiteral("after-reenable")), qlonglong(-1));
}

// The reason the profiler exists: a stage that allocates must report growth.
void TestMemProfiler::secondSampleReportsTheGrowth() {
    setEnabled(true);
    QCOMPARE(record(QStringLiteral("before-allocation")), qlonglong(-1));

    // Committed immediately, so Private Bytes grows with it
    QByteArray block(8 * 1024 * 1024, 'x');
    block[0] = 'y';
    block[block.size() - 1] = 'z';

    const qlonglong delta = record(QStringLiteral("after-allocation"));
    QVERIFY2(delta > 0, qPrintable(QString("delta=%1").arg(delta)));
}

// ScopedMem takes a sample in the constructor and its destructor. Both must be
// no-ops when sampling is off -- this runs on every catalog build.
void TestMemProfiler::scopedMemIsHarmlessWhenDisabled() {
    setEnabled(false);
    {
        ScopedMem scope(QStringLiteral("disabled-scope"));
    }
    QCOMPARE(record(QStringLiteral("still-disabled")), qlonglong(-1));
}

void TestMemProfiler::scopedMemRunsWhenEnabled() {
    setEnabled(true);
    {
        ScopedMem scope(QStringLiteral("enabled-scope"));
        QByteArray block(1024 * 1024, 'x');
    }
    // The destructor sampled the same counters the next record() uses
    QVERIFY(record(QStringLiteral("after-scope")) != -1);
}

QTEST_APPLESS_MAIN(TestMemProfiler)

#include "TestMemProfiler.moc"
