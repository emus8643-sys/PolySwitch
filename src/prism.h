// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QtQuick3D/QQuick3DGeometry>
#include <QVariantList>
#include <QVariantAnimation>
#include <QStringList>
#include <algorithm>
class PrismGeometry : public QQuick3DGeometry {
    Q_OBJECT
    Q_PROPERTY(int taskCount READ taskCount WRITE setTaskCount NOTIFY taskCountChanged)
    Q_PROPERTY(QVariantList faces READ faces NOTIFY facesChanged)
    Q_PROPERTY(QStringList taskIds READ taskIds WRITE setTaskIds NOTIFY taskCountChanged)
    Q_PROPERTY(bool animated READ animated WRITE setAnimated NOTIFY animationOptionsChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY animationOptionsChanged)
    Q_PROPERTY(bool morphing READ morphing NOTIFY morphingChanged)
public:
    QStringList taskIds() const {return m_ids;}
    void setTaskIds(const QStringList &ids);
    bool animated() const {return m_animated;}
    void setAnimated(bool value);
    int duration() const {return m_duration;}
    void setDuration(int value){m_duration=std::clamp(value,180,1200);Q_EMIT animationOptionsChanged();}
    bool morphing() const {return m_animation.state()==QAbstractAnimation::Running;}
    explicit PrismGeometry(QQuick3DObject *parent = nullptr);
    int taskCount() const { return m_taskCount; }
    QVariantList faces() const { return m_faces; }
    void setTaskCount(int count);
Q_SIGNALS:
    void taskCountChanged();
    void facesChanged();
    void morphingChanged();
    void animationOptionsChanged();
private:
    void rebuild();
    void uploadFaces();
    void transitionTo(const QVariantList &target);
    void interpolate(float progress);
    QStringList m_ids,m_displayIds;
    QVariantList m_startFaces,m_endFaces,m_targetFaces;
    QVariantAnimation m_animation;
    bool m_planeMorph=false,m_planesAvailable=false;
    bool m_animated=false;
    int m_duration=650;
    int m_taskCount = 0;
    QVariantList m_faces;
};
