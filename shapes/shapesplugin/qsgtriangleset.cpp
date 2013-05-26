/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scene Graph Playground module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgtriangleset.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

#include <QtGui/QTransform>

#include <private/qpolygonclipper_p.h>

#include <private/qtriangulator_p.h>

// #define QSGTRIANGLESET_DEBUG

QSGTriangleSet::QSGTriangleSet()
    : QObject()
    , m_mode(GL_TRIANGLES)
    , m_vertexType(IndexedPoint2D)
{
}


void QSGTriangleSet::setDrawingMode(int drawingMode)
{
    if (m_mode == drawingMode)
        return;

    switch (drawingMode) {
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_LINES:
        m_mode = drawingMode;
        break;
    default:
        qWarning("QSGTriangleSet::setDrawingMode: mode is unsupported: %d", drawingMode);
        return;
    }

    emit changed();
}

void QSGTriangleSet::setVertexType(VertexType type)
{
    m_vertexType = type;
    emit changed();
}

/*
 * Reads the file and returns an empty string if it fails.
 */

QString QSGTriangleSet::readFile(const QUrl &name)
{
    QFile file(name.isRelative() ? name.toString() : name.toLocalFile());
    if (!file.open(QFile::ReadOnly))
        return file.errorString() + QStringLiteral(", ") + QFileInfo(file).absoluteFilePath();

    QTextStream stream(&file);

    QByteArray tag, version;

    stream >> tag;
    stream >> version;
    if (tag != "qsgtriangleset")
        return "not a triangleset file";
    if (version != "0.1")
        return "invalid version, only 0.1 currently supported";

    stream >> tag;
    QByteArray value;

    int drawingMode = -1;
    int vertexType = -1;

    // Parse the rest of the header...
    while (tag != "v" && tag != "V") {
        if (tag == "drawing-mode") {
            stream >> value;
            if (value == "triangle-strip") drawingMode = GL_TRIANGLE_STRIP;
            else if (value == "triangle-fan") drawingMode = GL_TRIANGLE_FAN;
            else if (value == "triangles") drawingMode = GL_TRIANGLES;
            else if (value == "lines") drawingMode = GL_LINES;
            else return "bad drawing-mode: " + value;
        } else if (tag == "data-type") {
            stream >> value;
            if (value == "indexed-point-2d") vertexType = IndexedPoint2D;
            else return "bad data-type: " + value;
        }

        stream >> tag;

        if (stream.atEnd())
            return "premature end of file while parsing headers...";
    }

    if (drawingMode == -1)
        return QStringLiteral("missing drawing-mode tag...");
    if (vertexType == -1)
        return QStringLiteral("missing vertex-type tag...");

    QVector<QSGGeometry::Point2D> vertices;

    // Parse vertices...
    while (tag == "V" || tag == "v") {
        QSGGeometry::Point2D p;
        stream >> p.x >> p.y;
        vertices << p;
        stream >> tag;
    }
    if (vertices.size() == 0)
        return "no vertices";

    QVector<quint16> indices;
    while (tag == "I" || tag == "i") {
        quint16 i;
        stream >> i;
        indices << i;
        stream >> tag;
    }

    if (indices.size() == 0)
        return "no indices...";

    m_indices = indices;
    m_vertices = vertices;
    m_mode = drawingMode;
    m_vertexType = (VertexType) vertexType;

    emit changed();

    return QString();
}

QString QSGTriangleSet::saveFile(const QUrl &name)
{
    QFile file(name.isRelative() ? name.toString() : name.toLocalFile());
    if (!file.open(QFile::WriteOnly))
        return QStringLiteral("failed to open file, ") + QFileInfo(file).absoluteFilePath();

    QTextStream stream(&file);

    stream << "qsgtriangleset   0.1" << endl
           << "drawing-mode     ";
    if (m_mode == GL_TRIANGLES)
        stream << "triangles";
    else if (m_mode == GL_TRIANGLE_STRIP)
        stream << "triangle-strip";
    else if (m_mode == GL_TRIANGLE_FAN)
        stream << "triangle-fan";
    else if (m_mode == GL_LINES)
        stream << "lines";
    stream << endl << "data-type        indexed-point-2d" << endl;

    for (int i=0; i<m_vertices.size(); ++i)
        stream << "V " << m_vertices.at(i).x << " " << m_vertices.at(i).y << endl;
    for (int i=0; i<m_indices.size(); ++i)
        stream << "I " << m_indices.at(i) << endl;

    return QString();
}

void QSGTriangleSet::beginPathConstruction()
{
    m_path = QPainterPath();
}

void QSGTriangleSet::moveTo(qreal x, qreal y)
{
    m_path.moveTo(x, y);
}

void QSGTriangleSet::lineTo(qreal x, qreal y)
{
    m_path.lineTo(x, y);
}

void QSGTriangleSet::quadTo(qreal cx, qreal cy, qreal ex, qreal ey)
{
    m_path.quadTo(cx, cy, ex, ey);
}

void QSGTriangleSet::cubicTo(qreal cx1, qreal cy1, qreal cx2, qreal cy2, qreal ex, qreal ey)
{
    m_path.cubicTo(cx1, cy1, cx2, cy2, ex, ey);
}

void QSGTriangleSet::addEllipse(qreal x, qreal y, qreal width, qreal height)
{
    m_path.addEllipse(x, y, width, height);
}

void QSGTriangleSet::addRect(qreal x, qreal y, qreal width, qreal height)
{
    m_path.addRect(x, y, width, height);
}

void QSGTriangleSet::addRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal xrad, qreal yrad)
{
    m_path.addRoundedRect(x, y, width, height, xrad, yrad);
}

void QSGTriangleSet::closeSubpath()
{
    m_path.closeSubpath();
}

void QSGTriangleSet::setWindingMode(bool winding)
{
    m_path.setFillRule(winding ? Qt::WindingFill : Qt::OddEvenFill);
}

void QSGTriangleSet::clipToRect(qreal x, qreal y, qreal width, qreal height)
{
    QPainterPath bounds;
    bounds.addRect(x, y, width, height);
    m_path = m_path.intersected(bounds);
}

void QSGTriangleSet::finishPathConstruction()
{
    qreal scale = 100;

    QTriangleSet ts = qTriangulate(m_path, QTransform::fromScale(scale, scale));

    m_mode = GL_TRIANGLES;

    m_vertices.resize(ts.vertices.size() / 2);
    QSGGeometry::Point2D *vdst = m_vertices.data();
    const qreal *vdata = ts.vertices.constData();
    for (int i=0; i<ts.vertices.size() / 2; ++i) {
        vdst[i].set(vdata[i*2] / scale, vdata[i*2+1] / scale);
#ifdef QSGTRIANGLESET_DEBUG
        qDebug() << "vertex: " << vdst[i].x << vdst[i].y;
#endif
    }

    m_indices.resize(ts.indices.size());
    quint16 *idst = m_indices.data();
    if (ts.indices.type() == QVertexIndexVector::UnsignedShort) {
        memcpy(idst, ts.indices.data(), m_indices.size() * sizeof(quint16));
#ifdef QSGTRIANGLESET_DEBUG
        for (int i=0; i<m_indices.size(); ++i) {
            qDebug() << "index(16): " << idst[i];
        }
#endif
    } else {
        const quint32 *isrc = (const quint32 *) ts.indices.data();
        for (int i=0; i<m_indices.size(); ++i) {
            idst[i] = isrc[i];
#ifdef QSGTRIANGLESET_DEBUG
            qDebug() << "index: " << idst[i];
#endif
        }
    }

    emit changed();

}

