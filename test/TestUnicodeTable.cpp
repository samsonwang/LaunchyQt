/*
LaunchyQt: UnicodeTable / convertSearchName unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include "PluginInterface/CatalogItem.h"

using namespace launchy;

// NOTE: the table and its bounds are private to UnicodeTable.cpp, and the one
// query that crosses that translation unit (launchy::zhCNInitial()) is not
// exported from Launchy.dll, so this test exercises the pinyin mapping strictly
// through the public CatItem::convertSearchName() API instead of peeking at the
// table internals.

class TestUnicodeTable : public QObject {
    Q_OBJECT

private slots:
    void lowercaseEnglish();
    void mixedCaseEnglish();
    void chinesePinyinInitial();
    void chineseFullName();
    void asciiUnchanged();
    void chineseRoundTripViaConstructor();
    void emptyString();
    void chineseLastMappedChar();
    void justPastTableEnd();
};

void TestUnicodeTable::lowercaseEnglish() {
    QCOMPARE(CatItem::convertSearchName("hello"),
             QString("hello"));
}

void TestUnicodeTable::mixedCaseEnglish() {
    QCOMPARE(CatItem::convertSearchName("HelloWorld"),
             QString("helloworld"));
}

void TestUnicodeTable::chinesePinyinInitial() {
    // 中 (U+4E2D) -> pinyin initial "z"
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u4e2d")),
             QString("z"));
    // 国 (U+56FD) -> "g"
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u56fd")),
             QString("g"));
}

void TestUnicodeTable::chineseFullName() {
    // 中国 -> "zg"
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u4e2d\u56fd")),
             QString("zg"));
}

void TestUnicodeTable::asciiUnchanged() {
    // ASCII letters, digits and punctuation are lowercased but otherwise kept.
    QCOMPARE(CatItem::convertSearchName("Firefox 3.6!"),
             QString("firefox 3.6!"));
}

void TestUnicodeTable::chineseRoundTripViaConstructor() {
    // A CatItem built from a Chinese shortName derives searchNameTrans from the
    // pinyin initials, while searchName keeps the lowered raw name.
    CatItem item("C:/Apps/中.exe", QString::fromUtf8("\u4e2d"));
    QCOMPARE(item.searchName, QString::fromUtf8("\u4e2d"));
    QCOMPARE(item.searchNameTrans, QString("z"));
}

void TestUnicodeTable::emptyString() {
    QCOMPARE(CatItem::convertSearchName(QString()), QString());
}

void TestUnicodeTable::chineseLastMappedChar() {
    // U+9FA5 closes the GB2312 Han block the table covers -> "y". This pins
    // the inclusive upper bound of the range check.
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u9fa5")),
             QString("y"));
}

void TestUnicodeTable::justPastTableEnd() {
    // One past the table: the character must come back untouched. While the
    // bound was min + strlen, this indexed the terminating NUL and silently
    // replaced the character with U+0000.
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u9fa6")),
             QString::fromUtf8("\u9fa6"));
}

QTEST_APPLESS_MAIN(TestUnicodeTable)
#include "TestUnicodeTable.moc"
