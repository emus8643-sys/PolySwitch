// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "prism.h"
#include <QVector3D>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
using Polygon = std::vector<QVector3D>;
constexpr double pi = 3.14159265358979323846;
// Intersections of equally distant tangent planes produce a convex, faceted
// sphere. Each unit direction contributes exactly one usable face.
std::vector<Polygon> tangentSolid(const std::vector<QVector3D> &normals) {
    std::vector<QVector3D> corners;
    const int n = int(normals.size());
    for (int i=0; i<n; ++i) for (int j=i+1; j<n; ++j) for (int k=j+1; k<n; ++k) {
        const auto cross = QVector3D::crossProduct(normals[j], normals[k]);
        const double det = QVector3D::dotProduct(normals[i], cross);
        if (std::abs(det) < 1e-7) continue;
        const auto p = (cross + QVector3D::crossProduct(normals[k], normals[i])
                             + QVector3D::crossProduct(normals[i], normals[j])) / float(det);
        bool inside = true;
        for (const auto &normal : normals) {
            if (QVector3D::dotProduct(normal, p) > 1.00002f) { inside=false; break; }
        }
        if (!inside) continue;
        bool duplicate = false;
        for (const auto &corner : corners) {
            if ((p-corner).lengthSquared() < 1e-8f) { duplicate=true; break; }
        }
        if (!duplicate) corners.push_back(p);
    }
    float outerRadius = 0;
    for (const auto &p : corners) outerRadius = std::max(outerRadius,p.length());
    std::vector<Polygon> result;
    for (const auto &normal : normals) {
        Polygon polygon;
        QVector3D center;
        for (const auto &p : corners) {
            if (std::abs(QVector3D::dotProduct(normal,p)-1.f) < 0.00001f) {
                polygon.push_back(p); center += p;
            }
        }
        if (polygon.size()<3) return {}; // Never publish malformed polygons.
        center /= float(polygon.size());
        const auto u = QVector3D::crossProduct(normal, std::abs(normal.y())<0.9f ? QVector3D(0,1,0) : QVector3D(1,0,0)).normalized();
        const auto v = QVector3D::crossProduct(normal,u);
        std::sort(polygon.begin(),polygon.end(),[&](const auto &a,const auto &b){
            const auto da=a-center, db=b-center;
            return std::atan2(QVector3D::dotProduct(da,v),QVector3D::dotProduct(da,u))
                 < std::atan2(QVector3D::dotProduct(db,v),QVector3D::dotProduct(db,u));
        });
        for (auto &p : polygon) p *= 185.f / outerRadius;
        result.push_back(polygon);
    }
    return result;
}
std::vector<QVector3D> faceDirections(int count) {
    std::vector<QVector3D> normals;
    const float phi = (1.f+std::sqrt(5.f))/2.f;
    if (count==6) {
        normals={{0,0,1},{1,0,0},{0,1,0},{0,-1,0},{0,0,-1},{-1,0,0}};
    } else if (count==8) {
        for (float x : {-1.f,1.f}) for (float y : {-1.f,1.f}) for (float z : {-1.f,1.f}) normals.emplace_back(x,y,z);
    } else if (count==12) {
        // Icosahedron vertices are the face normals of a regular dodecahedron.
        for (float a : {-1.f,1.f}) for (float b : {-phi,phi}) {
            normals.emplace_back(0,a,b); normals.emplace_back(a,b,0); normals.emplace_back(b,0,a);
        }
    } else if (count==20) {
        for (float x : {-1.f,1.f}) for (float y : {-1.f,1.f}) for (float z : {-1.f,1.f}) normals.emplace_back(x,y,z);
        for (float a : {-1.f/phi,1.f/phi}) for (float b : {-phi,phi}) {
            normals.emplace_back(0,a,b); normals.emplace_back(a,b,0); normals.emplace_back(b,0,a);
        }
    } else {
        // Fibonacci directions cover the whole sphere, rather than a side ring.
        const double goldenAngle = pi * (3.0-std::sqrt(5.0));
        for (int i=0; i<count; ++i) {
            const double y=1.0-2.0*(i+0.5)/count;
            const double r=std::sqrt(1.0-y*y), a=i*goldenAngle;
            normals.emplace_back(float(r*std::sin(a)),float(y),float(r*std::cos(a)));
        }
    }
    for (auto &normal : normals) normal.normalize();
    return normals;
}
}
PrismGeometry::PrismGeometry(QQuick3DObject *parent) : QQuick3DGeometry(parent) { rebuild(); }
void PrismGeometry::setTaskCount(int count) {
    count=std::clamp(count,0,64);
    if (count==m_taskCount) return;
    m_taskCount=count; rebuild(); Q_EMIT taskCountChanged();
}
void PrismGeometry::rebuild() {
    struct Vertex { float x,y,z,nx,ny,nz; };
    std::vector<Vertex> vertices;
    std::vector<quint32> indices;
    std::vector<Polygon> polygons;
    const int count=m_taskCount;
    if (count==1) {
        polygons.push_back({{-160,-105,0},{160,-105,0},{160,105,0},{-160,105,0}});
    } else if (count==2) {
        // Tiny separation prevents z-fighting, while reading as a two-sided card.
        polygons.push_back({{-160,-105,0.25f},{160,-105,0.25f},{160,105,0.25f},{-160,105,0.25f}});
        polygons.push_back({{160,-105,-0.25f},{-160,-105,-0.25f},{-160,105,-0.25f},{160,105,-0.25f}});
    } else if (count==3 || count==4) {
        // Deliberately open: only the rectangular ring, with no caps at all.
        for (int i=0;i<count;++i) {
            const double a=2*pi*i/count, b=2*pi*(i+1)/count;
            const QVector3D lowA(160*std::sin(a),-115,160*std::cos(a));
            const QVector3D lowB(160*std::sin(b),-115,160*std::cos(b));
            polygons.push_back({lowA,lowB,lowB+QVector3D(0,230,0),lowA+QVector3D(0,230,0)});
        }
    } else if (count==5) {
        const QVector3D apex(0,165,0);
        Polygon base{{-135,-100,-135},{135,-100,-135},{135,-100,135},{-135,-100,135}};
        for (int i=0;i<4;++i) polygons.push_back({base[i],base[(i+1)%4],apex});
        polygons.push_back(base);
    } else if (count>=6) {
        polygons=tangentSolid(faceDirections(count));
    }
    m_faces.clear();
    for (auto &polygon : polygons) {
        QVector3D center;
        for (const auto &point : polygon) center+=point;
        center/=float(polygon.size());
        // Near-four-plane intersections can produce very short polygon edges.
        // Choose the widest triangle, avoiding an unstable normal from three
        // nearly collinear consecutive vertices.
        QVector3D normal;
        for (size_t i=1;i+1<polygon.size();++i) {
            const auto candidate=QVector3D::crossProduct(polygon[i]-polygon[0],polygon[i+1]-polygon[0]);
            if (candidate.lengthSquared()>normal.lengthSquared()) normal=candidate;
        }
        normal.normalize();
        if (QVector3D::dotProduct(normal,center)<0) {
            std::reverse(polygon.begin(),polygon.end()); normal=-normal;
        }
        QVariantList points;
        for (const auto &point : polygon) points.append(QVariant::fromValue(point));
        m_faces.append(QVariantMap{{"center",QVariant::fromValue(center)}, {"normal",QVariant::fromValue(normal)}, {"vertices",points}});
        for (size_t i=1;i+1<polygon.size();++i) {
            const auto offset=quint32(vertices.size());
            for (const auto &p : {polygon[0],polygon[i],polygon[i+1]}) vertices.push_back({p.x(),p.y(),p.z(),normal.x(),normal.y(),normal.z()});
            indices.insert(indices.end(),{offset,offset+1,offset+2});
        }
    }
    clear(); setStride(sizeof(Vertex));
    setVertexData(QByteArray(reinterpret_cast<const char *>(vertices.data()),qsizetype(vertices.size()*sizeof(Vertex))));
    setIndexData(QByteArray(reinterpret_cast<const char *>(indices.data()),qsizetype(indices.size()*sizeof(quint32))));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,0,QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,3*sizeof(float),QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,0,QQuick3DGeometry::Attribute::U32Type);
    setBounds({-200,-200,-200},{200,200,200}); update(); Q_EMIT facesChanged();
}
