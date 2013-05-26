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

#ifndef QSGMASKEDIMAGE_H
#define QSGMASKEDIMAGE_H

#include <private/qquickimagebase_p.h>

#include <QtGui/QImage>

#include "qsgtriangleset.h"

class QSGMaskedImage : public QQuickImageBase
{
    Q_OBJECT

public:
    QSGMaskedImage(QQuickItem *parent = 0);
    ~QSGMaskedImage();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    void updatePolish();

    void pixmapChange();

public slots:
    void sizeWasChanged();

signals:
    void sourceChanged(QUrl source);

private:
    QSGTriangleSet *m_opaqueTriangles;
    QSGTriangleSet *m_alphaTriangles;

    uint m_sourceChanged : 1;
    uint m_explicitSize : 1;
};

QML_DECLARE_TYPE(QSGMaskedImage)

#endif // QSGMASKEDIMAGE_H

