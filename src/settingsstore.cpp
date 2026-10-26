// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "settingsstore.h"
#include <KConfigGroup>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImage>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <cmath>
#include <algorithm>
QVariantMap SettingsStore::defaults(){
    return {{"borderColor","#ef334d"},{"hoverColor","#ff163b"},{"rimColor","#17131b"},
        {"showHeader",true},{"backgroundImage",QString()},{"backgroundEnabled",true},{"backgroundMode","fill"},{"backgroundOpacity",100.0},{"backgroundColor","#06090f"},{"rimPercent",3.5},{"edgePercent",0.8},
        {"glassFrames",true},{"frameGlowColor","#ff183d"},{"frameGlowStyle","neon"},{"frameGlowSpread",18.0},{"frameGlowStrength",55.0},{"glassSheen",16.0},{"glowStyle","neon"},{"glowEnabled",true},{"glowStrength",100.0},{"glowRadius",24.0},
        {"autoUpright",false},{"zoomPercent",100.0},{"rotationSpeed",1.0},{"inertiaEnabled",true},{"livePreviews",true},
        {"previewSize",640},{"hoverTitles",true},
        {"previewMode","fill"},{"faceColor","#11131a"},{"titleColor","#fff0f3"},
        {"titleBackgroundColor","#17131b"},{"titleSize",24.0},{"titleBold",true},
        {"showFaceTitle",true},{"titlePosition","bottom"},{"previewOpacity",100.0},
        {"titleBackgroundOpacity",90.0},{"previewPadding",0.0}};
}
SettingsStore::SettingsStore(QObject *parent):QObject(parent),m_config(KSharedConfig::openConfig(QStringLiteral("polyswitchrc"),KConfig::SimpleConfig)){
    m_saveTimer.setSingleShot(true);m_saveTimer.setInterval(250);
    connect(&m_saveTimer,&QTimer::timeout,this,[this]{flush();});reload();
}
SettingsStore::~SettingsStore(){flush();}
QVariant SettingsStore::normalized(const QString &key,const QVariant &value)const{
    const auto defs=defaults();if(!defs.contains(key))return {};
    if(key.endsWith("Color")){
        const QColor color(value.toString());return color.isValid()?QVariant(color.name(QColor::HexRgb)):QVariant();
    }
    if(defs[key].metaType().id()==QMetaType::Bool){
        if(value.metaType().id()!=QMetaType::Bool)return {};
        return value.toBool();
    }
    if(key=="backgroundImage"){
        const QString path=value.toString();const QUrl url(path);
        return path.isEmpty() || (url.isLocalFile() && QFileInfo(url.toLocalFile()).isAbsolute()) ? QVariant(path) : QVariant();
    }
    if(key=="backgroundMode")return QStringList{"fit","fill","stretch"}.contains(value.toString())?value:QVariant();
    if(key=="frameGlowStyle")return QStringList{"neon","soft","pulse","radiant"}.contains(value.toString())?value:QVariant();
    if(key=="glowStyle")return QStringList{"neon","sunlight","ice","pulse"}.contains(value.toString())?value:QVariant();
    if(key=="previewMode")return QStringList{"fit","fill","title"}.contains(value.toString())?value:QVariant();
    if(key=="titlePosition")return QStringList{"top","center","bottom"}.contains(value.toString())?value:QVariant();
    bool ok=false;const double number=value.toDouble(&ok);
    if(!ok||!std::isfinite(number))return {};
    if(key=="previewSize"){
        for(int size:{256,512,640,1024})if(number==size)return size;
        return {};
    }
    double low=0,high=100;
    if(key=="frameGlowSpread"){low=6;high=64;}
    else if(key=="glassSheen"){low=0;high=40;}
    else if(key=="zoomPercent"){low=50;high=200;}
    else if(key=="titleSize"){low=12;high=64;}
    else if(key=="previewPadding"){low=0;high=20;}
    else if(key=="rimPercent"){low=0.5;high=12;}
    else if(key=="edgePercent"){low=0.2;high=3;}
    else if(key=="glowRadius"){low=6;high=48;}
    else if(key=="rotationSpeed"){low=0.25;high=2.5;}
    return std::clamp(number,low,high);
}
void SettingsStore::reload(){
    if(m_dirty&&!flush())return;
    m_config->reparseConfiguration();KConfigGroup group(m_config,"General");QVariantMap next;
    const auto defs=defaults();
    for(auto it=defs.cbegin();it!=defs.cend();++it){
        auto value=normalized(it.key(),group.readEntry(it.key(),it.value()));
        next[it.key()]=value.isValid()?value:it.value();
    }
    if(next!=m_values){m_values=next;Q_EMIT valuesChanged();}
}
bool SettingsStore::setValue(const QString &key,const QVariant &value){
    const auto valid=normalized(key,value);if(!valid.isValid())return false;
    if(m_values.value(key)==valid)return true;
    m_values[key]=valid;m_dirty=true;m_saveTimer.start();Q_EMIT valuesChanged();return true;
}
void SettingsStore::resetDefaults(){m_values=defaults();m_dirty=true;Q_EMIT valuesChanged();flush();}
bool SettingsStore::flush(){
    m_saveTimer.stop();if(!m_dirty)return true;
    KConfigGroup group(m_config,"General");
    for(auto it=m_values.cbegin();it!=m_values.cend();++it)group.writeEntry(it.key(),it.value());
    const bool saved=m_config->sync();
    const QString message=saved?QString():QStringLiteral("Settings could not be saved. Check that your configuration folder is writable.");
    if(message!=m_error){m_error=message;Q_EMIT errorChanged();}
    if(saved)m_dirty=false;return saved;
}

