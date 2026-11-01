/*
LaunchyQt: InputDataList unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QStringList>

#include "InputDataList.h"

using namespace launchy;

namespace {

// The separator Launchy ships with: space, U+25BA, space
QString defaultSeparator() {
    return QString(" ") + QChar(0x25ba) + QString(" ");
}

} // namespace

// InputDataList is what turns the typed text into query segments. It reuses the
// existing entries instead of rebuilding the list on every keystroke, so the
// interesting cases are all about that reuse: shrinking, growing, and dropping
// the per-keystroke labels.
class TestInputDataList : public QObject {
    Q_OBJECT

private slots:
    void init();

    void parseSplitsOnTheSeparator();
    void parseKeepsTrailingEmptySegment();
    void parseReusesTheCommonPrefix();
    void parseTruncatesWhenTheQueryShrinks();
    void parseEmptyClearsTheList();
    void parseDropsLabelsOfRetainedSegments();

    void toStringRoundTrips();
    void toStringOmitLastDropsTheLastSegment();

    void setSeparatorIsHonoured();
};

void TestInputDataList::init() {
    // A static, so every case has to put it back
    InputDataList::setSeparator(defaultSeparator());
}

void TestInputDataList::parseSplitsOnTheSeparator() {
    InputDataList list;
    list.parse("google" + defaultSeparator() + "this is my search");

    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(0).getText(), QString("google"));
    QCOMPARE(list.at(1).getText(), QString("this is my search"));
}

// Tab on a plugin key appends the separator before there is anything behind it,
// and that has to yield a (still empty) segment: it is what the next keystroke
// fills in.
void TestInputDataList::parseKeepsTrailingEmptySegment() {
    InputDataList list;
    list.parse("google" + defaultSeparator());

    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(1).getText(), QString(""));
    QVERIFY(!list.at(1).hasText());
}

void TestInputDataList::parseReusesTheCommonPrefix() {
    InputDataList list;
    list.parse("a" + defaultSeparator() + "b");

    list[0].setTopResult(CatItem("C:/a.exe", "a"));
    const CatItem kept = list.at(0).getTopResult();

    list.parse("a" + defaultSeparator() + "c" + defaultSeparator() + "d");

    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(0).getText(), QString("a"));
    QCOMPARE(list.at(1).getText(), QString("c"));
    QCOMPARE(list.at(2).getText(), QString("d"));
    // The retained segment keeps its resolved result: that is the whole point
    // of parsing incrementally instead of rebuilding the list
    QCOMPARE(list.at(0).getTopResult().fullPath, kept.fullPath);
}

void TestInputDataList::parseTruncatesWhenTheQueryShrinks() {
    InputDataList list;
    list.parse("a" + defaultSeparator() + "b" + defaultSeparator() + "c");
    QCOMPARE(list.count(), 3);

    list.parse("a" + defaultSeparator() + "b");
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(1).getText(), QString("b"));

    // A different second segment replaces everything from there on
    list.parse("a" + defaultSeparator() + "z" + defaultSeparator() + "c");
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(1).getText(), QString("z"));
    QCOMPARE(list.at(2).getText(), QString("c"));
}

void TestInputDataList::parseEmptyClearsTheList() {
    InputDataList list;
    list.parse("a" + defaultSeparator() + "b");
    QCOMPARE(list.count(), 2);

    list.parse(QString());
    QCOMPARE(list.count(), 0);
}

// Labels are per-pass scratch space: plugins tag a segment in getLabels() and
// read the tag back in getResults(). parse() must drop them, otherwise a tag
// from the previous keystroke would be applied to text it never matched.
void TestInputDataList::parseDropsLabelsOfRetainedSegments() {
    InputDataList list;
    list.parse("web" + defaultSeparator() + "query");

    list[0].setLabel("websearch");
    QVERIFY(list[0].hasLabel("websearch"));

    list.parse("webx" + defaultSeparator() + "query");
    QVERIFY(!list[0].hasLabel("websearch"));
}

void TestInputDataList::toStringRoundTrips() {
    const QString text = "google" + defaultSeparator() + "this is my search";

    InputDataList list;
    list.parse(text);

    QCOMPARE(list.toString(), text);
}

void TestInputDataList::toStringOmitLastDropsTheLastSegment() {
    const QString separator = defaultSeparator();

    InputDataList list;
    list.parse("google" + separator + "query");

    // Used by Tab completion: complete the first segment and leave the rest
    QCOMPARE(list.toString(true), QString("google" + separator));
}

void TestInputDataList::setSeparatorIsHonoured() {
    InputDataList::setSeparator("|");

    InputDataList list;
    list.parse("a|b|c");
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(2).getText(), QString("c"));
    QCOMPARE(list.toString(), QString("a|b|c"));
}

QTEST_GUILESS_MAIN(TestInputDataList)
#include "TestInputDataList.moc"
