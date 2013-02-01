#include "threaduploadtexture.h"

#include <QtCore/qdebug.h>
#include <QtCore/QThread>

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <QtQuick/QQuickWindow>

#include <private/qquickpixmapcache_p.h>
#include <private/qsgtexture_p.h>

namespace CustomContext {

class ThreadUploadTextureFactory : public QQuickTextureFactory
{
public:
    ThreadUploadTextureFactory(const QImage &i, QSGTexture *t)
        : m_image(i)
        , m_texture(t)
    {
    }

    QSGTexture *createTexture(QQuickWindow *window) const {
        // In the odd case where we were not initialized...
        if (!m_texture)
            m_texture = window->createTextureFromImage(m_image);
        return m_texture;
    }

    QSize textureSize() const { return m_image.size(); }
    int textureByteCount() const { return m_image.byteCount(); }
    QImage image() const { return m_image; }

    void resetTexture() {
        // We don't need to delete it because the scene graph will handle that part.
        m_texture = 0;
    }

private:
    QImage m_image;
    mutable QSGTexture *m_texture;
};



ThreadUploadTextureManager::ThreadUploadTextureManager()
    : m_sgGLContext(0)
    , m_context(0)
    , m_dummySurface(0)
{
}



ThreadUploadTextureManager::~ThreadUploadTextureManager()
{
    Q_ASSERT_X(!m_context, "ThreadUploadTextureManager::~ThreadUploadTextureManager", "GL context should have been deleted in invalidate!");
    delete m_dummySurface;
}



void ThreadUploadTextureManager::invalidated()
{
    // After an invalidate, all textures need to "come back" normally. That is a bit
    // unfortunate. We could maybe post an event to them to re-upload them, but
    // then we need to have a lock in createTexture(). Need to prove that it
    // is worth it.
    for (int i=0; i<m_factories.size(); ++i) {
        ThreadUploadTextureFactory *factory = m_factories[i];
        factory->resetTexture();
    }
    // Make sure the context is deleted on the "right" thread
    m_context->deleteLater();
    m_context = 0;
    m_sgGLContext = 0;
}



void ThreadUploadTextureManager::initialized(QOpenGLContext *context)
{
    m_sgGLContext = context;
}



QQuickTextureFactory *ThreadUploadTextureManager::create(const QImage &image)
{
    QSGPlainTexture *texture = 0;

    // If the QSGContext was not initialized yet, return a plain texture factory.
    if (m_sgGLContext) {
        if (!m_context) {
            m_context = new QOpenGLContext();
            m_context->setShareContext(m_sgGLContext);
            m_context->create();
        }
        if (!m_dummySurface) {
            m_dummySurface = new QWindow();
            m_dummySurface->setSurfaceType(QSurface::OpenGLSurface);
            m_dummySurface->resize(64, 64);
            m_dummySurface->create();
        }
        if (m_context->surface() != m_dummySurface) {
            m_context->makeCurrent(m_dummySurface);
        }
        texture = new QSGPlainTexture();
        texture->setImage(image);
        texture->bind();
    }

    ThreadUploadTextureFactory *factory = new ThreadUploadTextureFactory(image, texture);
    m_factories << factory;
    return factory;
}

} // end of namespace

