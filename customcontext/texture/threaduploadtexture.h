#ifndef THREADUPLOADTEXTURE_H
#define THREADUPLOADTEXTURE_H

#include <QObject>
#include <QQuickTextureFactory>

class QOpenGLContext;
class QWindow;

namespace CustomContext {

class ThreadUploadTextureFactory;

class ThreadUploadTextureManager : public QObject
{
    Q_OBJECT
public:
    ThreadUploadTextureManager();
    ~ThreadUploadTextureManager();

    QQuickTextureFactory *create(const QImage &image);

public slots:
    void initialized(QOpenGLContext *context);
    void invalidated();

private:
    QOpenGLContext *m_sgGLContext;
    QOpenGLContext *m_context;
    QWindow *m_dummySurface;

    QList<ThreadUploadTextureFactory *> m_factories;
};

}

#endif // THREADUPLOADTEXTURE_H
