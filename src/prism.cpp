// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
#include "prism.h"
#include <QVector3D>
#include <QQuaternion>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>

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
PrismGeometry::PrismGeometry(QQuick3DObject *parent) : QQuick3DGeometry(parent) {
    m_animation.setStartValue(0.0);m_animation.setEndValue(1.0);m_animation.setEasingCurve(QEasingCurve::InOutCubic);
    connect(&m_animation,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){interpolate(v.toFloat());});
    connect(&m_animation,&QVariantAnimation::stateChanged,this,[this]{Q_EMIT morphingChanged();});
    connect(&m_animation,&QVariantAnimation::finished,this,[this]{m_faces=m_targetFaces;m_displayIds=m_ids;m_planesAvailable=m_taskCount>=5;uploadFaces();});
    rebuild();
}
void PrismGeometry::setAnimated(bool value){if(m_animated==value)return;m_animated=value;if(!value && morphing()){m_animation.stop();m_faces=m_targetFaces;m_displayIds=m_ids;m_planesAvailable=m_taskCount>=5;uploadFaces();}Q_EMIT animationOptionsChanged();}
void PrismGeometry::setTaskIds(const QStringList &ids){
    const auto next=ids.mid(0,64);if(next==m_ids)return;
    m_ids=next;m_taskCount=next.size();rebuild();Q_EMIT taskCountChanged();
}
void PrismGeometry::setTaskCount(int count) {
    count=std::clamp(count,0,64);
    if (count==m_taskCount) return;
    m_ids.clear();for(int i=0;i<count;i++)m_ids.append(QString::number(i));m_taskCount=count; rebuild(); Q_EMIT taskCountChanged();
}
void PrismGeometry::rebuild() {
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
    QVariantList target;
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
        target.append(QVariantMap{{"center",QVariant::fromValue(center)}, {"normal",QVariant::fromValue(normal)}, {"vertices",points}});
    }
    transitionTo(target);
}
void PrismGeometry::uploadFaces(){
    struct Vertex {float x,y,z,nx,ny,nz;};std::vector<Vertex> vertices;std::vector<quint32> indices;
    for(const auto &entry:m_faces){const auto face=entry.toMap();const auto points=face["vertices"].toList();const auto normal=face["normal"].value<QVector3D>();
        for(qsizetype i=1;i+1<points.size();i++){const auto offset=quint32(vertices.size());for(const auto &v:{points[0],points[i],points[i+1]}){const auto p=v.value<QVector3D>();vertices.push_back({p.x(),p.y(),p.z(),normal.x(),normal.y(),normal.z()});}indices.insert(indices.end(),{offset,offset+1,offset+2});}
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

namespace {
QVector3D faceUp(const QVariantMap &face){const auto n=face["normal"].value<QVector3D>();if(face.contains("up"))return face["up"].value<QVector3D>();const auto preferred=std::abs(n.y())<0.95f?QVector3D(0,1,0):QVector3D(0,0,-1);return (preferred-n*QVector3D::dotProduct(preferred,n)).normalized();}
QVector3D blendedUp(const QVariantMap &a,const QVariantMap &b,const QVector3D &normal,float t){
    const auto an=a["normal"].value<QVector3D>(),bn=b["normal"].value<QVector3D>(),au=faceUp(a),bu=faceUp(b);
    const auto from=QQuaternion::fromAxes(QVector3D::crossProduct(au,an),au,an),to=QQuaternion::fromAxes(QVector3D::crossProduct(bu,bn),bu,bn);
    const auto candidate=QQuaternion::slerp(from,to,t).rotatedVector(QVector3D(0,1,0));const auto projected=candidate-normal*QVector3D::dotProduct(candidate,normal);return projected.lengthSquared()>0.0001f?projected.normalized():au;
}
QVariantMap collapsed(const QVariantMap &face,const QVector3D &center){auto copy=face;QVariantList points;for(const auto &v:face["vertices"].toList()){Q_UNUSED(v);points.append(QVariant::fromValue(center));}copy["vertices"]=points;copy["center"]=QVariant::fromValue(center);return copy;}
// Use the union of both polygon corner positions along their perimeters. This
// preserves every old/new corner at the endpoints even when edge counts differ.
std::pair<QVariantList,QVariantList> correspondence(QVariantList a,QVariantList b){
    if(a.isEmpty() || b.isEmpty())return {a,b};
    int shift=0;float best=std::numeric_limits<float>::max();for(int i=0;i<a.size();i++){const float d=(a[i].value<QVector3D>()-b[0].value<QVector3D>()).lengthSquared();if(d<best){best=d;shift=i;}}
    std::rotate(a.begin(),a.begin()+shift,a.end());
    auto lengths=[](const QVariantList &p){QList<float> v{0};for(int i=0;i<p.size();i++)v.append(v.last()+(p[(i+1)%p.size()].value<QVector3D>()-p[i].value<QVector3D>()).length());if(v.last()<0.0001f){for(int i=0;i<v.size();i++)v[i]=float(i)/p.size();}else{const float total=v.last();for(auto &x:v)x/=total;}return v;};
    const auto la=lengths(a),lb=lengths(b);QList<float> knots=la;knots.append(lb);std::sort(knots.begin(),knots.end());knots.erase(std::unique(knots.begin(),knots.end(),[](float x,float y){return std::abs(x-y)<0.00001f;}),knots.end());
    auto sample=[](const QVariantList &p,const QList<float>&l,float t){int i=0;while(i+1<p.size() && l[i+1]<=t)i++;const float f=(t-l[i])/std::max(0.000001f,l[i+1]-l[i]);return p[i].value<QVector3D>()*(1-f)+p[(i+1)%p.size()].value<QVector3D>()*f;};
    QVariantList aa,bb;for(float t:knots)if(t<0.99999f){aa.append(QVariant::fromValue(sample(a,la,t)));bb.append(QVariant::fromValue(sample(b,lb,t)));}return {aa,bb};
}
}
void PrismGeometry::transitionTo(const QVariantList &target){
    m_animation.stop();
    QVariantList ordered=target;
    // Minimum-cost assignment keeps surviving windows on nearby face planes.
    // Dummy rows allow newly opened tasks to take the remaining destinations.
    if(m_animated && !m_faces.isEmpty() && !target.isEmpty()){
        const int n=target.size();QList<double> u(n+1),v(n+1);QList<int> assignment(n+1),way(n+1);
        for(int i=1;i<=n;i++){assignment[0]=i;int j0=0;QList<double> minimum(n+1,1e9);QList<bool> used(n+1,false);
            do{used[j0]=true;const int row=assignment[j0];double delta=1e9;int next=0;const int prior=m_displayIds.indexOf(m_ids.value(row-1));
                for(int j=1;j<=n;j++)if(!used[j]){double cost=0;if(prior>=0 && prior<m_faces.size())cost=1-QVector3D::dotProduct(m_faces[prior].toMap()["normal"].value<QVector3D>(),target[j-1].toMap()["normal"].value<QVector3D>());const double current=cost-u[row]-v[j];if(current<minimum[j]){minimum[j]=current;way[j]=j0;}if(minimum[j]<delta){delta=minimum[j];next=j;}}
                for(int j=0;j<=n;j++){if(used[j]){u[assignment[j]]+=delta;v[j]-=delta;}else minimum[j]-=delta;}j0=next;
            }while(assignment[j0]!=0);
            do{const int next=way[j0];assignment[j0]=assignment[next];j0=next;}while(j0);
        }
        for(int j=1;j<=n;j++)ordered[assignment[j]-1]=target[j-1];
    }
    m_targetFaces=ordered;
    if(!m_animated || m_faces.isEmpty()){m_faces=ordered;m_displayIds=m_ids;m_planesAvailable=m_taskCount>=5;uploadFaces();return;}
    const auto old=m_faces;const auto oldIds=m_displayIds;
    m_planeMorph=m_planesAvailable && m_taskCount>=5;
    m_planesAvailable=m_planeMorph;m_startFaces.clear();m_endFaces.clear();m_displayIds=m_ids;
    auto nearest=[](const QVariantMap &face,const QVariantList &others){QVariantMap nearest=face;float best=std::numeric_limits<float>::max();for(const auto &other:others){const auto f=other.toMap();const float distance=(f["center"].value<QVector3D>()-face["center"].value<QVector3D>()).lengthSquared();if(distance<best){best=distance;nearest=f;}}return nearest;};
    for(int i=0;i<ordered.size();i++){
        const auto end=ordered[i].toMap();const int prior=oldIds.indexOf(m_ids.value(i));
        const auto start=prior>=0 && prior<old.size()?old[prior].toMap():collapsed(end,m_planeMorph?end["normal"].value<QVector3D>()*400:nearest(end,old)["center"].value<QVector3D>());
        m_startFaces.append(start);m_endFaces.append(end);
    }
    for(int i=0;i<oldIds.size();i++)if(!m_ids.contains(oldIds[i]) && i<old.size()){
        const auto start=old[i].toMap();m_displayIds.append(oldIds[i]);m_startFaces.append(start);m_endFaces.append(collapsed(start,m_planeMorph?start["normal"].value<QVector3D>()*400:nearest(start,ordered)["center"].value<QVector3D>()));
    }
    for(int i=0;i<m_startFaces.size();i++){auto a=m_startFaces[i].toMap(),b=m_endFaces[i].toMap();const auto pair=correspondence(a["vertices"].toList(),b["vertices"].toList());a["vertices"]=pair.first;b["vertices"]=pair.second;m_startFaces[i]=a;m_endFaces[i]=b;}
    interpolate(0);m_animation.setDuration(m_duration);m_animation.start();
}
void PrismGeometry::interpolate(float t){
    if(m_planeMorph){
        std::vector<QVector3D> normals;std::vector<float> distances;
        for(int i=0;i<m_startFaces.size();i++){const auto a=m_startFaces[i].toMap(),b=m_endFaces[i].toMap();const auto an=a["normal"].value<QVector3D>(),bn=b["normal"].value<QVector3D>();normals.push_back(QQuaternion::slerp(QQuaternion(),QQuaternion::rotationTo(an,bn),t).rotatedVector(an).normalized());
            const float ad=a.contains("distance")?a["distance"].toFloat():QVector3D::dotProduct(an,a["center"].value<QVector3D>());const float bd=QVector3D::dotProduct(bn,b["center"].value<QVector3D>());distances.push_back(ad*(1-t)+bd*t);
        }
        const int n=normals.size();std::vector<QVector3D> corners;
        for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)for(int k=j+1;k<n;k++){
            const auto cross=QVector3D::crossProduct(normals[j],normals[k]);const float det=QVector3D::dotProduct(normals[i],cross);if(std::abs(det)<1e-6f)continue;
            const auto point=(cross*distances[i]+QVector3D::crossProduct(normals[k],normals[i])*distances[j]+QVector3D::crossProduct(normals[i],normals[j])*distances[k])/det;
            bool inside=true;for(int p=0;p<n;p++)if(QVector3D::dotProduct(normals[p],point)>distances[p]+0.002f){inside=false;break;}if(!inside)continue;
            bool duplicate=false;for(const auto &old:corners)if((old-point).lengthSquared()<0.0001f){duplicate=true;break;}if(!duplicate)corners.push_back(point);
        }
        m_faces.clear();for(int i=0;i<n;i++){
            std::vector<QVector3D> polygon;QVector3D center;for(const auto &point:corners)if(std::abs(QVector3D::dotProduct(normals[i],point)-distances[i])<0.004f){polygon.push_back(point);center+=point;}
            if(polygon.empty())center=normals[i]*distances[i];else center/=float(polygon.size());
            const auto preferred=std::abs(normals[i].y())<0.95f?QVector3D(0,1,0):QVector3D(0,0,-1);const auto up=(preferred-normals[i]*QVector3D::dotProduct(preferred,normals[i])).normalized();const auto right=QVector3D::crossProduct(up,normals[i]);
            std::sort(polygon.begin(),polygon.end(),[&](const auto &a,const auto &b){const auto aa=a-center,bb=b-center;return std::atan2(QVector3D::dotProduct(aa,up),QVector3D::dotProduct(aa,right))<std::atan2(QVector3D::dotProduct(bb,up),QVector3D::dotProduct(bb,right));});
            QVariantList points;for(const auto &p:polygon)points.append(QVariant::fromValue(p));m_faces.append(QVariantMap{{"vertices",points},{"center",QVariant::fromValue(center)},{"normal",QVariant::fromValue(normals[i])},{"distance",distances[i]},{"up",QVariant::fromValue(blendedUp(m_startFaces[i].toMap(),m_endFaces[i].toMap(),normals[i],t))}});
        }uploadFaces();return;
    }
    m_faces.clear();for(int i=0;i<m_startFaces.size();i++){
        const auto a=m_startFaces[i].toMap(),b=m_endFaces[i].toMap();const auto av=a["vertices"].toList(),bv=b["vertices"].toList();QVariantList points;QVector3D center;
        for(int j=0;j<av.size();j++){const auto p=av[j].value<QVector3D>()*(1-t)+bv[j].value<QVector3D>()*t;points.append(QVariant::fromValue(p));center+=p;}
        center=a["center"].value<QVector3D>()*(1-t)+b["center"].value<QVector3D>()*t;
        const auto an=a["normal"].value<QVector3D>(),bn=b["normal"].value<QVector3D>();const auto normal=QQuaternion::slerp(QQuaternion(),QQuaternion::rotationTo(an,bn),t).rotatedVector(an).normalized();
        m_faces.append(QVariantMap{{"vertices",points},{"center",QVariant::fromValue(center)},{"normal",QVariant::fromValue(normal)},{"up",QVariant::fromValue(blendedUp(a,b,normal,t))}});
    }uploadFaces();
}
