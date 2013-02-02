#ifndef MACTEXTURE_H
#define MACTEXTURE_H

#include <QSGTexture>

#include <QtGui/qopengl.h>

namespace CustomContext {

class MacTexture : public QSGTexture
{
    Q_OBJECT
public:
    MacTexture(const QImage &image);
    ~MacTexture();

    void bind();

    int textureId() const { return m_textureId; }
    QSize textureSize() const { return m_image.size(); }
    bool hasAlphaChannel() const { return m_image.hasAlphaChannel(); }
    bool hasMipmaps() const { return false; }

private:
    QImage m_image;

    GLuint m_textureId;
};

}

#endif // MACTEXTURE_H
