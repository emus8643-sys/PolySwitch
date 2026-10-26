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
    connect(&m_timeout,&QTimer::timeout,this,[this]{QDBusInterface(service,m_request,requestInterface).asyncCall("Close");complete(QStringLiteral("Image selection timed out. Please try again."));});
}
void BackgroundChooser::open(){
    if(m_active)return;m_active=true;Q_EMIT started();
    auto bus=QDBusConnection::sessionBus();
    const QString token="polyswitch_"+QUuid::createUuid().toString(QUuid::Id128);
    QString sender=bus.baseService().mid(1);sender.replace('.','_');
    m_request="/org/freedesktop/portal/desktop/request/"+sender+"/"+token;
    bus.connect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));
    auto message=QDBusMessage::createMethodCall(service,"/org/freedesktop/portal/desktop","org.freedesktop.portal.FileChooser","OpenFile");
    message.setArguments({QString(),QStringLiteral("Choose PolySwitch background image"),QVariantMap{{"handle_token",token},{"multiple",false},{"directory",false},{"modal",false},{"accept_label",QStringLiteral("Use background")}}});
    auto *watcher=new QDBusPendingCallWatcher(bus.asyncCall(message),this);
    connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher]{
        QDBusPendingReply<QDBusObjectPath> reply=*watcher;watcher->deleteLater();if(!m_active)return;
        if(reply.isError()){complete(QStringLiteral("The system file picker is unavailable. You can use Browse inside PolySwitch instead."));return;}
        const QString actual=reply.value().path();if(actual!=m_request){auto bus=QDBusConnection::sessionBus();bus.disconnect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));m_request=actual;bus.connect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));}
    });m_timeout.start();
}
void BackgroundChooser::response(uint code,const QVariantMap &results){
    if(!m_active)return;
    if(code!=0){complete(QString());return;}
    const auto uris=results.value("uris").toStringList();
    if(uris.isEmpty() || !m_store->importBackground(QUrl(uris.first())))complete(m_store->backgroundError().isEmpty()?QStringLiteral("No usable image was selected."):m_store->backgroundError());
    else complete(QString());
}
void BackgroundChooser::complete(const QString &error){
    if(!m_active)return;m_active=false;m_timeout.stop();QDBusConnection::sessionBus().disconnect(service,m_request,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));m_request.clear();Q_EMIT finished(error);
}
