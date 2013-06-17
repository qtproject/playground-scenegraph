/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
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

#ifndef OVERLAPRENDERER_H
#define OVERLAPRENDERER_

#include <private/qsgrenderer_p.h>

#include <QtGui/private/qdatabuffer_p.h>
#ifdef USE_HALF_FLOAT
#include <GLES2/gl2ext.h>
#endif

class QSGRenderNode;

namespace OverlapRenderer {

struct VertexFormat
{
    struct Attribute
    {
        int inputRegister;
        int byteOffset;
        int componentCount;
        int dataType;
    };

    bool matches(const QSGGeometry::Attribute *otherAttributes, int otherAttrCount, int otherStride)
    {
        if (attributeCount != otherAttrCount)
            return false;

#ifndef USE_HALF_FLOAT
        if (stride != otherStride)
            return false;
#endif

        for (int i=0 ; i < attributeCount ; ++i) {
            bool goodReg = attributes[i].inputRegister == otherAttributes[i].position;
#ifdef USE_HALF_FLOAT
            bool goodType = (attributes[i].dataType == otherAttributes[i].type) || (attributes[i].dataType == GL_HALF_FLOAT_OES && otherAttributes[i].type == GL_FLOAT);
#else
            bool goodType = (attributes[i].dataType == otherAttributes[i].type);
#endif
            bool goodCount = attributes[i].componentCount == otherAttributes[i].tupleSize;

            if (!goodReg || !goodType || !goodCount)
                return false;
        }

        return true;
    }

    static inline int componentSize(int type)
    {
#ifdef USE_HALF_FLOAT
        if (type == GL_HALF_FLOAT_OES)
            return 2;
#endif

        static int sizes[] = {
            sizeof(char),
            sizeof(unsigned char),
            sizeof(short),
            sizeof(unsigned short),
            sizeof(int),
            sizeof(unsigned int),
            sizeof(float),
            2,
            3,
            4,
            sizeof(double)
        };
        Q_ASSERT(type >= GL_BYTE && type <= 0x140A); // the value of GL_DOUBLE
        return sizes[type - GL_BYTE];
    }

    VertexFormat(const QSGGeometry::Attribute *attrList, int attrCount)
    {
        positionAttribute = -1;
        stride = 0;
        refCount = -1;

        attributeCount = attrCount;
        attributes = new Attribute[attrCount];

        for (int i=0 ; i < attrCount ; ++i) {
            const QSGGeometry::Attribute &srcAttr = attrList[i];
            VertexFormat::Attribute &destAttr = attributes[i];

            destAttr.inputRegister = srcAttr.position;
            destAttr.byteOffset = stride;
            destAttr.componentCount = srcAttr.tupleSize;
            destAttr.dataType = srcAttr.type;

#ifdef USE_HALF_FLOAT
            if (destAttr.dataType == GL_FLOAT) {
                destAttr.dataType = GL_HALF_FLOAT_OES;
            }
#endif

            if (srcAttr.isVertexCoordinate) {
                Q_ASSERT(positionAttribute == -1);
                positionAttribute = i;
            }

            stride += destAttr.componentCount * componentSize(destAttr.dataType);
        }
    }

    ~VertexFormat()
    {
        delete [] attributes;
    }

    Attribute *attributes;
    int attributeCount;
    int positionAttribute;
    int stride;
    int refCount;
};

// Using this instead of QVector2D so that accessors aren't required for x() and y(), as that makes debug builds much slower.
struct Vec2
{
    float x, y;

    Vec2(qreal x_ = 0.0, qreal y_ = 0.0) : x(x_), y(y_) {}

    void set(qreal x_, qreal y_)
    {
        x = x_;
        y = y_;
    }

    Vec2 operator+(const Vec2 &o)
    {
        Vec2 r(x + o.x, y + o.y);
        return r;
    }

    Vec2 operator*(qreal s)
    {
        Vec2 r(x * s, y * s);
        return r;
    }

    Vec2 operator*(const QMatrix4x4 &mat) const
    {
        Vec2 r;

        const float *m = mat.constData();

        r.x = x * m[0] + y * m[4] + m[12];
        r.y = x * m[1] + y * m[5] + m[13];

        return r;
    }

};

struct Rect
{
    Vec2 minPoint, maxPoint;

    bool intersects(const Rect &r)
    {
        bool xOverlap = r.minPoint.x < maxPoint.x && r.maxPoint.x > minPoint.x;
        bool yOverlap = r.minPoint.y < maxPoint.y && r.maxPoint.y > minPoint.y;
        return xOverlap && yOverlap;
    }

