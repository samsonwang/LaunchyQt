/*
LaunchyQt: PluginInfo unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include "PluginInfo.h"

#include "PluginInterface/PluginInterface.h"
#include "PluginInterface/PluginMsg.h"

#include <stdexcept>

using namespace launchy;

namespace {

// A minimal, concrete PluginInterface for exercising PluginInfo::sendMsg().
// It records the last message it saw and can be told to throw.
class MockPlugin : public PluginInterface {
public:
    MockPlugin()
        : lastMsgId(-1),
          lastWParam(nullptr),
          lastLParam(nullptr),
          result(0),
          shouldThrow(false) {
    }

    int msg(int msgId, void* wParam, void* lParam) override {
        lastMsgId = msgId;
        lastWParam = wParam;
        lastLParam = lParam;
        if (shouldThrow) {
            throw std::runtime_error("mock failure");
        }
        return result;
    }

    int lastMsgId;
    void* lastWParam;
    void* lastLParam;
    int result;
    bool shouldThrow;
};

} // namespace

class TestPluginInfo : public QObject {
    Q_OBJECT

private slots:
    void defaultConstructor();
    void isValidEmpty();
    void isValidMissingObject();
    void isValidMissingName();
    void isValidComplete();
    void sendMsgForwardsArguments();
    void sendMsgReturnsPluginResult();
    void sendMsgCatchesException();
};

void TestPluginInfo::defaultConstructor() {
    PluginInfo info;
    QCOMPARE(info.name, QString());
    QCOMPARE(info.path, QString());
    QVERIFY(!info.loaded);
    QVERIFY(info.obj == nullptr);
    QVERIFY(!info.isValid());
}

void TestPluginInfo::isValidEmpty() {
    PluginInfo info;
    QVERIFY(!info.isValid());
}

void TestPluginInfo::isValidMissingObject() {
    PluginInfo info;
    info.name = "webby";
    // name set but no object -> still invalid
    QVERIFY(!info.isValid());
}

void TestPluginInfo::isValidMissingName() {
    PluginInfo info;
    MockPlugin plugin;
    info.obj = &plugin;
    // object set but name empty -> invalid
    QVERIFY(!info.isValid());
}

void TestPluginInfo::isValidComplete() {
    PluginInfo info;
    MockPlugin plugin;
    info.name = "webby";
    info.obj = &plugin;
    QVERIFY(info.isValid());
}

void TestPluginInfo::sendMsgForwardsArguments() {
    PluginInfo info;
    MockPlugin plugin;
    info.obj = &plugin;

    int arg = 123;
    info.sendMsg(MSG_INIT, &arg, nullptr);

    QCOMPARE(plugin.lastMsgId, static_cast<int>(MSG_INIT));
    QCOMPARE(plugin.lastWParam, static_cast<void*>(&arg));
    QVERIFY(plugin.lastLParam == nullptr);
}

void TestPluginInfo::sendMsgReturnsPluginResult() {
    PluginInfo info;
    MockPlugin plugin;
    plugin.result = 1;
    info.obj = &plugin;

    int ret = info.sendMsg(MSG_GET_NAME, nullptr, nullptr);
    QCOMPARE(ret, 1);

    plugin.result = 0;
    ret = info.sendMsg(MSG_GET_NAME, nullptr, nullptr);
    QCOMPARE(ret, 0);
}

void TestPluginInfo::sendMsgCatchesException() {
    PluginInfo info;
    MockPlugin plugin;
    plugin.shouldThrow = true;
    info.obj = &plugin;

    // A throwing plugin must not crash the host: the exception is swallowed
    // and the default return value (MSG_CONTROL_LAUNCHITEM) is returned.
    int ret = info.sendMsg(MSG_INIT, nullptr, nullptr);
    QCOMPARE(ret, static_cast<int>(MSG_CONTROL_LAUNCHITEM));
    QCOMPARE(plugin.lastMsgId, static_cast<int>(MSG_INIT));
}

QTEST_APPLESS_MAIN(TestPluginInfo)
#include "TestPluginInfo.moc"
