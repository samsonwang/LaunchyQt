/*
LaunchyQt: CatalogItem / InputData unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include "LaunchyLib/CatalogItem.h"
#include "LaunchyLib/InputData.h"

using namespace launchy;

class TestCatalogItem : public QObject {
    Q_OBJECT

private slots:
    // --- CatItem constructors ---
    void defaultConstructor();
    void fullPathConstructor();
    void fullPathConstructorDir();
    void fullPathConstructorLeadingDot();
    void fullPathConstructorNoExtension();
    void twoArgConstructor();
    void fourArgConstructor();

    // --- CatItem::convertSearchName ---
    void convertSearchNameLowercase();
    void convertSearchNameChinese();

    // --- CatItem comparison ---
    void equality();
    void inequality();

    // --- CatItem JSON round-trip ---
    void jsonRoundTrip();
    void jsonFromMissingFields();
    void jsonFromEmptyPath();
    void jsonNegativeUsage();

    // --- InputData ---
    void inputDataDefaults();
    void inputDataText();
    void inputDataLabels();
    void inputDataTopResult();
};

void TestCatalogItem::defaultConstructor() {
    CatItem item;
    QVERIFY(item.fullPath.isEmpty());
    QVERIFY(item.shortName.isEmpty());
    QVERIFY(item.searchName.isEmpty());
    QCOMPARE(item.usage, 0);
    QVERIFY(item.pluginName.isEmpty());
}

void TestCatalogItem::fullPathConstructor() {
    CatItem item("C:/Windows/notepad.exe");
    QCOMPARE(item.fullPath, QString("C:/Windows/notepad.exe"));
    // shortName strips the path and the extension
    QCOMPARE(item.shortName, QString("notepad"));
    QCOMPARE(item.searchName, QString("notepad"));
}

void TestCatalogItem::fullPathConstructorDir() {
    // A directory keeps its full name (no extension stripping)
    CatItem item("C:/Windows/System32", true);
    QCOMPARE(item.shortName, QString("System32"));
}

void TestCatalogItem::fullPathConstructorLeadingDot() {
    // A leading dot (".bashrc") keeps the whole name
    CatItem item("/home/user/.bashrc");
    QCOMPARE(item.shortName, QString(".bashrc"));
}

void TestCatalogItem::fullPathConstructorNoExtension() {
    // No extension -> the whole name is kept (lastIndexOf(".") == -1)
    CatItem item("/usr/bin/ls");
    QCOMPARE(item.shortName, QString("ls"));
}

void TestCatalogItem::twoArgConstructor() {
    // Note: the shortName argument must be a QString. A bare "Foo Bar" literal
    // would match CatItem(const QString&, bool) instead (const char* -> bool is
    // a valid conversion), so the constructor silently takes the isDir path.
    CatItem item("C:/Apps/Foo Bar.exe", QString("Foo Bar"));
    QCOMPARE(item.fullPath, QString("C:/Apps/Foo Bar.exe"));
    QCOMPARE(item.shortName, QString("Foo Bar"));
    // searchName is the lowered shortName
    QCOMPARE(item.searchName, QString("foo bar"));
    QCOMPARE(item.usage, 0);
}

void TestCatalogItem::fourArgConstructor() {
    CatItem item("C:/Apps/Firefox.exe", "Firefox", "webby", "C:/icons/firefox.png");
    QCOMPARE(item.fullPath, QString("C:/Apps/Firefox.exe"));
    QCOMPARE(item.shortName, QString("Firefox"));
    QCOMPARE(item.pluginName, QString("webby"));
    QCOMPARE(item.iconPath, QString("C:/icons/firefox.png"));
    QCOMPARE(item.searchName, QString("firefox"));
}

void TestCatalogItem::convertSearchNameLowercase() {
    QCOMPARE(CatItem::convertSearchName("HelloWorld"),
             QString("helloworld"));
}

void TestCatalogItem::convertSearchNameChinese() {
    // A simplified-Chinese character is mapped to its pinyin initial.
    // 中 (U+4E2D) -> "z"
    QCOMPARE(CatItem::convertSearchName(QString::fromUtf8("\u4e2d")),
             QString("z"));
}

void TestCatalogItem::equality() {
    CatItem a("C:/a.exe", QString("A"));
    CatItem b("C:/a.exe", QString("A"));
    CatItem c("C:/a.exe", QString("B"));
    QVERIFY(a == b);
    // equality requires BOTH fullPath and shortName to match
    QVERIFY(!(a == c));
}

void TestCatalogItem::inequality() {
    CatItem a("C:/a.exe", QString("A"));
    CatItem b("C:/b.exe", QString("A"));
    QVERIFY(a != b);
}

void TestCatalogItem::jsonRoundTrip() {
    CatItem original("C:/Apps/Firefox.exe", "Firefox", "webby", "C:/i/firefox.png");
    original.usage = 42;

    QJsonObject obj = catItemToJson(original);
    CatItem restored;
    QVERIFY(catItemFromJson(obj, restored));

    QCOMPARE(restored.fullPath, original.fullPath);
    QCOMPARE(restored.shortName, original.shortName);
    QCOMPARE(restored.iconPath, original.iconPath);
    QCOMPARE(restored.pluginName, original.pluginName);
    QCOMPARE(restored.usage, 42);
    // derived fields are recomputed from shortName
    QCOMPARE(restored.searchName, QString("firefox"));
}

void TestCatalogItem::jsonFromMissingFields() {
    QJsonObject obj; // no fullPath / shortName
    CatItem item;
    QVERIFY(!catItemFromJson(obj, item));
}

void TestCatalogItem::jsonFromEmptyPath() {
    QJsonObject obj;
    obj["fullPath"] = "";
    obj["shortName"] = "foo";
    CatItem item;
    QVERIFY(!catItemFromJson(obj, item));
}

void TestCatalogItem::jsonNegativeUsage() {
    // A demoted item has usage -1, and that must survive the round-trip
    QJsonObject obj;
    obj["fullPath"] = "C:/x.exe";
    obj["shortName"] = "x";
    obj["usage"] = -1;
    CatItem item;
    QVERIFY(catItemFromJson(obj, item));
    QCOMPARE(item.usage, -1);
}

void TestCatalogItem::inputDataDefaults() {
    InputData data;
    QVERIFY(!data.hasText());
    QVERIFY(data.getLabels().isEmpty());
}

void TestCatalogItem::inputDataText() {
    InputData data("hello world");
    QVERIFY(data.hasText());
    QCOMPARE(data.getText(), QString("hello world"));

    data.setText("new text");
    QCOMPARE(data.getText(), QString("new text"));
}

void TestCatalogItem::inputDataLabels() {
    InputData data;
    QVERIFY(!data.hasLabel("alpha"));

    data.setLabel("alpha");
    QVERIFY(data.hasLabel("alpha"));
    QVERIFY(!data.hasLabel("beta"));

    data.setLabel("beta");
    QCOMPARE(data.getLabels().size(), 2);

    data.removeLabel("alpha");
    QVERIFY(!data.hasLabel("alpha"));
    QVERIFY(data.hasLabel("beta"));

    data.clearLabel();
    QVERIFY(data.getLabels().isEmpty());
}

void TestCatalogItem::inputDataTopResult() {
    InputData data;
    CatItem top("C:/Apps/Firefox.exe", "Firefox", "webby", "C:/i/firefox.png");

    data.setTopResult(top);
    QCOMPARE(data.getTopResult().shortName, QString("Firefox"));
    QCOMPARE(data.getTopResult().pluginName, QString("webby"));
}

QTEST_APPLESS_MAIN(TestCatalogItem)
#include "TestCatalogItem.moc"
