// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#pragma once
#include <QtQuick3D/QQuick3DGeometry>
#include <QVariantList>
class PrismGeometry : public QQuick3DGeometry {
    Q_OBJECT
    Q_PROPERTY(int taskCount READ taskCount WRITE setTaskCount NOTIFY taskCountChanged)
    Q_PROPERTY(QVariantList faces READ faces NOTIFY facesChanged)
public:
    explicit PrismGeometry(QQuick3DObject *parent = nullptr);
    int taskCount() const { return m_taskCount; }
    QVariantList faces() const { return m_faces; }
    void setTaskCount(int count);
Q_SIGNALS:
    void taskCountChanged();
    void facesChanged();
private:
    void rebuild();
    int m_taskCount = 0;
    QVariantList m_faces;
};
