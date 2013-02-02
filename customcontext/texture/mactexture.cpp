#include "mactexture.h"

#include <QtCore/QElapsedTimer>

#include <QtGui/QOpenGLContext>

#ifndef QSG_NO_RENDERER_TIMING
static bool qsg_renderer_timing = !qgetenv("QML_RENDERER_TIMING").isEmpty();
static QElapsedTimer qsg_renderer_timer;
#endif

namespace CustomContext {



MacTexture::MacTexture(const QImage &image)
    : m_image(image)
    , m_textureId(0)
{
}



MacTexture::~MacTexture()
{
    glDeleteTextures(1, &m_textureId);
}



void MacTexture::bind()
{
    bool forceBindOptions = false;

    if (!m_textureId) {
#ifndef QSG_NO_RENDERER_TIMING
        if (qsg_renderer_timing)
            qsg_renderer_timer.start();
#endif
        forceBindOptions = true;

        glGenTextures(1, &m_textureId);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_SHARED_APPLE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0, GL_BGRA,
                     GL_UNSIGNED_INT_8_8_8_8_REV, m_image.constBits());

#ifndef QSG_NO_RENDERER_TIMING
    if (qsg_renderer_timing)
        printf("   - mactexture(%dx%d) total=%d\n",
               m_image.width(),
               m_image.height(),
               (int) qsg_renderer_timer.elapsed());
#endif

    } else {

        glBindTexture(GL_TEXTURE_2D, m_textureId);
    }

    updateBindOptions(forceBindOptions);

}

}
