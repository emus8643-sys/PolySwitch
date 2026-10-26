// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "settingsstore.h"
#include <KConfigGroup>
#include <KConfigGui>
#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QEvent>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImage>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QTemporaryFile>
#include <cmath>
#include <algorithm>
QVariantMap SettingsStore::defaults(){
    return {{"onboardingCompleted",false},{"plasmaArtwork",false},{"plasmaArtworkShadows",true},{"plasmaArtworkVariant","automatic"},{"themeColorSource","application"},{"autoUprightDelay",1.0},{"autoUprightDuration",650.0},{"themeMode","automatic"},{"themeOverrides",QStringList()},{"themeGlowPalette","complementary"},
        {"uiPanelColor","#14131c"},{"uiBaseColor","#211c26"},{"uiTextColor","#fff0f3"},{"uiMutedColor","#bfb3bd"},{"uiButtonColor","#303137"},{"uiBorderColor","#656770"},{"uiErrorColor","#ff778e"},{"uiHighlightTextColor","#ffffff"},
        {"lightingColor","#ff5265"},{"fillLightColor","#d9dbe1"},{"lightBrightness",0.75},{"headerOpacity",80.0},{"panelOpacity",96.0},
        {"borderColor","#ef334d"},{"hoverColor","#ff163b"},{"rimColor","#17131b"},
        {"shapeMorphEnabled",true},{"shapeMorphDuration",650.0},{"showHeader",true},{"backgroundImage",QString()},{"backgroundEnabled",true},{"backgroundMode","fill"},{"backgroundOpacity",100.0},{"backgroundColor","#06090f"},{"rimPercent",3.5},{"edgePercent",0.8},
        {"glassFrames",true},{"frameGlowColor","#ff183d"},{"frameGlowStyle","neon"},{"frameGlowSpread",18.0},{"frameGlowStrength",55.0},{"glassSheen",16.0},{"glowStyle","neon"},{"glowEnabled",true},{"glowStrength",100.0},{"glowRadius",24.0},
        {"autoUpright",false},{"zoomPercent",100.0},{"rotationSpeed",1.0},{"inertiaEnabled",true},{"livePreviews",true},
        {"previewSize",640},{"hoverTitles",true},
        {"previewMode","fill"},{"faceColor","#11131a"},{"titleColor","#fff0f3"},
        {"titleBackgroundColor","#17131b"},{"titleSize",24.0},{"titleBold",true},
        {"showFaceTitle",true},{"titlePosition","bottom"},{"previewOpacity",100.0},
        {"titleBackgroundOpacity",90.0},{"previewPadding",0.0}};
}
SettingsStore::SettingsStore(QObject *parent):QObject(parent),m_config(KSharedConfig::openConfig(QStringLiteral("polyswitchrc"),KConfig::SimpleConfig)){
    // Keep KConfigGui linked even with --as-needed: it registers QColor readers.
    (void)KConfigGui::hasSessionConfig();
    m_saveTimer.setSingleShot(true);m_saveTimer.setInterval(250);
    connect(&m_saveTimer,&QTimer::timeout,this,[this]{flush();});
    m_kdeConfig=KSharedConfig::openConfig(QStringLiteral("kdeglobals"));m_plasmaConfig=KSharedConfig::openConfig(QStringLiteral("plasmarc"),KConfig::SimpleConfig);
    m_kdeWatcher=KConfigWatcher::create(m_kdeConfig);m_plasmaWatcher=KConfigWatcher::create(m_plasmaConfig);
    m_themeRefresh.setSingleShot(true);m_themeRefresh.setInterval(80);
    connect(&m_themeRefresh,&QTimer::timeout,this,[this]{refreshTheme();});
    connect(m_kdeWatcher.data(),&KConfigWatcher::configChanged,this,[this]{m_themeRefresh.start();});
    connect(m_plasmaWatcher.data(),&KConfigWatcher::configChanged,this,[this]{m_themeRefresh.start();});
    connect(&m_themeFiles,&QFileSystemWatcher::fileChanged,this,[this]{m_themeRefresh.start();});
    connect(&m_themeFiles,&QFileSystemWatcher::directoryChanged,this,[this]{m_themeRefresh.start();});
    if(qApp)qApp->installEventFilter(this);
    refreshTheme();reload();
}
SettingsStore::~SettingsStore(){flush();}
QVariant SettingsStore::normalized(const QString &key,const QVariant &value)const{
    const auto defs=defaults();if(!defs.contains(key))return {};
    if(key=="themeColorSource")return QStringList{"application","plasma"}.contains(value.toString())?value:QVariant();
    if(key=="plasmaArtworkVariant")return QStringList{"automatic","translucent","opaque","solid"}.contains(value.toString())?value:QVariant();
    if(key=="themeMode")return QStringList{"automatic","manual","mixed"}.contains(value.toString())?value:QVariant();
    if(key=="themeGlowPalette")return QStringList{"accent","complementary","analogous"}.contains(value.toString())?value:QVariant();
    if(key=="themeOverrides"){const auto keys=value.toStringList();for(const auto &k:keys)if(!themedKey(k))return {};return keys;}
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
    if(key=="autoUprightDelay"){low=0.2;high=120;}
    else if(key=="autoUprightDuration"){low=150;high=3000;}
    else if(key=="lightBrightness"){low=0.1;high=2;}
    else if(key=="shapeMorphDuration"){low=180;high=1200;}
    else if(key=="frameGlowSpread"){low=6;high=64;}
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
    if(next!=m_values){m_values=next;refreshTheme();}
}
bool SettingsStore::setValue(const QString &key,const QVariant &value){
    if(key=="followKdeTheme"){
        if(value.metaType().id()!=QMetaType::Bool)return false;
        return setValue("themeMode",value.toBool()?(m_values["themeOverrides"].toStringList().isEmpty()?"automatic":"mixed"):"manual");
    }
    const auto valid=normalized(key,value);if(!valid.isValid())return false;
    if(themedKey(key) && m_values["themeMode"].toString()!="manual"){
        auto overrides=m_values["themeOverrides"].toStringList();if(!overrides.contains(key))overrides.append(key);
        m_values["themeOverrides"]=overrides;m_values["themeMode"]="mixed";
    }else if(m_values.value(key)==valid)return true;
    m_values[key]=valid;m_dirty=true;m_saveTimer.start();if(key=="themeColorSource"||key=="plasmaArtworkVariant")refreshTheme();else updateEffective();return true;
}
void SettingsStore::setThemeOverride(const QString &key,bool enabled){
    if(!themedKey(key))return;
    auto keys=m_values["themeOverrides"].toStringList();
    if(enabled){if(!keys.contains(key)){m_values[key]=m_effective.value(key,m_values[key]);keys.append(key);}m_values["themeMode"]="mixed";}
    else keys.removeAll(key);
    m_values["themeOverrides"]=keys;m_dirty=true;m_saveTimer.start();updateEffective();
}
void SettingsStore::clearThemeOverrides(){m_values["themeOverrides"]=QStringList();m_dirty=true;m_saveTimer.start();updateEffective();}
void SettingsStore::resetDefaults(){m_values=defaults();m_dirty=true;refreshTheme();flush();}
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

