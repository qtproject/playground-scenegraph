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

#include "qsgmaskedimage.h"
#include <QtQml/QQmlFile>

#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGTexture>
#include <QtQuick/QSGTextureMaterial>
#include <QtQuick/QQuickWindow>

#include <private/qquickpixmapcache_p.h>
#include <private/qquickimagebase_p_p.h>

namespace QSGMaskedImageNS
{

class Mesh : public QSGGeometryNode
{
public:

    Mesh()
        : geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0, 0)
    {
        setGeometry(&geometry);
        setMaterial(&material);
        setOpaqueMaterial(&opaqueMaterial);
        setFlags(OwnedByParent | OwnsOpaqueMaterial | OwnsMaterial | OwnsGeometry, false);
        geometry.setDrawingMode(GL_TRIANGLE_STRIP);
    }

    void setTexture(QSGTexture *texture) {
        opaqueMaterial.setTexture(texture);
        material.setTexture(texture);
    }

    void setTriangles(QSGTriangleSet *t) {
        Q_ASSERT_X(t->vertexType() == QSGTriangleSet::IndexedPoint2D, "QSGMaskedImage", "only IndexedPoint2D is supported by masked image");

        QVector<QSGGeometry::Point2D> vertices = t->vertices2D();
        QVector<quint16> indices = t->indices();
        geometry.allocate(vertices.size(), indices.size());
        geometry.setDrawingMode(t->drawingMode());

        QSGGeometry::TexturedPoint2D *v = geometry.vertexDataAsTexturedPoint2D();
        for (int i=0; i<vertices.size(); ++i) {
            const QSGGeometry::Point2D &pt = vertices.at(i);
            v[i].set(pt.x, pt.y, pt.x, pt.y);
        }

        quint16 *in = geometry.indexDataAsUShort();
        memcpy(in, indices.constData(), indices.size() * sizeof(quint16));
    }

    QSGOpaqueTextureMaterial opaqueMaterial;
    QSGTextureMaterial material;
    QSGGeometry geometry;
};

class Root : public QSGTransformNode
{
public:
    Root()
    {
#ifdef QML_RUNTIME_TESTING
        description = "MaskedImage";
        opaque.description = QStringLiteral("MaskedImage.opaque");
        alpha.description = QStringLiteral("MaskedImage.alpha");
#endif
    }

    void setTexture(QSGTexture *t) {
        alpha.setTexture(t);
        opaque.setTexture(t);
        // Disable blending on this node as setTexture will set it to true.
        opaque.opaqueMaterial.setFlag(QSGMaterial::Blending, false);
        opaque.material.setFlag(QSGMaterial::Blending, false);
    }

    Mesh opaque;
    Mesh alpha;
};



class MaskStore : public QObject
{
    Q_OBJECT

public:
    static QSGTriangleSet *lookup(QQuickTextureFactory *factory, const QString &url, bool opaque);

public slots:
    void factoryDestroyed(QObject *o);

private:
    QHash<qintptr, QSGTriangleSet *> store;

    static MaskStore instance;
};

MaskStore MaskStore::instance;

QSGTriangleSet *MaskStore::lookup(QQuickTextureFactory *factory, const QString &url, bool opaque)
{
    qintptr key = (qintptr) factory;
    if (opaque)
        key += 1;

    if (instance.store.contains(key))
        return instance.store.value(key);

    QSGTriangleSet *set = new QSGTriangleSet();
    QString res = set->readFile(url);
    if (res.length() > 0) {
        qDebug() << "Failed to read" << url << "," << res;
        delete set;
        set = 0;
    }

    // Only connect to opaque to avoid connecting twice...
    if (opaque)
        connect(factory, SIGNAL(destroyed(QObject *)), &instance, SLOT(factoryDestroyed(QObject *)));

    instance.store[key] = set;
    return set;
}

void MaskStore::factoryDestroyed(QObject *o)
{
    QQuickTextureFactory *f = static_cast<QQuickTextureFactory *>(o);
    store.remove(qintptr(f));
    store.remove(qintptr(f) + 1);
}

}

using namespace QSGMaskedImageNS;


QSGMaskedImage::QSGMaskedImage(QQuickItem *parent)
    : QQuickImageBase(parent)
    , m_opaqueTriangles(0)
    , m_alphaTriangles(0)
    , m_sourceChanged(false)
{
    setFlag(ItemHasContents, true);

    connect(this, SIGNAL(widthChanged()), this, SLOT(sizeWasChanged()));
    connect(this, SIGNAL(heightChanged()), this, SLOT(sizeWasChanged()));
}

QSGMaskedImage::~QSGMaskedImage()
{

}

void QSGMaskedImage::pixmapChange()
{
    m_sourceChanged = true;
    polish();
    QQuickImageBase::pixmapChange();
}

void QSGMaskedImage::updatePolish()
{
    if (m_sourceChanged) {
        QString baseName = QQmlFile::urlToLocalFileOrQrc(source());
        m_opaqueTriangles = MaskStore::lookup(static_cast<QQuickImageBasePrivate *>(d_ptr.data())->pix.textureFactory(),
                                              baseName + QStringLiteral(".opaquemask"),
                                              true);
        m_alphaTriangles = MaskStore::lookup(static_cast<QQuickImageBasePrivate *>(d_ptr.data())->pix.textureFactory(),
                                             baseName + QStringLiteral(".alphamask"),
                                             false);
    }

    update();
}

void QSGMaskedImage::sizeWasChanged()
{
    update();
}

QSGNode *QSGMaskedImage::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    Root *node = static_cast<Root *>(old);

    if (m_sourceChanged) {
        QQuickPixmap *pix = &static_cast<QQuickImageBasePrivate *>(d_ptr.data())->pix;
        delete node;
        if (pix->isNull())
            return 0;

        node = new Root();

        QSGTexture *t = pix->textureFactory()->createTexture(window());
        node->setTexture(t);
        m_sourceChanged = false;

        if (m_alphaTriangles && m_alphaTriangles->isValid()) {
            node->alpha.setTriangles(m_alphaTriangles);
            node->appendChildNode(&node->alpha);
        }

        if (m_opaqueTriangles && m_opaqueTriangles->isValid()) {
            node->opaque.setTriangles(m_opaqueTriangles);
            node->appendChildNode(&node->opaque);
        }
    }

    if (!node)
        return 0;

    QSGTexture::Filtering filter = smooth() ? QSGTexture::Linear : QSGTexture::Nearest;
    node->opaque.material.setFiltering(filter);
    node->opaque.opaqueMaterial.setFiltering(filter);
    node->alpha.material.setFiltering(filter);
    node->alpha.opaqueMaterial.setFiltering(filter);

    QMatrix4x4 m;
    m.scale(width(), height());
    node->setMatrix(m);

    return node;
}

#include "qsgmaskedimage.moc"
