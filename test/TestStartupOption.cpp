/*
LaunchyQt: command line parsing unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QString>
#include <QStringList>

#include "Core/StartupOption.h"

using namespace launchy;

namespace {

// The parser skips the first element, so every case has to carry a program path
// the way main() does.
QStringList commandLine(const QString& first,
                        const QString& second = QString(),
                        const QString& third = QString()) {
    QStringList arguments;
    arguments << "Launchy.exe" << first;
    if (!second.isNull()) {
        arguments << second;
    }
    if (!third.isNull()) {
        arguments << third;
    }
    return arguments;
}

// Nothing but the program path: a launcher started from a plain shortcut.
void verifyNothingRequested(const StartupOption& option) {
    QVERIFY(!option.rescue);
    QVERIFY(!option.show);
    QVERIFY(!option.options);
    QVERIFY(!option.rescan);
    QVERIFY(!option.exit);
    QVERIFY(!option.debug);
    QVERIFY(!option.multiple);
    QVERIFY(option.profile.isEmpty());
}

} // namespace

class TestStartupOption : public QObject {
    Q_OBJECT

private slots:
    // --- the flags themselves ---
    void programPathOnlyRequestsNothing();
    void everyDocumentedFlagIsParsed();
    void rescueStaysAFlagAndIsNotExpandedHere();
    void flagsCanBeCombinedInAnyOrder();

    // --- -profile ---
    void profileTakesTheNextArgument();
    void profileAcceptsTheEqualsForm();
    void profileWithoutAValueStaysEmpty();

    // --- what is deliberately NOT an option ---
    void slashPrefixIsNotAnOption();
    void optionNamesAreCaseSensitive();

    // --- leniency: a shortcut must not be able to stop the launcher ---
    void unknownOptionKeepsTheFlagsBeforeIt();
};

void TestStartupOption::programPathOnlyRequestsNothing() {
    verifyNothingRequested(parseStartupOption(QStringList() << "Launchy.exe"));
}

void TestStartupOption::everyDocumentedFlagIsParsed() {
    QVERIFY(parseStartupOption(commandLine("-rescue")).rescue);
    QVERIFY(parseStartupOption(commandLine("-show")).show);
    QVERIFY(parseStartupOption(commandLine("-options")).options);
    QVERIFY(parseStartupOption(commandLine("-rescan")).rescan);
    QVERIFY(parseStartupOption(commandLine("-exit")).exit);
    QVERIFY(parseStartupOption(commandLine("-debug")).debug);
    QVERIFY(parseStartupOption(commandLine("-multiple")).multiple);

    // --name is the same option: the parser does not care how many dashes.
    QVERIFY(parseStartupOption(commandLine("--show")).show);
}

// -rescue means "reset the skin and the position, then show", but the expansion
// belongs to main(): the parser only reports that the word was there.
void TestStartupOption::rescueStaysAFlagAndIsNotExpandedHere() {
    const StartupOption option = parseStartupOption(commandLine("-rescue"));
    QVERIFY(option.rescue);
    QVERIFY(!option.show);
}

// The old hand-written loop assigned for -rescue and OR-ed everything else, so
// "-options -rescue" lost -options. Order must not matter any more.
void TestStartupOption::flagsCanBeCombinedInAnyOrder() {
    const StartupOption forward = parseStartupOption(commandLine("-rescue", "-options"));
    const StartupOption backward = parseStartupOption(commandLine("-options", "-rescue"));
    QVERIFY(forward.rescue && forward.options);
    QVERIFY(backward.rescue && backward.options);
    QCOMPARE(forward.rescan, backward.rescan);
}

void TestStartupOption::profileTakesTheNextArgument() {
    const StartupOption option = parseStartupOption(commandLine("-profile", "work"));
    QCOMPARE(option.profile, QString("work"));
    // The value is not a flag of its own, even if it looks like one.
    QVERIFY(!option.show);
}

void TestStartupOption::profileAcceptsTheEqualsForm() {
    QCOMPARE(parseStartupOption(commandLine("-profile=work")).profile, QString("work"));
}

// A trailing -profile with nothing behind it is "not specified" rather than a
// failure: the launcher then uses the default profile.
void TestStartupOption::profileWithoutAValueStaysEmpty() {
    verifyNothingRequested(parseStartupOption(commandLine("-profile")));
}

// The installer's shortcuts used "/show" and "/rescue"; a leading slash is not
// an option prefix, so it becomes a positional argument and is ignored.
void TestStartupOption::slashPrefixIsNotAnOption() {
    verifyNothingRequested(parseStartupOption(commandLine("/show")));
    verifyNothingRequested(parseStartupOption(commandLine("/rescue")));
}

void TestStartupOption::optionNamesAreCaseSensitive() {
    verifyNothingRequested(parseStartupOption(commandLine("-SHOW")));
    verifyNothingRequested(parseStartupOption(commandLine("-Rescue")));
}

// The error is only reported (qWarning) and parsing carries on: whatever was
// understood before the bad word still takes effect, so a typo in a shortcut
// cannot leave the user with a launcher that did nothing at all.
void TestStartupOption::unknownOptionKeepsTheFlagsBeforeIt() {
    const StartupOption option = parseStartupOption(commandLine("-show", "-bogus"));
    QVERIFY(option.show);
}

QTEST_APPLESS_MAIN(TestStartupOption)

#include "TestStartupOption.moc"
