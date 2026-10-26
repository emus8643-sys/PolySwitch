// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuaternion>
#include <QTemporaryDir>
#include <QImage>
#include <QImageReader>
#include "prism.h"
#include "settingsstore.h"

class CoreTests : public QObject {
    Q_OBJECT
    QTemporaryDir config;
private Q_SLOTS:
    void initTestCase() {
        QVERIFY(config.isValid());
        qputenv("XDG_CONFIG_HOME",config.path().toUtf8());
        qputenv("XDG_DATA_HOME",(config.path()+"/data").toUtf8());
    }
    void geometryCounts() {
        for(int count=0;count<=64;++count) {
            PrismGeometry geometry;geometry.setTaskCount(count);
            QCOMPARE(geometry.faces().size(),count);
            for(const auto &item:geometry.faces()) {
                const auto face=item.toMap();
                QVERIFY(face["vertices"].toList().size()>=3);
                QVERIFY(qAbs(face["normal"].value<QVector3D>().length()-1)<0.0001);
            }
        }
    }
    void uprightTextureBasis() {
        QQmlEngine engine;QQmlComponent component(&engine);
        const auto module=QUrl::fromLocalFile(QStringLiteral(POLYSWITCH_SOURCE_DIR "/qml/ScreenRotation.js")).toString();
        component.setData(("import QtQuick\nimport \""+module+"\" as Motion\nQtObject { function upright(n) { return Motion.uprightFace(n); } }").toUtf8(),QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object,qPrintable(component.errorString()));
        int checked=0;
        for(int count=1;count<=64;++count) {
            PrismGeometry geometry;geometry.setTaskCount(count);
            for(const auto &item:geometry.faces()) {
                const auto n=item.toMap()["normal"].value<QVector3D>();
                const auto preferred=qAbs(n.y())<0.95f?QVector3D(0,1,0):QVector3D(0,0,-1);
                const auto up=(preferred-n*QVector3D::dotProduct(preferred,n)).normalized();
                QVariant result;
                QVERIFY(QMetaObject::invokeMethod(object.get(),"upright",Q_RETURN_ARG(QVariant,result),Q_ARG(QVariant,QVariant::fromValue(n))));
                const auto rotation=result.value<QQuaternion>();
                QVERIFY(qAbs(rotation.length()-1)<0.00001);
                QVERIFY(rotation.rotatedVector(n).z()>0.99999);
                QVERIFY(rotation.rotatedVector(up).y()>0.99999);
                ++checked;
            }
        }
        QCOMPARE(checked,2080);
    }
    void settingsValidationAndPersistence() {
        {
            SettingsStore settings;settings.resetDefaults();
            QVERIFY(!settings.setValue("autoUpright",QStringLiteral("true")));
            QVERIFY(!settings.setValue("frameGlowStyle",QStringLiteral("unknown")));
            QVERIFY(!settings.setValue("frameGlowColor",QStringLiteral("invalid-color")));
            QVERIFY(settings.setValue("frameGlowSpread",1000));
            QCOMPARE(settings.values()["frameGlowSpread"].toInt(),64);
            QVERIFY(settings.setValue("frameGlowStrength",-10));
            QCOMPARE(settings.values()["frameGlowStrength"].toInt(),0);
            QVERIFY(settings.setValue("frameGlowStyle",QStringLiteral("radiant")));
            QVERIFY(settings.setValue("frameGlowColor",QStringLiteral("#46bbff")));
            QVERIFY(settings.setValue("autoUpright",true));
            QVERIFY(settings.flush());
        }
        SettingsStore readback;
        QCOMPARE(readback.values()["frameGlowStyle"].toString(),QStringLiteral("radiant"));
        QCOMPARE(readback.values()["frameGlowColor"].toString(),QStringLiteral("#46bbff"));
        QVERIFY(readback.values()["autoUpright"].toBool());
        QVERIFY(QFile::exists(config.path()+"/polyswitchrc"));
    }
    void importedBackgroundSurvivesOriginalRemoval() {
        const QString original=config.path()+"/test background.png";
        QImage image(4200,2100,QImage::Format_RGB32);image.fill(QColor("#4477aa"));
        QVERIFY(image.save(original));
        QString imported;
        {
            SettingsStore settings;
            QVERIFY(!settings.importBackground(QUrl("https://example.com/image.png")));
            QVERIFY(!settings.setValue("backgroundImage",QStringLiteral("https://example.com/image.png")));
            QVERIFY(!settings.setValue("backgroundMode",QStringLiteral("invalid")));
            QVERIFY(settings.importBackground(QUrl::fromLocalFile(original)));
            imported=QUrl(settings.values()["backgroundImage"].toString()).toLocalFile();
            QVERIFY(imported!=original);
            QVERIFY(QFile::exists(imported));
            QCOMPARE(QImageReader(imported).size(),QSize(4096,2048));
            QVERIFY(settings.backgroundError().isEmpty());
            QVERIFY(settings.setValue("showHeader",false));
            QVERIFY(settings.setValue("backgroundMode",QStringLiteral("fit")));
            QVERIFY(settings.setValue("backgroundOpacity",64));
            QVERIFY(settings.flush());
        }
        QVERIFY(QFile::remove(original));
        SettingsStore readback;
        QVERIFY(!readback.values()["showHeader"].toBool());
        QCOMPARE(readback.values()["backgroundMode"].toString(),QStringLiteral("fit"));
        QCOMPARE(readback.values()["backgroundOpacity"].toInt(),64);
        QCOMPARE(QUrl(readback.values()["backgroundImage"].toString()).toLocalFile(),imported);
        QVERIFY(!QImage(imported).isNull());
        const auto entries=readback.backgroundFolder(QUrl::fromLocalFile(QFileInfo(imported).absolutePath()))["entries"].toList();
        QVERIFY(!entries.isEmpty());
    }
};
QTEST_MAIN(CoreTests)
#include "test_core.moc"
