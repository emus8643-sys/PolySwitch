// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "facegeometry.h"
#include <QVector3D>
#include <algorithm>
#include <limits>
#include <vector>
void FaceGeometry::rebuild() {
    struct Vertex {float x,y,z,nx,ny,nz,u,v;};
    std::vector<Vertex> vertices;
    const auto points=m_face.value("vertices").toList();
    const auto center=m_face.value("center").value<QVector3D>();
    const auto outward=m_face.value("normal").value<QVector3D>();
    const float scale=1.f-std::clamp(m_inset,0.f,0.4f);
    if(points.size()>=3 && outward.lengthSquared()>0.5f) {
        for(int side=0;side<(m_twoSided?2:1);++side) {
            const auto normal=side==0?outward:-outward;
            // Keep source windows upright in the local face plane. Caps use
            // scene -Z as up; the reverse side gets its own non-mirrored basis.
            const auto preferred=std::abs(normal.y())<0.95f?QVector3D(0,1,0):QVector3D(0,0,-1);
            const auto up=(preferred-normal*QVector3D::dotProduct(preferred,normal)).normalized();
            const auto right=QVector3D::crossProduct(up,normal).normalized();
            float minU=std::numeric_limits<float>::max(),minV=minU,maxU=-minU,maxV=-minU;
            for(const auto &value:points) {
                const auto p=value.value<QVector3D>()-center;
                const float u=QVector3D::dotProduct(p,right),v=QVector3D::dotProduct(p,up);
                minU=std::min(minU,u);maxU=std::max(maxU,u);minV=std::min(minV,v);maxV=std::max(maxV,v);
            }
            auto append=[&](int index){
                const auto local=points[index].value<QVector3D>()-center;
                const auto p=center+local*scale+normal*m_offset;
                vertices.push_back({p.x(),p.y(),p.z(),normal.x(),normal.y(),normal.z(),
                    (QVector3D::dotProduct(local,right)-minU)/std::max(0.001f,maxU-minU),
                    (QVector3D::dotProduct(local,up)-minV)/std::max(0.001f,maxV-minV)});
            };
            for(int i=1;i+1<points.size();++i) {
                append(0);append(side==0?i:i+1);append(side==0?i+1:i);
            }
        }
    }
    clear();setStride(sizeof(Vertex));
    setVertexData(QByteArray(reinterpret_cast<const char *>(vertices.data()),qsizetype(vertices.size()*sizeof(Vertex))));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,0,QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,3*sizeof(float),QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,6*sizeof(float),QQuick3DGeometry::Attribute::F32Type);
    setBounds({-201,-201,-201},{201,201,201});update();Q_EMIT geometryChanged();
}