namespace {
double luminance(const QColor &c){auto channel=[](double x){return x<=0.04045?x/12.92:std::pow((x+0.055)/1.055,2.4);};return 0.2126*channel(c.redF())+0.7152*channel(c.greenF())+0.0722*channel(c.blueF());}
QColor readable(const QColor &text,const QColor &background){const double a=luminance(text),b=luminance(background);if((std::max(a,b)+0.05)/(std::min(a,b)+0.05)>=4.5)return text;return b>0.179?QColor(Qt::black):QColor(Qt::white);}
QColor blend(const QColor &a,const QColor &b,double amount){return QColor::fromRgbF(a.redF()*(1-amount)+b.redF()*amount,a.greenF()*(1-amount)+b.greenF()*amount,a.blueF()*(1-amount)+b.blueF()*amount);}
}
bool SettingsStore::themedKey(const QString &key)const{return defaults().contains(key) && (key.endsWith("Color") || QStringList{"headerOpacity","panelOpacity","titleBackgroundOpacity","frameGlowStrength","glowStrength","lightBrightness"}.contains(key));}
bool SettingsStore::eventFilter(QObject *object,QEvent *event){if(event->type()==QEvent::ApplicationPaletteChange)m_themeRefresh.start();return QObject::eventFilter(object,event);}
void SettingsStore::armThemeWatch(){
    const auto directory=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);QDir().mkpath(directory);
    if(!m_themeFiles.directories().contains(directory))m_themeFiles.addPath(directory);
    for(const auto &name:{"kdeglobals","plasmarc"}){const auto path=directory+"/"+name;if(QFile::exists(path)&&!m_themeFiles.files().contains(path))m_themeFiles.addPath(path);}
}
void SettingsStore::refreshTheme(){
    m_kdeConfig->reparseConfiguration();m_plasmaConfig->reparseConfiguration();armThemeWatch();
    const QString styleName=KConfigGroup(m_plasmaConfig,"Theme").readEntry("name",QStringLiteral("default"));
    const auto safeStyle=[](const QString &name){return !name.contains('/')&&!name.contains('\\')&&!name.contains("..");};
    const auto styleRoot=[&](const QString &name){return safeStyle(name)?QStandardPaths::locate(QStandardPaths::GenericDataLocation,"plasma/desktoptheme/"+name,QStandardPaths::LocateDirectory):QString();};
    const auto selectedRoot=styleRoot(styleName),fallbackRoot=styleRoot("default");
    QVariantMap artwork;QStringList watched;QString stamp=styleName;
    const auto variant=m_values.value("plasmaArtworkVariant","automatic").toString();
    const auto variants=variant=="automatic"?QStringList{"translucent/","","solid/","opaque/"}:QStringList{variant+"/","","translucent/","solid/","opaque/"};
    for(const auto &role:QList<QPair<QString,QString>>{{"panel","dialogs/background"},{"header","widgets/background"},{"button","widgets/button"},{"input","widgets/lineedit"}}){
        QString found;
        for(const auto &root:{selectedRoot,fallbackRoot}){if(root.isEmpty())continue;for(const auto &part:variants){for(const auto &ext:{".svgz",".svg"}){const auto candidate=root+"/"+part+role.second+ext;if(QFileInfo(candidate).isFile()){found=candidate;break;}}if(!found.isEmpty())break;}if(!found.isEmpty())break;}
        artwork[role.first]=found;
        if(!found.isEmpty()){watched.append(found);const QFileInfo f(found);watched.append(f.absolutePath());stamp+=found+QString::number(f.lastModified().toMSecsSinceEpoch())+QString::number(f.size());}
    }
    artwork["_revision"]=stamp;
    const QString styleColors=selectedRoot.isEmpty()?QString():selectedRoot+"/colors";
    KSharedConfig::Ptr colorConfig=m_kdeConfig;
    const bool ownStyleColors=QFileInfo(styleColors).isFile();
    if(m_values.value("themeColorSource","application").toString()=="plasma" && ownStyleColors){colorConfig=KSharedConfig::openConfig(styleColors,KConfig::SimpleConfig);colorConfig->reparseConfiguration();}
    if(ownStyleColors){watched.append(styleColors);watched.append(selectedRoot);}
    for(const auto &path:watched){if(QFileInfo(path).isDir()){if(!m_themeFiles.directories().contains(path))m_themeFiles.addPath(path);}else if(!m_themeFiles.files().contains(path))m_themeFiles.addPath(path);}
    const QPalette palette=QGuiApplication::palette();
    auto color=[&colorConfig](const char *group,const char *key,const QColor &fallback){const auto c=KConfigGroup(colorConfig,group).readEntry(key,fallback);return c.isValid()?c:fallback;};
    const auto background=color("Colors:Window","BackgroundNormal",palette.window().color());
    const auto base=color("Colors:View","BackgroundNormal",palette.base().color());
    const auto text=readable(color("Colors:Window","ForegroundNormal",palette.windowText().color()),background);
    const auto kdeAccent=KConfigGroup(m_kdeConfig,"General").readEntry("AccentColor",KConfigGroup(m_kdeConfig,"Colors:Selection").readEntry("BackgroundNormal",palette.highlight().color()));
    const auto accent=colorConfig==m_kdeConfig?kdeAccent:color("Colors:Selection","BackgroundNormal",kdeAccent);
    const bool dark=luminance(background)<luminance(text);
    double hue=accent.hsvHueF();if(hue<0)hue=0;
    const auto complement=QColor::fromHsvF(std::fmod(hue+0.5,1.0),std::clamp(accent.hsvSaturationF(),0.35f,0.9f),dark?0.95:0.68);
    const auto analogous=QColor::fromHsvF(std::fmod(hue+0.08,1.0),std::clamp(accent.hsvSaturationF(),0.35f,0.9f),dark?0.95:0.68);
    QVariantMap info{{"kdeAccent",kdeAccent.name()},{"schemeName",KConfigGroup(m_kdeConfig,"General").readEntry("ColorScheme",QStringLiteral("Current KDE colors"))},{"dark",dark},{"accent",accent.name()},{"complementary",complement.name()},{"analogous",analogous.name()},{"plasmaTheme",styleName},{"plasmaHasOwnColors",ownStyleColors},{"artwork",artwork},{"artworkRevision",stamp},{"plasmaArtworkFallback",selectedRoot.isEmpty()},{"colorSource",colorConfig==m_kdeConfig?QStringLiteral("application"):QStringLiteral("plasma")}};
    if(info!=m_themeInfo){m_themeInfo=info;Q_EMIT themeInfoChanged();}
    const auto button=blend(background,text,dark?0.09:0.06),border=blend(background,text,0.36);
    m_themeValues={{"backgroundColor",background.name()},{"faceColor",base.name()},{"borderColor",accent.name()},{"hoverColor",dark?accent.lighter(125).name():accent.darker(120).name()},{"rimColor",dark?background.darker(160).name():background.darker(115).name()},
        {"frameGlowColor",accent.name()},{"titleColor",text.name()},{"titleBackgroundColor",background.name()},{"uiPanelColor",background.name()},{"uiBaseColor",base.name()},{"uiTextColor",text.name()},{"uiMutedColor",blend(text,background,0.25).name()},{"uiButtonColor",button.name()},{"uiBorderColor",border.name()},{"uiErrorColor",dark?"#ff9b9b":"#a31924"},{"uiHighlightTextColor",readable(color("Colors:Selection","ForegroundNormal",palette.highlightedText().color()),accent).name()},
        {"lightingColor",blend(accent,QColor(Qt::white),dark?0.32:0.15).name()},{"fillLightColor",dark?blend(text,background,0.08).name():QStringLiteral("#ffffff")},{"lightBrightness",dark?0.75:0.95},{"headerOpacity",dark?86.0:96.0},{"panelOpacity",dark?96.0:98.0},{"titleBackgroundOpacity",dark?90.0:96.0},{"frameGlowStrength",dark?55.0:38.0},{"glowStrength",dark?100.0:75.0}};
    updateEffective();
}
void SettingsStore::updateEffective(){
    QVariantMap next=m_values;const auto mode=m_values.value("themeMode","automatic").toString();const auto overrides=m_values["themeOverrides"].toStringList();
    if(mode!="manual"){
        auto themed=m_themeValues;const auto glow=m_values.value("themeGlowPalette","complementary").toString();
        if(glow=="complementary")themed["frameGlowColor"]=m_themeInfo["complementary"];
        else if(glow=="analogous")themed["frameGlowColor"]=m_themeInfo["analogous"];
        for(auto it=themed.cbegin();it!=themed.cend();++it)if(mode!="mixed" || !overrides.contains(it.key()))next[it.key()]=it.value();
    }
    next["followKdeTheme"]=mode!="manual";
    if(next!=m_effective){m_effective=next;Q_EMIT valuesChanged();}
}

