#ifndef NONPRESERVEDTEXTURE_H
#define NONPRESERVEDTEXTURE_H

#include <QSize>
#include <QImage>
#include <QQuickTextureFactory>

namespace CustomContext {

class NonPreservedTextureFactory : public QQuickTextureFactory
{
    Q_OBJECT
public:
    NonPreservedTextureFactory(const QImage &image);

    QSGTexture *createTexture(QQuickWindow *window) const;
    QSize textureSize() const { return m_size; }
    int textureByteCount() const { return m_byteCount; }
    QImage image() const { return m_image; }

private:
    mutable QImage m_image;
    QSize m_size;
    int m_byteCount;
};

}

#endif // NONPRESERVEDTEXTURE_H
