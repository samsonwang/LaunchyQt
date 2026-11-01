/*
LaunchyQt: InputData unit tests (boundary / const-overload coverage)

TestCatalogItem.cpp already covers the basic InputData label/text/top-result
flow. This file exercises the remaining edges: the const getText/getTopResult
overloads, the string constructor, and top-result replacement semantics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include "PluginInterface/InputData.h"

using namespace launchy;

class TestInputData : public QObject {
    Q_OBJECT

private slots:
    void stringConstructor();
    void constGetText();
    void constGetTopResult();
    void topResultReplaceable();
    void topResultDefaultEmpty();
    void labelIdempotent();
    void removeMissingLabel();
};

void TestInputData::stringConstructor() {
    InputData data("google");
    QVERIFY(data.hasText());
    QCOMPARE(data.getText(), QString("google"));
}

void TestInputData::constGetText() {
    const InputData data("const text");
    // const getText() returns the same value without mutation.
    QCOMPARE(data.getText(), QString("const text"));
    QVERIFY(data.hasText());
}

void TestInputData::constGetTopResult() {
    CatItem item("C:/a.exe", QString("A"));
    InputData data;
    data.setTopResult(item);

    const InputData& cdata = data;
    QCOMPARE(cdata.getTopResult().fullPath, QString("C:/a.exe"));
    QCOMPARE(cdata.getTopResult().shortName, QString("A"));
}

void TestInputData::topResultReplaceable() {
    InputData data;
    CatItem first("C:/a.exe", QString("A"));
    CatItem second("C:/b.exe", QString("B"));

    data.setTopResult(first);
    QCOMPARE(data.getTopResult().shortName, QString("A"));

    // A later setTopResult replaces (not appends) the previous one.
    data.setTopResult(second);
    QCOMPARE(data.getTopResult().shortName, QString("B"));
    QCOMPARE(data.getTopResult().fullPath, QString("C:/b.exe"));
}

void TestInputData::topResultDefaultEmpty() {
    InputData data;
    // A fresh InputData has an empty (default-constructed) top result.
    QVERIFY(data.getTopResult().fullPath.isEmpty());
    QVERIFY(data.getTopResult().shortName.isEmpty());
    QCOMPARE(data.getTopResult().usage, 0);
}

void TestInputData::labelIdempotent() {
    InputData data;
    data.setLabel("alpha");
    data.setLabel("alpha"); // setting the same label twice is a no-op
    QCOMPARE(data.getLabels().size(), 1);
    QVERIFY(data.hasLabel("alpha"));
}

void TestInputData::removeMissingLabel() {
    InputData data;
    data.setLabel("alpha");
    data.removeLabel("beta"); // removing a label that is not present is safe
    QCOMPARE(data.getLabels().size(), 1);
    QVERIFY(data.hasLabel("alpha"));
}

QTEST_APPLESS_MAIN(TestInputData)
#include "TestInputData.moc"
