/*
LaunchyQt: release tag parsing unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QString>

#include "VersionTag.h"

// The update notification compares one number with LAUNCHY_VERSION, so the tag
// has to be encoded exactly the way that macro is built. The header lives in
// the app directory: it is include-only (macros plus a constexpr), which is why
// a headless test can use it as the reference for the encoding.
#include "Launchy/LaunchyVersion.h"

using namespace launchy;

class TestVersionTag : public QObject {
    Q_OBJECT

private slots:
    // --- accepted forms ---
    void tagWithPrefixIsEncoded();
    void tagWithoutPrefixIsEncoded();
    void uppercasePrefixIsAccepted();
    void surroundingWhitespaceIsIgnored();

    // --- rejected forms: "0" means "no version here", not "older" ---
    void twoComponentTagIsRejected();
    void fourComponentTagIsRejected();
    void nonNumericComponentIsRejected();
    void prereleaseSuffixIsRejected();
    void emptyAndGarbageAreRejected();

    // --- the encoding itself ---
    void currentVersionRoundTrips();
    void patchOutranksThePrefixOfTheNextMinor();
};

void TestVersionTag::tagWithPrefixIsEncoded() {
    QCOMPARE(versionFromTag("v3.1.8"), 30108u);
}

void TestVersionTag::tagWithoutPrefixIsEncoded() {
    // The feed is supposed to use "v3.1.8", but a plain tag must not be
    // silently discarded: that would look like "no release found".
    QCOMPARE(versionFromTag("3.1.8"), 30108u);
}

void TestVersionTag::uppercasePrefixIsAccepted() {
    QCOMPARE(versionFromTag("V3.1.8"), 30108u);
}

void TestVersionTag::surroundingWhitespaceIsIgnored() {
    QCOMPARE(versionFromTag("  v3.1.8\n"), 30108u);
}

void TestVersionTag::twoComponentTagIsRejected() {
    // "3.1" would otherwise encode as 30100 and compare as older than 3.1.9,
    // hiding a real release behind a silently wrong number.
    QCOMPARE(versionFromTag("v3.1"), 0u);
}

void TestVersionTag::fourComponentTagIsRejected() {
    QCOMPARE(versionFromTag("v3.1.9.1"), 0u);
}

void TestVersionTag::nonNumericComponentIsRejected() {
    QCOMPARE(versionFromTag("v3.1.x"), 0u);
    QCOMPARE(versionFromTag("vx.1.9"), 0u);
    // A single non-numeric component must not leave the others half-parsed
    QCOMPARE(versionFromTag("v3..9"), 0u);
}

void TestVersionTag::prereleaseSuffixIsRejected() {
    // "9-rc1" fails the numeric conversion; the release feed never has to
    // understand a pre-release, it just must not treat it as a version.
    QCOMPARE(versionFromTag("v3.1.9-rc1"), 0u);
}

void TestVersionTag::emptyAndGarbageAreRejected() {
    QCOMPARE(versionFromTag(QString()), 0u);
    QCOMPARE(versionFromTag("v"), 0u);
    QCOMPARE(versionFromTag("release"), 0u);
    QCOMPARE(versionFromTag("v.3.1"), 0u);
}

// The invariant that ties this file to LaunchyVersion.h: bumping the macro must
// keep the two encodings identical, or every user is told there is an update.
void TestVersionTag::currentVersionRoundTrips() {
    QCOMPARE(versionFromTag(LAUNCHY_VERSION_STRING), LAUNCHY_VERSION);
    QCOMPARE(versionFromTag(QString("v") + LAUNCHY_VERSION_STRING),
             LAUNCHY_VERSION);
}

// Two digits per component is what makes a plain integer comparison correct:
// 3.1.10 has to rank above 3.1.9, and 3.2.0 above 3.1.99.
void TestVersionTag::patchOutranksThePrefixOfTheNextMinor() {
    QVERIFY(versionFromTag("v3.1.10") > versionFromTag("v3.1.9"));
    QVERIFY(versionFromTag("v3.2.0") > versionFromTag("v3.1.99"));
    QVERIFY(versionFromTag("v4.0.0") > versionFromTag("v3.99.99"));
}

QTEST_APPLESS_MAIN(TestVersionTag)

#include "TestVersionTag.moc"
