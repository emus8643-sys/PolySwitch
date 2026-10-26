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
    void liveShapeMorphsRetargetAndSettle() {
        PrismGeometry geometry;geometry.setTaskIds({"a","b","c","d","e","f"});geometry.setAnimated(true);geometry.setDuration(180);
        const auto old=geometry.faces();geometry.setTaskIds({"a","b","c","d","e","f","g"});
        QVERIFY(geometry.morphing());QCOMPARE(geometry.faces().size(),7);
        QVERIFY((geometry.faces()[0].toMap()["center"].value<QVector3D>()-old[0].toMap()["center"].value<QVector3D>()).length()<0.01);
        QTest::qWait(70);const auto middle=geometry.faces();
        QVERIFY(middle[0].toMap()["center"]!=old[0].toMap()["center"]);
        // Close a middle task while a new window is still unfolding.
        geometry.setTaskIds({"a","c","d","e","f","g","h","i"});
        QVERIFY((geometry.faces()[1].toMap()["center"].value<QVector3D>()-middle[2].toMap()["center"].value<QVector3D>()).length()<0.01);
        QTRY_VERIFY_WITH_TIMEOUT(!geometry.morphing(),500);QCOMPARE(geometry.faces().size(),8);
        PrismGeometry target;target.setTaskCount(8);for(const auto &face:geometry.faces())QVERIFY(target.faces().contains(face));
        geometry.setTaskIds({"a","c","d"});QVERIFY(geometry.morphing());
        QTRY_VERIFY_WITH_TIMEOUT(!geometry.morphing(),500);QCOMPARE(geometry.faces().size(),3);
        geometry.setTaskIds({});QVERIFY(geometry.morphing());QTRY_VERIFY_WITH_TIMEOUT(!geometry.morphing(),500);QVERIFY(geometry.faces().isEmpty());
    }
    void closedMorphKeepsPlanarJoinedFaces() {
        PrismGeometry geometry;geometry.setTaskCount(6);geometry.setAnimated(true);geometry.setDuration(180);
        bool valid=true;int frames=0;
        connect(&geometry,&PrismGeometry::facesChanged,this,[&]{
            if(!geometry.morphing())return;++frames;QMap<QString,int> edges;
            for(const auto &entry:geometry.faces()){
                const auto face=entry.toMap();const auto points=face["vertices"].toList();if(points.size()<3)continue;
                const auto n=face["normal"].value<QVector3D>(),c=face["center"].value<QVector3D>();
                if(qAbs(n.length()-1)>0.0001)valid=false;
                for(int i=0;i<points.size();i++){
                    const auto a=points[i].value<QVector3D>(),b=points[(i+1)%points.size()].value<QVector3D>();if(qAbs(QVector3D::dotProduct(n,a-c))>0.02)valid=false;
                    auto key=[](const QVector3D &v){return QString::number(v.x(),'f',3)+","+QString::number(v.y(),'f',3)+","+QString::number(v.z(),'f',3);};
                    const auto aa=key(a),bb=key(b);if(aa==bb)continue;edges[aa<bb?aa+"/"+bb:bb+"/"+aa]++;
                }
            }
            for(int uses:edges)if(uses!=2)valid=false;
        });
        for(int count:{7,12,20,5}){geometry.setTaskCount(count);QTRY_VERIFY_WITH_TIMEOUT(!geometry.morphing(),600);}
        QVERIFY(frames>8);QVERIFY(valid);
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
