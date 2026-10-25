// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
.pragma library

function multiply(a, b) {
    return Qt.quaternion(
        a.scalar*b.scalar-a.x*b.x-a.y*b.y-a.z*b.z,
        a.scalar*b.x+a.x*b.scalar+a.y*b.z-a.z*b.y,
        a.scalar*b.y-a.x*b.z+a.y*b.scalar+a.z*b.x,
        a.scalar*b.z+a.x*b.y-a.y*b.x+a.z*b.scalar);
}
function normalized(q) {
    const length = Math.sqrt(q.scalar*q.scalar+q.x*q.x+q.y*q.y+q.z*q.z);
    if (!Number.isFinite(length) || length < 1e-12) return Qt.quaternion(1,0,0,0);
    return Qt.quaternion(q.scalar/length,q.x/length,q.y/length,q.z/length);
}
function rotate(current, pitchStep, yawStep) {
    // Camera is fixed, looking along -Z. Apply the drag in camera/screen axes,
    // before the existing orientation, rather than editing local Euler angles.
    const angle = Math.sqrt(pitchStep*pitchStep+yawStep*yawStep);
    if (angle < 1e-12) return current;
    const half = angle*Math.PI/360;
    const factor = Math.sin(half)/angle;
    const delta = Qt.quaternion(Math.cos(half),pitchStep*factor,yawStep*factor,0);
    return normalized(multiply(delta,current));
}
function initialOrientation() {
    return rotate(rotate(Qt.quaternion(1,0,0,0),-23,0),0,17);
}

function faceForward(current, normal) {
    const v = Qt.vector3d(current.x,current.y,current.z);
    const n = normal.plus(v.crossProduct(normal).times(2*current.scalar)).plus(v.crossProduct(v.crossProduct(normal)).times(2));
    const dot = Math.max(-1,Math.min(1,n.z));
    const delta = dot < -0.999999 ? Qt.quaternion(0,0,1,0) : normalized(Qt.quaternion(1+dot,n.y,-n.x,0));
    return normalized(multiply(delta,current));
}

function rotated(q, p) {
    const v=Qt.vector3d(q.x,q.y,q.z);
    return p.plus(v.crossProduct(p).times(2*q.scalar)).plus(v.crossProduct(v.crossProduct(p)).times(2));
}
function uprightFace(normal) {
    // Use the identical local up vector as FaceGeometry's texture coordinates.
    // Match the native float(0.95) pole threshold exactly.
    const preferred=Math.abs(normal.y)<0.949999988079071?Qt.vector3d(0,1,0):Qt.vector3d(0,0,-1);
    const up=preferred.minus(normal.times(preferred.dotProduct(normal))).normalized();
    const forward=faceForward(Qt.quaternion(1,0,0,0),normal);
    const screenUp=rotated(forward,up);
    const half=Math.atan2(screenUp.x,screenUp.y)/2;
    return normalized(multiply(Qt.quaternion(Math.cos(half),0,0,Math.sin(half)),forward));
}
