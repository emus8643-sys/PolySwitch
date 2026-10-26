// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "backgroundchooser.h"
#include "settingsstore.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusInterface>
#include <QUuid>
namespace {const QString service=QStringLiteral("org.freedesktop.portal.Desktop");const QString requestInterface=QStringLiteral("org.freedesktop.portal.Request");}
BackgroundChooser::BackgroundChooser(SettingsStore *store,QObject *parent):QObject(parent),m_store(store){
    m_timeout.setSingleShot(true);m_timeout.setInterval(300000);
    connect(&m_timeout,&QTimer::timeout,this,[this]{QDBusInterface(service,m_request,requestInterface).asyncCall("Close");complete(QStringLiteral("File selection timed out. Please try again."));});
}
void BackgroundChooser::open(){start("background");}
void BackgroundChooser::importProfile(){start("import");}
void BackgroundChooser::exportProfile(const QString &id){start("export",id);}
void BackgroundChooser::start(const QString &mode,const QString &id){
    if(m_active)return;m_mode=mode;m_profileId=id;m_active=true;Q_EMIT started();
    auto bus=QDBusConnection::sessionBus();
    const QString token="polyswitch_"+QUuid::createUuid().toString(QUuid::Id128);
    QString sender=bus.baseService().mid(1);sender.replace('.','_');
    m_request="/org/freedesktop/portal/desktop/request/"+sender+"/"+token;
    bus.connect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));
    auto message=QDBusMessage::createMethodCall(service,"/org/freedesktop/portal/desktop","org.freedesktop.portal.FileChooser",mode=="export"?"SaveFile":"OpenFile");
    QVariantMap options{{"handle_token",token},{"multiple",false},{"directory",false},{"modal",false},{"accept_label",mode=="background"?"Use background":mode=="import"?"Import profile":"Export profile"}};
    if(mode=="export")options["current_name"]="PolySwitch-profile.json";
    message.setArguments({QString(),mode=="background"?QStringLiteral("Choose PolySwitch background image"):mode=="import"?QStringLiteral("Import PolySwitch appearance profile"):QStringLiteral("Export PolySwitch appearance profile"),options});
    auto *watcher=new QDBusPendingCallWatcher(bus.asyncCall(message),this);
    connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher]{
        QDBusPendingReply<QDBusObjectPath> reply=*watcher;watcher->deleteLater();if(!m_active)return;
        if(reply.isError()){complete(QStringLiteral("The system file picker is unavailable. Check that the KDE desktop portal is installed and running."));return;}
        const QString actual=reply.value().path();if(actual!=m_request){auto bus=QDBusConnection::sessionBus();bus.disconnect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));m_request=actual;bus.connect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));}
    });m_timeout.start();
}
void BackgroundChooser::response(uint code,const QVariantMap &results){
    if(!m_active)return;
    if(code!=0){complete(QString());return;}
    const auto uris=results.value("uris").toStringList();
    if(uris.isEmpty()){complete(QStringLiteral("No file was selected."));return;}
    const auto url=QUrl(uris.first());
    const bool success=m_mode=="background"?m_store->importBackground(url):m_mode=="import"?m_store->importPreset(url):m_store->exportPreset(m_profileId,url);
    complete(success?QString():m_mode=="background"?m_store->backgroundError():m_store->presetError());
}
void BackgroundChooser::complete(const QString &error){
    if(!m_active)return;m_active=false;m_timeout.stop();QDBusConnection::sessionBus().disconnect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));m_request.clear();Q_EMIT finished(error);
}
