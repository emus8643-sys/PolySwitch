// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
class SettingsStore;
class BackgroundChooser:public QObject {
    Q_OBJECT
public:
    explicit BackgroundChooser(SettingsStore *store,QObject *parent=nullptr);
    bool active()const{return m_active;}
    void open();
Q_SIGNALS:
    void started();
    void finished(const QString &error);
private Q_SLOTS:
    void response(uint code,const QVariantMap &results);
private:
    void complete(const QString &error);
    SettingsStore *m_store;
    QString m_request;
    bool m_active=false;
    QTimer m_timeout;
};
