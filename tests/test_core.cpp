// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuaternion>
#include <QTemporaryDir>
#include <QImage>
#include <QImageReader>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
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
    void kdeThemeWatchingModesAndOverrides() {
        auto theme=[&](bool dark,const QString &accent,const QString &name){
            QSaveFile file(config.path()+"/kdeglobals");QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(("[General]\nColorScheme="+name+"\nAccentColor="+accent+"\n[Colors:Window]\nBackgroundNormal="+(dark?"24,24,28":"245,245,247")+"\nForegroundNormal="+(dark?"245,245,245":"24,24,24")+"\n[Colors:View]\nBackgroundNormal="+(dark?"16,16,19":"255,255,255")+"\n").toUtf8());QVERIFY(file.commit());
        };
        theme(false,"10,90,180","TestLight");SettingsStore settings;settings.resetDefaults();
        QVERIFY(settings.values()["followKdeTheme"].toBool());QCOMPARE(settings.values()["themeMode"].toString(),QStringLiteral("automatic"));
        QVERIFY(!settings.themeInfo()["dark"].toBool());QCOMPARE(settings.themeInfo()["schemeName"].toString(),QStringLiteral("TestLight"));QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#0a5ab4"));
        QVERIFY(settings.values()["frameGlowColor"]!=settings.values()["borderColor"]);
        settings.setValue("borderColor",QStringLiteral("#112233"));QCOMPARE(settings.values()["themeMode"].toString(),QStringLiteral("mixed"));
        theme(true,"204,34,170","TestDark");QTRY_VERIFY(settings.themeInfo()["dark"].toBool());
        QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#112233"));QCOMPARE(settings.themeInfo()["accent"].toString(),QStringLiteral("#cc22aa"));
        settings.setValue("themeMode",QStringLiteral("automatic"));QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#cc22aa"));
        settings.setValue("themeMode",QStringLiteral("mixed"));QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#112233"));
        settings.setValue("followKdeTheme",false);QVERIFY(!settings.values()["followKdeTheme"].toBool());
        const auto manual=settings.values();theme(false,"20,150,100","ChangedWhileManual");QTRY_VERIFY(!settings.themeInfo()["dark"].toBool());QCOMPARE(settings.values(),manual);
        settings.setValue("followKdeTheme",true);QCOMPARE(settings.values()["themeMode"].toString(),QStringLiteral("mixed"));
        settings.clearThemeOverrides();QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#149664"));
        settings.setValue("headerOpacity",67);QCOMPARE(settings.values()["headerOpacity"].toInt(),67);QVERIFY(settings.values()["themeOverrides"].toStringList().contains("headerOpacity"));
        settings.setThemeOverride("borderColor",true);settings.setValue("borderColor",QStringLiteral("#445566"));QVERIFY(settings.flush());
        SettingsStore readback;QCOMPARE(readback.values()["borderColor"].toString(),QStringLiteral("#445566"));QCOMPARE(readback.values()["headerOpacity"].toInt(),67);
        readback.setThemeOverride("borderColor",false);QCOMPARE(readback.values()["borderColor"].toString(),QStringLiteral("#149664"));
        QSaveFile plasma(config.path()+"/plasmarc");QVERIFY(plasma.open(QIODevice::WriteOnly));plasma.write("[Theme]\nname=SeparatePlasmaStyle\n");QVERIFY(plasma.commit());QTRY_COMPARE(readback.themeInfo()["plasmaTheme"].toString(),QStringLiteral("SeparatePlasmaStyle"));
        QCOMPARE(readback.values()["borderColor"].toString(),QStringLiteral("#149664"));
    }
    void plasmaArtworkAndUprightPreferences() {
        const auto root=config.path()+"/data/plasma/desktoptheme/TestArtwork";
        QVERIFY(QDir().mkpath(root+"/opaque/dialogs"));
        auto write=[](const QString &path,const QByteArray &data){QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit();};
        QVERIFY(write(root+"/opaque/dialogs/background.svg","<svg xmlns=\"http://www.w3.org/2000/svg\"/>"));
        QVERIFY(write(root+"/colors","[Colors:Window]\nBackgroundNormal=12,12,14\nForegroundNormal=245,245,245\n[Colors:Selection]\nBackgroundNormal=100,20,200\n"));
        QVERIFY(write(config.path()+"/plasmarc","[Theme]\nname=TestArtwork\n"));
        SettingsStore settings;settings.resetDefaults();
        settings.setValue("plasmaArtworkVariant",QStringLiteral("opaque"));
        QCOMPARE(settings.themeInfo()["artwork"].toMap()["panel"].toString(),root+"/opaque/dialogs/background.svg");
        const auto applicationAccent=settings.values()["borderColor"];
        settings.setValue("themeColorSource",QStringLiteral("plasma"));
        QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#6414c8"));
        QVERIFY(settings.themeInfo()["dark"].toBool());
        settings.setValue("autoUprightDelay",500);QCOMPARE(settings.values()["autoUprightDelay"].toDouble(),120.0);
        settings.setValue("autoUprightDuration",2);QCOMPARE(settings.values()["autoUprightDuration"].toDouble(),150.0);
        settings.setValue("autoUprightDelay",2.7);settings.setValue("autoUprightDuration",950.0);QVERIFY(settings.flush());
        SettingsStore readback;QCOMPARE(readback.values()["borderColor"].toString(),QStringLiteral("#6414c8"));
        QCOMPARE(readback.values()["autoUprightDelay"].toDouble(),2.7);QCOMPARE(readback.values()["autoUprightDuration"].toDouble(),950.0);
        QVERIFY(write(root+"/colors","[Colors:Selection]\nBackgroundNormal=10,100,30\n"));
        QTRY_COMPARE(readback.values()["borderColor"].toString(),QStringLiteral("#0a641e"));
        readback.resetDefaults();QCOMPARE(readback.values()["borderColor"],applicationAccent);
        QVERIFY(write(config.path()+"/plasmarc","[Theme]\nname=MissingStyle\n"));
        QTRY_VERIFY(readback.themeInfo()["plasmaArtworkFallback"].toBool());
        QVERIFY(!readback.themeInfo()["artwork"].toMap()["panel"].toString().startsWith(root));
    }
    void profilesPortableBackgroundRecoveryAndValidation() {
        SettingsStore settings;settings.resetDefaults();settings.setValue("onboardingCompleted",true);
        settings.setValue("themeMode",QStringLiteral("manual"));settings.setValue("borderColor",QStringLiteral("#123456"));
        settings.setValue("autoUprightDelay",7.0);
        QImage image(80,60,QImage::Format_RGB32);image.fill(Qt::blue);
        const auto original=config.path()+"/profile-original.png";QVERIFY(image.save(original));QVERIFY(settings.importBackground(QUrl::fromLocalFile(original)));
        const auto first=settings.savePreset("My blue look");QVERIFY(!first.isEmpty());
        const auto exported=config.path()+"/portable-profile.json";QVERIFY(settings.exportPreset(first,QUrl::fromLocalFile(exported)));
        QFile file(exported);QVERIFY(file.open(QIODevice::ReadOnly));const auto bytes=file.readAll();QVERIFY(!bytes.contains(config.path().toUtf8()));QVERIFY(!bytes.contains("backgroundImage"));
        QVERIFY(QFile::remove(original));image.fill(Qt::green);QVERIFY(image.save(original));QVERIFY(settings.importBackground(QUrl::fromLocalFile(original)));
        settings.setValue("borderColor",QStringLiteral("#abcdef"));settings.setValue("autoUprightDelay",11.0);
        const auto before=settings.values();const auto beforeCount=settings.presets().size();
        QVERIFY(settings.importPreset(QUrl::fromLocalFile(exported)));QCOMPARE(settings.values(),before);QCOMPARE(settings.presets().size(),beforeCount+1);
        QVERIFY(settings.applyPreset(first));QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#123456"));
        QCOMPARE(settings.values()["autoUprightDelay"].toDouble(),11.0);QVERIFY(settings.values()["onboardingCompleted"].toBool());
        QImage restored(QUrl(settings.values()["backgroundImage"].toString()).toLocalFile());QCOMPARE(restored.pixelColor(0,0),QColor(Qt::blue));
        QVERIFY(settings.canUndoAppearance());QVERIFY(settings.undoAppearance());QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#abcdef"));
        QImage undone(QUrl(settings.values()["backgroundImage"].toString()).toLocalFile());QCOMPARE(undone.pixelColor(0,0),QColor(Qt::green));
        QVERIFY(!settings.canUndoAppearance());QVERIFY(settings.resetAppearance());QVERIFY(settings.values()["onboardingCompleted"].toBool());QCOMPARE(settings.values()["autoUprightDelay"].toDouble(),11.0);
        QVERIFY(settings.undoAppearance());QCOMPARE(settings.values()["borderColor"].toString(),QStringLiteral("#abcdef"));
        const auto duplicate=settings.savePreset("My blue look");QVERIFY(!duplicate.isEmpty());QVERIFY(first!=duplicate);QVERIFY(settings.deletePreset(duplicate));
        QVERIFY(!settings.deletePreset("../../polyswitchrc"));QVERIFY(settings.savePreset("   ").isEmpty());
        const auto stable=settings.values();const auto stableCount=settings.presets().size();
        auto document=QJsonDocument::fromJson(bytes).object();auto entries=document["settings"].toObject();entries["onboardingCompleted"]=false;document["settings"]=entries;
        const auto invalid=config.path()+"/invalid-profile.json";
        auto write=[&](const QJsonObject &object){QSaveFile out(invalid);QVERIFY(out.open(QIODevice::WriteOnly));out.write(QJsonDocument(object).toJson());QVERIFY(out.commit());};
        write(document);QVERIFY(!settings.importPreset(QUrl::fromLocalFile(invalid)));QCOMPARE(settings.values(),stable);QCOMPARE(settings.presets().size(),stableCount);
        document=QJsonDocument::fromJson(bytes).object();document["backgroundPng"]="broken!!!";write(document);QVERIFY(!settings.importPreset(QUrl::fromLocalFile(invalid)));
        document=QJsonDocument::fromJson(bytes).object();document["version"]=999;write(document);QVERIFY(!settings.importPreset(QUrl::fromLocalFile(invalid)));
        QCOMPARE(settings.values(),stable);QCOMPARE(settings.presets().size(),stableCount);QVERIFY(settings.flush());
        SettingsStore readback;QCOMPARE(readback.presets().size(),stableCount);QVERIFY(readback.values()["onboardingCompleted"].toBool());
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
