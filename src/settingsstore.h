// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <KSharedConfig>
#include <QUrl>
#include <QFileSystemWatcher>
#include <KConfigWatcher>
class SettingsStore:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantMap themeInfo READ themeInfo NOTIFY themeInfoChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString backgroundError READ backgroundError NOTIFY backgroundErrorChanged)
    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(QString presetError READ presetError NOTIFY presetsChanged)
    Q_PROPERTY(bool canUndoAppearance READ canUndoAppearance NOTIFY presetsChanged)
public:
    QVariantList presets()const;
    QString presetError()const{return m_presetError;}
    bool canUndoAppearance()const;
    Q_INVOKABLE QString savePreset(const QString &name);
    Q_INVOKABLE bool applyPreset(const QString &id);
    Q_INVOKABLE bool deletePreset(const QString &id);
    Q_INVOKABLE bool exportPreset(const QString &id,const QUrl &target);
    Q_INVOKABLE bool importPreset(const QUrl &source);
    Q_INVOKABLE bool resetAppearance();
    Q_INVOKABLE bool undoAppearance();
    explicit SettingsStore(QObject *parent=nullptr);
    ~SettingsStore() override;
    QVariantMap values()const{return m_effective;}
    QVariantMap themeInfo()const{return m_themeInfo;}
    Q_INVOKABLE void setThemeOverride(const QString &key,bool enabled);
    Q_INVOKABLE void clearThemeOverrides();
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
    void presetsChanged();
    void valuesChanged();
    void themeInfoChanged();
    void errorChanged();
    void backgroundErrorChanged();
protected:
    bool eventFilter(QObject *object,QEvent *event) override;
private:
    QVariantMap appearance()const;
    bool applyAppearance(const QVariantMap &document,bool snapshot);
    bool writeAppearance(const QString &path,const QVariantMap &document);
    QVariantMap readAppearance(const QString &path)const;
    QString presetDirectory()const;
    QString presetPath(const QString &id)const;
    bool presetFailure(const QString &message);
    QString m_presetError;
    void refreshTheme();
    void updateEffective();
    void armThemeWatch();
    bool themedKey(const QString &key)const;
    QVariant normalized(const QString &key,const QVariant &value)const;
    KSharedConfig::Ptr m_config;
    QVariantMap m_values,m_effective,m_themeInfo,m_themeValues;
    KSharedConfig::Ptr m_kdeConfig,m_plasmaConfig;
    KConfigWatcher::Ptr m_kdeWatcher,m_plasmaWatcher;
    QFileSystemWatcher m_themeFiles;
    QTimer m_themeRefresh;
    QTimer m_saveTimer;
    QString m_error;
    QString m_backgroundError;
    bool m_dirty=false;
};
