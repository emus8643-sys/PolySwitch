// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QtQuick3D/QQuick3DGeometry>
#include <QVariantMap>
class FaceGeometry : public QQuick3DGeometry {
    Q_OBJECT
    Q_PROPERTY(QVariantMap face READ face WRITE setFace NOTIFY geometryChanged)
    Q_PROPERTY(float inset READ inset WRITE setInset NOTIFY geometryChanged)
    Q_PROPERTY(float offset READ offset WRITE setOffset NOTIFY geometryChanged)
    Q_PROPERTY(bool twoSided READ twoSided WRITE setTwoSided NOTIFY geometryChanged)
public:
    explicit FaceGeometry(QQuick3DObject *parent=nullptr):QQuick3DGeometry(parent){}
    QVariantMap face() const {return m_face;}
    float inset() const {return m_inset;}
    float offset() const {return m_offset;}
    bool twoSided() const {return m_twoSided;}
    void setFace(const QVariantMap &value) {if(m_face==value)return;m_face=value;rebuild();}
    void setInset(float value) {if(m_inset==value)return;m_inset=value;rebuild();}
    void setOffset(float value) {if(m_offset==value)return;m_offset=value;rebuild();}
    void setTwoSided(bool value) {if(m_twoSided==value)return;m_twoSided=value;rebuild();}
Q_SIGNALS:
    void geometryChanged();
private:
    void rebuild();
    QVariantMap m_face;
    float m_inset=0;
    float m_offset=0;
    bool m_twoSided=false;
};