namespace {
bool appearanceKey(const QString &key) {
    return !QStringList{"onboardingCompleted","autoUpright","autoUprightDelay","autoUprightDuration","rotationSpeed","inertiaEnabled","zoomPercent","shapeMorphEnabled","shapeMorphDuration"}.contains(key);
}
}
QString SettingsStore::presetDirectory()const{return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/polyswitch/presets";}
QString SettingsStore::presetPath(const QString &id)const {
    const QUuid uuid(id);return uuid.isNull() || uuid.toString(QUuid::WithoutBraces)!=id?QString():presetDirectory()+"/"+id+".json";
}
bool SettingsStore::presetFailure(const QString &message){m_presetError=message;Q_EMIT presetsChanged();return false;}
QVariantMap SettingsStore::appearance()const {
    QVariantMap settings;
    for(auto it=m_values.cbegin();it!=m_values.cend();++it)if(appearanceKey(it.key()) && it.key()!="backgroundImage")settings[it.key()]=it.value();
    QVariantMap document{{"format","org.kde.polyswitch.appearance"},{"version",1},{"settings",settings}};
    const QString image=QUrl(m_values.value("backgroundImage").toString()).toLocalFile();
    if(!image.isEmpty()){if(!QFileInfo(image).isFile())return {};QFile file(image);if(!file.open(QIODevice::ReadOnly)||file.size()>16*1024*1024)return {};document["backgroundPng"]=QString::fromLatin1(file.readAll().toBase64());}
    return document;
}
bool SettingsStore::writeAppearance(const QString &path,const QVariantMap &document){
    if(document.isEmpty())return presetFailure("The current background is missing, unreadable or larger than the 16 MB profile limit.");
    QDir().mkpath(QFileInfo(path).absolutePath());
    if(QFileInfo(path).absolutePath()==presetDirectory())QFile::setPermissions(presetDirectory(),QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
    QSaveFile file(path);const auto bytes=QJsonDocument::fromVariant(document).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())return presetFailure("Could not save the appearance file.");
    QFile::setPermissions(path,QFile::ReadOwner|QFile::WriteOwner);m_presetError.clear();Q_EMIT presetsChanged();return true;
}
QVariantMap SettingsStore::readAppearance(const QString &path)const {
    if(!QFileInfo(path).isFile())return {};
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>24*1024*1024)return {};
    const auto bytes=file.read(24*1024*1024+1);if(bytes.size()>24*1024*1024)return {};
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError || !document.isObject())return {};
    return document.object().toVariantMap();
}
QVariantList SettingsStore::presets()const {
    QVariantList list;
    for(const auto &file:QDir(presetDirectory()).entryInfoList({"*.json"},QDir::Files,QDir::Name)){
        const auto id=file.completeBaseName();if(presetPath(id).isEmpty())continue;
        const auto document=readAppearance(file.absoluteFilePath());if(document.value("format").toString()!="org.kde.polyswitch.appearance")continue;
        list.append(QVariantMap{{"id",id},{"name",document.value("name").toString()}});
    }
    std::sort(list.begin(),list.end(),[](const QVariant &a,const QVariant &b){return QString::localeAwareCompare(a.toMap()["name"].toString(),b.toMap()["name"].toString())<0;});return list;
}
QString SettingsStore::savePreset(const QString &name) {
    const auto clean=name.trimmed();if(clean.isEmpty()||clean.size()>80){presetFailure("Enter a preset name of 1–80 characters.");return {};}
    // Every save has its own ID: duplicate names never overwrite a saved look.
    auto document=appearance();document["name"]=clean;
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    return writeAppearance(presetPath(id),document)?id:QString();
}
bool SettingsStore::applyAppearance(const QVariantMap &document,bool snapshot) {
    if(document.value("format").toString()!="org.kde.polyswitch.appearance" || document.value("version").toInt()!=1 || document.value("settings").metaType().id()!=QMetaType::QVariantMap)return presetFailure("This is not a supported PolySwitch appearance preset.");
    const auto entries=document.value("settings").toMap();QVariantMap next=m_values;
    for(auto it=entries.cbegin();it!=entries.cend();++it){
        if(!appearanceKey(it.key()) || it.key()=="backgroundImage")return presetFailure("The preset contains a non-appearance setting.");
        const auto value=normalized(it.key(),it.value());if(!value.isValid())return presetFailure("The preset contains an invalid setting: "+it.key());next[it.key()]=value;
    }
    const auto encoded=document.value("backgroundPng").toString();QByteArray png;
    if(!encoded.isEmpty()){
        if(encoded.size()>23*1024*1024)return presetFailure("The preset background is too large.");
        const auto decoded=QByteArray::fromBase64Encoding(encoded.toLatin1(),QByteArray::AbortOnBase64DecodingErrors);if(!decoded)return presetFailure("The preset background is invalid.");
        png=decoded.decoded;QBuffer buffer(&png);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");const auto size=reader.size();
        if(!size.isValid()||size.width()>4096||size.height()>4096||qint64(size.width())*size.height()>32000000||reader.read().isNull())return presetFailure("The preset background cannot be decoded safely.");
    }
    if(snapshot && !writeAppearance(presetDirectory()+"/recovery.json",appearance()))return false;
    // Give each applied background its own managed copy; older presets contain their own bytes.
    QString target;
    if(!png.isEmpty()){
        target=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/polyswitch/backgrounds/background-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";
        QDir().mkpath(QFileInfo(target).absolutePath());QSaveFile output(target);
        if(!output.open(QIODevice::WriteOnly)||output.write(png)!=png.size()||!output.commit())return presetFailure("Could not save the preset background.");
        QFile::setPermissions(target,QFile::ReadOwner|QFile::WriteOwner);
    }
    next["backgroundImage"]=target.isEmpty()?QString():QUrl::fromLocalFile(target).toString();
    const auto previous=m_values;m_values=next;m_dirty=true;
    if(!flush()){m_values=previous;m_dirty=false;if(!target.isEmpty())QFile::remove(target);return presetFailure("Could not save the appearance settings.");}
    const auto oldImage=QUrl(previous.value("backgroundImage").toString()).toLocalFile();const QFileInfo oldFile(oldImage);
    const auto backgroundDir=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/polyswitch/backgrounds";
    if(oldFile.absolutePath()==QDir(backgroundDir).absolutePath() && oldFile.fileName().startsWith("background-") && oldImage!=target)QFile::remove(oldImage);
    refreshTheme();m_presetError.clear();Q_EMIT presetsChanged();return true;
}
bool SettingsStore::applyPreset(const QString &id){const auto path=presetPath(id);return path.isEmpty()?presetFailure("Select a saved preset."):applyAppearance(readAppearance(path),true);}
bool SettingsStore::deletePreset(const QString &id){const auto path=presetPath(id);if(path.isEmpty()||!QFile::remove(path))return presetFailure("Could not delete the preset.");m_presetError.clear();Q_EMIT presetsChanged();return true;}
bool SettingsStore::exportPreset(const QString &id,const QUrl &target){
    const auto path=presetPath(id);if(path.isEmpty()||!target.isLocalFile())return presetFailure("Choose a local destination for the preset.");
    const auto document=readAppearance(path);if(document.isEmpty())return presetFailure("The saved preset could not be read.");return writeAppearance(target.toLocalFile(),document);
}
bool SettingsStore::importPreset(const QUrl &source){
    if(!source.isLocalFile())return presetFailure("Choose a local preset file.");
    const auto document=readAppearance(source.toLocalFile());
    // Validate with the same rules as apply, without changing current appearance.
    if(document.value("format").toString()!="org.kde.polyswitch.appearance"||document.value("version").toInt()!=1||document.value("settings").metaType().id()!=QMetaType::QVariantMap)return presetFailure("This is not a supported PolySwitch appearance preset.");
    const auto entries=document.value("settings").toMap();for(auto it=entries.cbegin();it!=entries.cend();++it)if(!appearanceKey(it.key())||it.key()=="backgroundImage"||!normalized(it.key(),it.value()).isValid())return presetFailure("The preset contains invalid or non-appearance settings.");
    if(document.value("backgroundPng").toString().size()>23*1024*1024)return presetFailure("The preset background is too large.");
    const auto encoded=document.value("backgroundPng").toString();
    if(!encoded.isEmpty()){
        const auto decoded=QByteArray::fromBase64Encoding(encoded.toLatin1(),QByteArray::AbortOnBase64DecodingErrors);if(!decoded)return presetFailure("The preset background is invalid.");
        auto bytes=decoded.decoded;QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");const auto size=reader.size();
        if(!size.isValid()||size.width()>4096||size.height()>4096||qint64(size.width())*size.height()>32000000||reader.read().isNull())return presetFailure("The preset background cannot be decoded safely.");
    }
    QVariantMap imported{{"format","org.kde.polyswitch.appearance"},{"version",1},{"settings",entries}};
    if(!encoded.isEmpty())imported["backgroundPng"]=encoded;
    auto name=document.value("name").toString().trimmed();imported["name"]=name.isEmpty()?"Imported appearance":name.left(80);
    return writeAppearance(presetPath(QUuid::createUuid().toString(QUuid::WithoutBraces)),imported);
}
bool SettingsStore::canUndoAppearance()const{return QFile::exists(presetDirectory()+"/recovery.json");}
bool SettingsStore::resetAppearance(){QVariantMap settings;const auto defs=defaults();for(auto it=defs.cbegin();it!=defs.cend();++it)if(appearanceKey(it.key())&&it.key()!="backgroundImage")settings[it.key()]=it.value();return applyAppearance({{"format","org.kde.polyswitch.appearance"},{"version",1},{"settings",settings}},true);}
bool SettingsStore::undoAppearance(){const auto document=readAppearance(presetDirectory()+"/recovery.json");if(!applyAppearance(document,false))return false;QFile::remove(presetDirectory()+"/recovery.json");Q_EMIT presetsChanged();return true;}
