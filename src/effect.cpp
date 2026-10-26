// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include <QStandardPaths>
#include <QDesktopServices>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <workspace.h>
#include <input_event.h>
#include <QElapsedTimer>
#include <QPointer>
#include <input_event_spy.h>
#include <pointer_input.h>
#include <functional>
#include "prism.h"
#include "facegeometry.h"
#include "settingsstore.h"
#include "backgroundchooser.h"
#include <effect/quickeffect.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <effect/effect.h>
#include <QAction>
#include <QTimer>
#include <QKeyEvent>
#include <QKeySequence>
#include <QQmlEngine>
#include <QUrl>
#include <QStringList>
#include <algorithm>

namespace KWin {
class PointerObserver final : public InputEventSpy {
public:
    std::function<void(PointerMotionEvent *)> motion;
    std::function<void(PointerButtonEvent *)> button;
    std::function<void()> interaction;
    void pointerMotion(PointerMotionEvent *event) override { if (motion) motion(event); }
    void pointerButton(PointerButtonEvent *event) override { if (button) button(event); }
    void pointerAxis(PointerAxisEvent *) override { if(interaction)interaction(); }
    void keyboardKey(KeyboardKeyEvent *) override { if(interaction)interaction(); }
};
class PolySwitchEffect final : public QuickSceneEffect {
    Q_OBJECT
    Q_PROPERTY(QString pickerError READ pickerError NOTIFY backgroundPickerFinished)
    Q_PROPERTY(QObject *settings READ settings CONSTANT)
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(QString primaryOutputName READ primaryOutputName NOTIFY primaryOutputChanged)
public:
    QString pickerError()const{return m_pickerError;}
    Q_INVOKABLE bool consumeSettingsRequest(){const bool value=m_reopenSettings;m_reopenSettings=false;return value;}
    Q_INVOKABLE void chooseBackground(){if(m_chooser->active())return;QTimer::singleShot(0,this,[this]{m_chooser->open();});}
    Q_INVOKABLE void importProfile(){if(!m_chooser->active())QTimer::singleShot(0,this,[this]{m_chooser->importProfile();});}
    Q_INVOKABLE void exportProfile(const QString &id){if(!m_chooser->active())QTimer::singleShot(0,this,[this,id]{m_chooser->exportProfile(id);});}
    Q_INVOKABLE void disablePolySwitch(){QTimer::singleShot(0,this,[this]{m_settings.flush();KConfigGroup plugins(KSharedConfig::openConfig("kwinrc"),"Plugins");plugins.writeEntry("polyswitch_native4Enabled",false);plugins.sync();m_sessionDisabled=true;setRunning(false);});}
    Q_INVOKABLE void openRecoveryTools(){QTimer::singleShot(0,this,[this]{setRunning(false);QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/polyswitch/recovery"));});}
    QObject *settings() { return &m_settings; }
    QVariantList tasks() const { return m_tasks; }
    QString primaryOutputName() const { return m_primaryOutput ? m_primaryOutput->name() : QString(); }
Q_SIGNALS:
    void backgroundPickerFinished();
    void tasksChanged();
    void primaryOutputChanged();
    void animationTick(double milliseconds);
    void idleElapsed(double milliseconds);
public:
    Q_INVOKABLE void markInteraction() { m_idleClock.start(); }
    void setRunning(bool running) {
        // QuickSceneEffect unconditionally releases its keyboard grab on stop.
        // Reacquire ours first, or wait if another effect temporarily owns it.
        if (!running && isRunning() && !m_keyboardCaptured) {
            if (effects->hasKeyboardGrab()) { m_stopPending=true; return; }
            m_keyboardCaptured=effects->grabKeyboard(this);
        }
        m_stopPending=false;
        QuickSceneEffect::setRunning(running);
    }
    explicit PolySwitchEffect() {
        m_chooser=new BackgroundChooser(&m_settings,this);
        connect(m_chooser,&BackgroundChooser::started,this,[this]{m_settings.flush();setRunning(false);});
        connect(m_chooser,&BackgroundChooser::finished,this,[this](const QString &error){m_pickerError=error;m_reopenSettings=true;refreshWindows();setRunning(true);Q_EMIT backgroundPickerFinished();});
        qmlRegisterType<PrismGeometry>("PolySwitch.Native", 1, 0, "PrismGeometry");
        qmlRegisterType<FaceGeometry>("PolySwitch.Native", 1, 0, "FaceGeometry");
        setSource(QUrl(QStringLiteral("qrc:/polyswitch4/qml/main.qml")));
        setRunning(false);
        m_pointerObserver=std::make_unique<PointerObserver>();
        m_pointerObserver->motion=[this](PointerMotionEvent *event) {
            if (!isRunning()) return;
            if((event->position-m_lastInputPosition).manhattanLength()>0.75) { m_lastInputPosition=event->position;markInteraction(); }
            updateInputCapture(event->position);
            if (!m_mouseCaptured) pointerMotion(event); // clear hover outside
        };
        m_pointerObserver->button=[this](PointerButtonEvent *event) {
            if (!isRunning()) return;
            markInteraction();
            if (event->state==PointerButtonState::Pressed && !m_primaryPointerGrab && !onPrimary(event->position)) m_externalPointerGrab=true;
            updateInputCapture(event->position);
            if (!event->buttons) {
                m_externalPointerGrab=false;
                QTimer::singleShot(0,this,[this] { if(isRunning()) updateInputCapture(effects->cursorPos()); });
            }
        };
        m_pointerObserver->interaction=[this] { if(isRunning())markInteraction(); };
        input()->installInputEventSpy(m_pointerObserver.get());
        refreshPrimaryOutput();
        connect(workspace(), &Workspace::outputOrderChanged, this, [this] { refreshPrimaryOutput(); }, Qt::QueuedConnection);
        connect(effects, &EffectsHandler::screenAdded, this, [this] { refreshPrimaryOutput(); }, Qt::QueuedConnection);
        connect(effects, &EffectsHandler::screenRemoved, this, [this] { refreshPrimaryOutput(); }, Qt::QueuedConnection);
        m_animationTimer.setInterval(16);
        m_animationTimer.setTimerType(Qt::PreciseTimer);
        connect(&m_animationTimer, &QTimer::timeout, this, [this] {
            if (!isRunning()) return;
            const double elapsed = std::clamp(double(m_animationClock.restart()), 1.0, 64.0);
            Q_EMIT animationTick(elapsed);
            Q_EMIT idleElapsed(m_idleClock.isValid()?double(m_idleClock.elapsed()):0.0);
        });
        connect(this, &QuickSceneEffect::activated, this, [this] {
            m_lastInputPosition=effects->cursorPos();markInteraction();
            m_mouseCaptured=true; m_keyboardCaptured=true;
            m_externalPointerGrab=false;
            refreshPrimaryOutput();
            updateInputCapture(effects->cursorPos());
            m_animationClock.start(); m_animationTimer.start();
            QTimer::singleShot(0, this, [this] { syncPrimaryView(); });
        });
        connect(this, &QuickSceneEffect::deactivated, this, [this] {
            m_animationTimer.stop(); m_primaryPointerGrab=false; m_externalPointerGrab=false; m_mouseCaptured=false; m_keyboardCaptured=false;
        });
        m_windowTimer.setInterval(200);
        connect(&m_windowTimer, &QTimer::timeout, this, [this] {
            if (isRunning()) { if(m_stopPending) { setRunning(false); return; } refreshWindows(); syncPrimaryView(); }
        });
        m_windowTimer.start();
        auto *action = new QAction(this);
        action->setObjectName(QStringLiteral("PolySwitchNative4Toggle"));
        action->setText(QStringLiteral("PolySwitch Native Stage 4 toggle"));
        connect(action, &QAction::triggered, this, [this] {
            if(m_chooser->active() || m_sessionDisabled)return;
            if (isRunning()) { setRunning(false); return; }
            m_settings.reload();
            refreshWindows();
            setRunning(true);
        });
        action->setProperty("componentName", QStringLiteral("kwin"));
        KGlobalAccel::setGlobalShortcut(action, QKeySequence(QStringLiteral("Meta+Shift+P")));
    }
    // QML calls this from a MouseArea.clicked handler. Never call setRunning(false)
    // inside that handler: QuickSceneEffect destroys the event receiver on stop.
    Q_INVOKABLE void activateFace(int index) {
        if (!isRunning() || m_activationPending || index < 0 || index >= m_windows.size())
            return;
        EffectWindow *selected = m_windows.at(index);
        m_activationPending = true;
        QTimer::singleShot(0, this, [this, selected]() {
            m_activationPending = false;
            // The selected window may have closed between click and callback.
            if (!isRunning())
                return;
            const bool stillExists = effects->stackingOrder().contains(selected);
            if (!stillExists) {
                refreshWindows();
                setRunning(false);
                return;
            }
            if (selected->isDeleted()) {
                setRunning(false);
                return;
            }
            setRunning(false);
            if (selected->isMinimized())
                selected->setMinimized(false);
            effects->activateWindow(selected);
        });
    }
    Q_INVOKABLE void activateWindowId(const QString &id) {
        refreshWindows();
        const int index = m_windowIds.indexOf(id);
        if (index >= 0) activateFace(index);
    }
    QVariantMap initialProperties(LogicalOutput *output) override {
        return {{QStringLiteral("effectBridge"), QVariant::fromValue(static_cast<QObject *>(this))},
                {QStringLiteral("outputName"), output->name()}};
    }
    void grabbedKeyboardEvent(QKeyEvent *event) override {
        if (event->type() == QEvent::KeyPress && event->key() == Qt::Key_Escape) {
            m_settings.flush();
            setRunning(false);
            return;
        }
        syncPrimaryView();
        if (auto *view=viewForScreen(m_primaryOutput)) view->forwardKeyEvent(event);
    }
    void pointerMotion(PointerMotionEvent *event) override {
        // Forward outside motion too: clear hover and finish a drag that
        // crosses an output boundary without creating a second input scene.
        if (auto *view=viewForScreen(m_primaryOutput)) view->forwardPointerMotionEvent(event);
    }
    void pointerButton(PointerButtonEvent *event) override {
        if (!m_primaryPointerGrab && !onPrimary(event->position)) return;
        if (event->buttons) m_primaryPointerGrab=true;
        syncPrimaryView();
        if (auto *view=viewForScreen(m_primaryOutput)) view->forwardPointerButtonEvent(event);
        if (!event->buttons) m_primaryPointerGrab=false;
    }
    void pointerAxis(PointerAxisEvent *event) override {
        if (onPrimary(event->position))
            if (auto *view=viewForScreen(m_primaryOutput)) view->forwardPointerAxisEvent(event);
    }
private:
    void updateInputCapture(const QPointF &position) {
        if (!isRunning()) return;
        const bool wanted=m_primaryPointerGrab || (!m_externalPointerGrab && onPrimary(position));
        bool changed=false;
        if (wanted && !m_mouseCaptured) { effects->startMouseInterception(this,Qt::ArrowCursor); m_mouseCaptured=true; changed=true; }
        else if (!wanted && m_mouseCaptured) { effects->stopMouseInterception(this); m_mouseCaptured=false; changed=true; }
        if (wanted && !m_keyboardCaptured && !effects->hasKeyboardGrab()) m_keyboardCaptured=effects->grabKeyboard(this);
        else if (!wanted && m_keyboardCaptured) { effects->ungrabKeyboard(); m_keyboardCaptured=false; }
        if (changed) input()->pointer()->update();
    }
    std::unique_ptr<PointerObserver> m_pointerObserver;
    bool m_mouseCaptured=false;
    bool m_keyboardCaptured=false;
    bool m_externalPointerGrab=false;
    bool m_stopPending=false;
    bool onPrimary(const QPointF &position) const {
        return m_primaryOutput && m_primaryOutput->geometry().contains(position.toPoint());
    }
    void refreshPrimaryOutput() {
        const auto screens=effects->screens();
        LogicalOutput *primary=nullptr;
        for (auto *output : workspace()->outputOrder())
            if (screens.contains(output)) { primary=output; break; }
        if (!primary && !screens.isEmpty()) primary=screens.first();
        if (m_primaryOutput != primary) {
            m_primaryOutput=primary;
            Q_EMIT primaryOutputChanged();
        }
        syncPrimaryView();
        if(isRunning()) updateInputCapture(effects->cursorPos());
    }
    void syncPrimaryView() {
        if (!isRunning()) return;
        for (auto *screen : effects->screens()) {
            if (auto *view=viewForScreen(screen)) {
                const bool shown=screen==m_primaryOutput;
                if (view->isVisible()!=shown) view->setVisible(shown);
            }
        }
        if (auto *view=viewForScreen(m_primaryOutput)) activateView(view);
    }
    QPointer<LogicalOutput> m_primaryOutput;
    bool m_primaryPointerGrab=false;
    QTimer m_animationTimer;
    QElapsedTimer m_animationClock;
    QElapsedTimer m_idleClock;
    QPointF m_lastInputPosition;
    void refreshWindows() {
        QList<EffectWindow *> available;
        for (auto *window : effects->stackingOrder()) {
            if (window && window->isNormalWindow() && !window->isDeleted()
                && !window->caption().trimmed().isEmpty()) available.append(window);
        }
        // Preserve order; validate old pointers against the live list before use.
        QList<EffectWindow *> ordered;
        for (auto *window : std::as_const(m_windows))
            if (available.contains(window)) ordered.append(window);
        for (auto *window : std::as_const(available))
            if (!ordered.contains(window) && ordered.size() < 64) ordered.append(window);
        QVariantList next;
        m_windows = ordered;
        m_titles.clear(); m_windowIds.clear();
        for (auto *window : std::as_const(m_windows)) {
            const QString id = window->internalId().toString();
            const QString title = window->caption().trimmed();
            m_windowIds.append(id); m_titles.append(title);
            next.append(QVariantMap{{"id",id},{"title",title}});
        }
        if (next != m_tasks) { m_tasks = next; Q_EMIT tasksChanged(); }
    }
    bool m_sessionDisabled=false;
    BackgroundChooser *m_chooser=nullptr;
    QString m_pickerError;
    bool m_reopenSettings=false;
    QVariantList m_tasks;
    QTimer m_windowTimer;
    SettingsStore m_settings;
    bool m_activationPending = false;
    QStringList m_titles;
    QStringList m_windowIds;
    QList<EffectWindow *> m_windows;
};
}
KWIN_EFFECT_FACTORY(KWin::PolySwitchEffect, "effect.json")
#include "effect.moc"
