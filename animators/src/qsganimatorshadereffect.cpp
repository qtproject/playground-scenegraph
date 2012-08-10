/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of %MODULE%.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsganimatorshadereffect.h"
#include "qsganimatornode.h"

class QSGShaderEffectAnimatorNode : public QSGAnimatorNode
{
public:
    QSGShaderEffectAnimatorNode(QSGAnimatorShaderEffect *item)
        : QSGAnimatorNode(item)
        , m_shaderEffectNode(0)
    {
    }

    ~QSGShaderEffectAnimatorNode()
    {
    }

    void preprocess()
    {
        QSGAnimatorNode::preprocess();
        if (m_controller->isInitialized()) {
            QQuickShaderEffectMaterial *material = static_cast<QQuickShaderEffectMaterial *>(m_shaderEffectNode->material());

            for (int shaderType = 0; shaderType < Key::ShaderTypeCount; ++shaderType) {
                for (int i = 0; i < material->uniforms[shaderType].size(); ++i) {
                    QQuickShaderEffectMaterial::UniformData &d = material->uniforms[shaderType][i];
                    QSGAnimatedProperty *ap = m_controller->registeredProperty(d.name);
                    if (ap) d.value = ap->value();
                }
            }

            m_shaderEffectNode->markDirty(QSGNode::DirtyMaterial);
        }
    }

    void setShaderEffectNode(QQuickShaderEffectNode *n)
    {
        m_shaderEffectNode = n;
    }

private:
    typedef QQuickShaderEffectMaterialKey Key;
    QQuickShaderEffectNode *m_shaderEffectNode;
};

QSGAnimatorShaderEffect::QSGAnimatorShaderEffect(QQuickItem *parent)
    : QQuickShaderEffect(parent)
    , m_animatorNode(0)
{
    setFlag(ItemHasContents);
}

QSGAnimatorShaderEffect::~QSGAnimatorShaderEffect()
{
}

QSGNode *QSGAnimatorShaderEffect::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    QSGTransformNode *transformNode = static_cast<QSGTransformNode*>(QQuickItemPrivate::get(this)->itemNode());
    QSGOpacityNode *opacityNode = static_cast<QSGOpacityNode*>(QQuickItemPrivate::get(this)->opacityNode());
    m_shaderEffectNode = static_cast<QQuickShaderEffectNode*>(QQuickShaderEffect::updatePaintNode(node, data));

    if (!node) {
        if (m_shaderEffectNode) {
            m_animatorNode = new QSGShaderEffectAnimatorNode(this);
            m_animatorNode->setFlag(QSGNode::UsePreprocess, true);
            m_shaderEffectNode->appendChildNode(m_animatorNode);
        }
    }

    if (m_animatorNode) {
        m_animatorNode->controller().sync();
        m_animatorNode->setTransformNode(transformNode);
        m_animatorNode->setOpacityNode(opacityNode);
        m_animatorNode->setShaderEffectNode(m_shaderEffectNode);
    }

    return m_shaderEffectNode;
}
