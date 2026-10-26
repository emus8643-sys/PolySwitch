// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include <QtTest>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QImage>
#include <QTemporaryDir>
#include "backgroundchooser.h"
#include "settingsstore.h"
class Portal:public QObject,protected QDBusContext {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface","org.freedesktop.portal.FileChooser")
public:uint code=0;QString uri;
public Q_SLOTS:
 QDBusObjectPath OpenFile(const QString &,const QString &,const QVariantMap &options){
  QString sender=message().service().mid(1);sender.replace('.','_');const QString path="/org/freedesktop/portal/desktop/request/"+sender+"/"+options["handle_token"].toString();
  QTimer::singleShot(50,this,[this,path]{auto signal=QDBusMessage::createSignal(path,"org.freedesktop.portal.Request","Response");signal.setArguments({code,QVariantMap{{"uris",QStringList{uri}}}});QDBusConnection::sessionBus().send(signal);});return QDBusObjectPath(path);
 }
};
class PickerTests:public QObject {
 Q_OBJECT
 QTemporaryDir temp;
private Q_SLOTS:
 void nativePortalImportCancelAndFailure(){
  QVERIFY(temp.isValid());qputenv("XDG_CONFIG_HOME",(temp.path()+"/config").toUtf8());qputenv("XDG_DATA_HOME",(temp.path()+"/data").toUtf8());
  Portal portal;auto bus=QDBusConnection::sessionBus();QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));QVERIFY(bus.registerObject("/org/freedesktop/portal/desktop",&portal,QDBusConnection::ExportAllSlots));
  QImage image(80,60,QImage::Format_RGB32);image.fill(Qt::blue);const auto file=temp.path()+"/image.png";QVERIFY(image.save(file));portal.uri=QUrl::fromLocalFile(file).toString();
  SettingsStore settings;BackgroundChooser chooser(&settings);QSignalSpy started(&chooser,&BackgroundChooser::started),finished(&chooser,&BackgroundChooser::finished);
  chooser.open();chooser.open();QVERIFY(chooser.active());QCOMPARE(started.count(),1);QTRY_COMPARE(finished.count(),1);QVERIFY(finished[0][0].toString().isEmpty());QVERIFY(!chooser.active());QVERIFY(QFile::exists(QUrl(settings.values()["backgroundImage"].toString()).toLocalFile()));
  const auto saved=settings.values()["backgroundImage"];portal.code=1;chooser.open();QTRY_COMPARE(finished.count(),2);QCOMPARE(settings.values()["backgroundImage"],saved);
  portal.code=0;portal.uri="https://example.com/not-local.png";chooser.open();QTRY_COMPARE(finished.count(),3);QVERIFY(!finished[2][0].toString().isEmpty());QCOMPARE(settings.values()["backgroundImage"],saved);
  bus.unregisterObject("/org/freedesktop/portal/desktop");chooser.open();QTRY_COMPARE(finished.count(),4);QVERIFY(!finished[3][0].toString().isEmpty());QVERIFY(!chooser.active());
 }
};
QTEST_MAIN(PickerTests)
#include "test_backgroundchooser.moc"
