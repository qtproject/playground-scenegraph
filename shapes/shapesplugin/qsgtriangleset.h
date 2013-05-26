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

#ifndef QSGTRIANGLESET_H
#define QSGTRIANGLESET_H

#include <QObject>

#include <QtGui/QPainterPath>

#include <QtQuick/qsggeometry.h>
#include <QtQuick/QQuickItem>

class QSGTriangleSet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int drawingMode READ drawingMode WRITE setDrawingMode NOTIFY changed)

public:
    enum VertexType {
        IndexedPoint2D
    };

    QSGTriangleSet();

    QVector<quint16> indices() const { return m_indices; }
    QVector<QSGGeometry::Point2D> vertices2D() const { return m_vertices; }

    int drawingMode() const { return m_mode; }
    void setDrawingMode(int drawingMode);

    VertexType vertexType() const { return m_vertexType; }
    void setVertexType(VertexType type);

    bool isValid() const { return m_indices.size() > 0 && m_vertices.size() > 0; }

public slots:
    QString readFile(const QUrl &name);
    QString saveFile(const QUrl &name);

    void beginPathConstruction();
    void moveTo(qreal x, qreal y);
    void lineTo(qreal x, qreal y);
    void quadTo(qreal cx, qreal cy, qreal ex, qreal ey);
    void cubicTo(qreal cx1, qreal cy1, qreal cx2, qreal cy2, qreal ex, qreal ey);
    void addEllipse(qreal x, qreal y, qreal width, qreal height);
    void addRect(qreal x, qreal y, qreal width, qreal height);
    void addRoundedRect(qreal x, qreal y, qreal width, qreal height, qreal xrad, qreal yrad);
    void setWindingMode(bool winding);
    void closeSubpath();
    void clipToRect(qreal x, qreal y, qreal width, qreal height);
    void finishPathConstruction();

signals:
    void changed();

private:
    int m_mode;
    VertexType m_vertexType;
    QVector<quint16> m_indices;
    QVector<QSGGeometry::Point2D> m_vertices;

    QPainterPath m_path;
};

QML_DECLARE_TYPE(QSGTriangleSet)

#endif // QSGTRIANGLESET_H