    Rect &unite(const Rect &r)
    {
        if (minPoint.x > r.minPoint.x)
            minPoint.x = r.minPoint.x;
        if (maxPoint.x < r.maxPoint.x)
            maxPoint.x = r.maxPoint.x;
        if (minPoint.y > r.minPoint.y)
            minPoint.y = r.minPoint.y;
        if (maxPoint.y < r.maxPoint.y)
            maxPoint.y = r.maxPoint.y;
        return *this;
    }

    Rect &intersect(const Rect &r)
    {
        if (minPoint.x < r.minPoint.x)
            minPoint.x = r.minPoint.x;
        if (maxPoint.x > r.maxPoint.x)
            maxPoint.x = r.maxPoint.x;
        if (minPoint.y < r.minPoint.y)
            minPoint.y = r.minPoint.y;
        if (maxPoint.y > r.maxPoint.y)
            maxPoint.y = r.maxPoint.y;
        if (maxPoint.x < minPoint.x)
            maxPoint.x = minPoint.x;
        if (maxPoint.y < minPoint.y)
            maxPoint.y = minPoint.y;
        return *this;
    }

    void transform(const QMatrix4x4 *m, const Rect &r)
    {
        if (m) {
            Vec2 p0 = r.minPoint * *m;
            Vec2 p1 = r.maxPoint * *m;

            minPoint.x = qMin(p0.x, p1.x);
            minPoint.y = qMin(p0.y, p1.y);

            maxPoint.x = qMax(p0.x, p1.x);
            maxPoint.y = qMax(p0.y, p1.y);
        } else {
            *this = r;
        }
    }
};

struct RenderElement
{
    enum PrimitiveType
    {
        PT_Invalid = -1,

        PT_Triangles = GL_TRIANGLES,
        PT_Lines = GL_LINES,
        PT_Points = GL_POINTS
    };

    Rect localRect;
    Rect worldRect;
    QSGNode *basenode;
    VertexFormat *vf;
    int vertexCount, indexCount;
    char *vertices;
    short *indices;
    unsigned short transformDirty : 1;
    unsigned short geometryDirty : 1;
    unsigned short addedToBatch : 1;
    unsigned short usesFullMatrix : 1;
    unsigned short usesDeterminant : 1;
    unsigned short complexTransform : 1;
    short batchConfig;
    int currentVertexBufferSize;
    int currentIndexBufferSize;
    short nextInBatchConfig;
    PrimitiveType primType;
};

struct BatchConfig
{
    BatchConfig() : renderNode(0), type(0), material(0), clip(0), vf(0), opacity(0.0f), primType(0), determinant(0.0f), useMatrix(false), prevElement(0) {}

    QSGRenderNode *renderNode;

    QSGMaterialType *type;
    QSGMaterial *material;
    const QSGClipNode *clip;
    VertexFormat *vf;
    qreal opacity;
    GLenum primType;

    QMatrix4x4 transform;
    qreal determinant;
    bool useMatrix;

    RenderElement *prevElement;
};

struct RenderBatch
{
    RenderBatch(const BatchConfig *c) : cfg(c) {}

    const BatchConfig *cfg;

    std::vector<RenderElement *> elements;
    int vertexCount;
    int indexCount;

    unsigned char *vb, *ib;

    void build();
    void add(RenderElement *e);
};

class Renderer : public QSGRenderer
{
    Q_OBJECT
public:
    Renderer(QSGContext *context);
    ~Renderer();

    void render();
    void nodeChanged(QSGNode *node, QSGNode::DirtyState flags);

    void setClipProgram(QOpenGLShaderProgram *program, int matrixID);

private:

    void buildRenderOrderList(QSGNode *node);
    void buildBatches();
    void drawBatches();
    void clearBatches();

    void addNode(QSGNode *node);
    void removeNode(QSGNode *node);

    void dirtyChildNodes_Transform(QSGNode *node);
    void updateElementGeometry(RenderElement *__restrict e);
    void updateElementTransform(RenderElement *__restrict e, const QMatrix4x4 *mat);

    bool isOverlap(int i, int j, const Rect &r);

    VertexFormat *getVertexFormat(const QSGGeometry::Attribute *attributes, int attrCount, int stride);
    void releaseVertexFormat(VertexFormat *vf);

    void buildDrawLists();

    Rect m_viewportRect;
    QHash<QSGNode *, RenderElement *> m_elementHash;
    std::vector<RenderElement *> m_elementsInRenderOrder;
    std::vector<VertexFormat *> m_vertexFormats;
    std::vector<RenderBatch> m_batches;

    QOpenGLShaderProgram *clipProgram;
    int clipMatrixID;

    int minBatchConfig;
    std::vector<BatchConfig> batchConfigs;

    void addElementToBatchConfig(RenderElement *e);
};

} // end namespace

#endif // OVERLAPRENDERER_
