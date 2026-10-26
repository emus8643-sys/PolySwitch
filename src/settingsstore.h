// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <KSharedConfig>
#include <QUrl>
class SettingsStore:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString backgroundError READ backgroundError NOTIFY backgroundErrorChanged)
public:
    explicit SettingsStore(QObject *parent=nullptr);
    ~SettingsStore() override;
    QVariantMap values()const{return m_values;}
    QString error()const{return m_error;}
    QString backgroundError()const{return m_backgroundError;}
    Q_INVOKABLE QVariantMap backgroundFolder(const QUrl &folder = QUrl()) const;
    Q_INVOKABLE bool importBackground(const QUrl &source);
    Q_INVOKABLE bool setValue(const QString &key,const QVariant &value);
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE bool flush();
    void reload();
    static QVariantMap defaults();
Q_SIGNALS:
    void valuesChanged();
    void errorChanged();
    void backgroundErrorChanged();
private:
    QVariant normalized(const QString &key,const QVariant &value)const;
    KSharedConfig::Ptr m_config;
    QVariantMap m_values;
    QTimer m_saveTimer;
    QString m_error;
    QString m_backgroundError;
    bool m_dirty=false;
};