QVariantMap SettingsStore::backgroundFolder(const QUrl &folder) const {
    QString path=folder.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::PicturesLocation):(folder.scheme().isEmpty()?folder.toString():folder.toLocalFile());
    if(path.isEmpty() || !QFileInfo(path).isDir())path=QDir::homePath();
    QDir dir(path);QVariantList entries;
    const auto formats=QImageReader::supportedImageFormats();
    for(const auto &file:dir.entryInfoList(QDir::AllDirs|QDir::Files|QDir::NoDotAndDotDot|QDir::Readable,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase)){
        if(!file.isDir() && !formats.contains(file.suffix().toLower().toLatin1()))continue;
        entries.append(QVariantMap{{"name",file.fileName()},{"url",QUrl::fromLocalFile(file.absoluteFilePath())},{"directory",file.isDir()}});
    }
    QDir parent=dir;parent.cdUp();
    return {{"folder",QUrl::fromLocalFile(dir.absolutePath())},{"path",dir.absolutePath()},{"parent",QUrl::fromLocalFile(parent.absolutePath())},{"entries",entries}};
}
bool SettingsStore::importBackground(const QUrl &source){
    auto fail=[this](const QString &message){m_backgroundError=message;Q_EMIT backgroundErrorChanged();return false;};
    const QFileInfo file(source.toLocalFile());
    if(!source.isLocalFile() || !file.isFile() || !file.isReadable() || file.size()>64*1024*1024)
        return fail(QStringLiteral("Choose a readable local image smaller than 64 MB."));
    QImageReader reader(file.absoluteFilePath());reader.setAutoTransform(true);
    const QSize size=reader.size();
    if(!size.isValid() || qint64(size.width())*size.height()>32000000)
        return fail(QStringLiteral("Unsupported image or image dimensions too large."));
    if(size.width()>4096 || size.height()>4096)reader.setScaledSize(size.scaled(4096,4096,Qt::KeepAspectRatio));
    QImage image=reader.read();
    if(image.isNull())return fail(QStringLiteral("This image could not be decoded."));
    const QString dir=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+QStringLiteral("/polyswitch/backgrounds");
    if(!QDir().mkpath(dir))return fail(QStringLiteral("Could not create the background storage folder."));
    QFile::setPermissions(dir,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
    const QString target=dir+QStringLiteral("/background-")+QUuid::createUuid().toString(QUuid::WithoutBraces)+QStringLiteral(".png");
    QSaveFile output(target);
    if(!output.open(QIODevice::WriteOnly) || !image.save(&output,"PNG") || !output.commit())
        return fail(QStringLiteral("Could not save the imported background."));
    QFile::setPermissions(target,QFile::ReadOwner|QFile::WriteOwner);
    const QString old=m_values.value("backgroundImage").toString();
    setValue("backgroundImage",QUrl::fromLocalFile(target).toString());setValue("backgroundEnabled",true);
    if(!flush()){
        setValue("backgroundImage",old);QFile::remove(target);
        return fail(QStringLiteral("Could not save background preferences."));
    }
    // Only remove our own previous imported copy after the new preference is saved.
    const QString previous=QUrl(old).toLocalFile();const QFileInfo oldFile(previous);
    if(oldFile.absolutePath()==QDir(dir).absolutePath() && oldFile.fileName().startsWith("background-") && previous!=target)QFile::remove(previous);
    m_backgroundError.clear();Q_EMIT backgroundErrorChanged();return true;
}
