#ifndef ORDEREDDITHER2X2_H
#define ORDEREDDITHER2X2_H

#include <QOpenGLContext>
#include <QOpenGLShaderProgram>

namespace CustomContext
{

class OrderedDither2x2
{
public:
    OrderedDither2x2(QOpenGLContext *context)
        : context(context)
        , id_size(0)
        , id_texture(0)
        , prepared(false)
    {
    }

    void prepare();
    void draw(int width, int height);

private:
    QOpenGLShaderProgram program;
    QOpenGLContext *context;
    int id_size;
    GLuint id_texture;
    uint prepared : 1;
};

}

#endif // ORDEREDDITHER2X2_H
