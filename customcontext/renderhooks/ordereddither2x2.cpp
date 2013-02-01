#include "ordereddither2x2.h"

#include <QOpenGLFunctions>

namespace CustomContext {

void OrderedDither2x2::prepare()
{
    if (prepared)
        return;
    prepared = true;

    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    "attribute highp vec2 pos;\n"
                                    "attribute highp vec2 texCoord;\n"
                                    "varying highp vec2 tex;\n"
                                    "void main() {\n"
                                    "    gl_Position = vec4(pos.x, pos.y, 0, 1);\n"
                                    "    tex = texCoord;\n"
                                    "}");
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    "uniform lowp sampler2D texture;"
                                    "varying highp vec2 tex;\n"
                                    "void main() {\n"
                                    "    gl_FragColor = texture2D(texture, tex);\n"
                                    "}\n");

    program.bindAttributeLocation("pos", 0);
    program.bindAttributeLocation("texCoord", 1);

    program.link();
    program.setUniformValue("texture", 0);

    glGenTextures(1, &id_texture);
    glBindTexture(GL_TEXTURE_2D, id_texture);

    int data[4] = {
        0x00000000,
        0x02020202,
        0x03030303,
        0x01010101
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OrderedDither2x2::draw(int width, int height)
{
    program.bind();
    program.enableAttributeArray(0);
    program.enableAttributeArray(1);

    float posData[] = { -1, 1, 1, 1, -1, -1, 1, -1 };
    program.setAttributeArray(0, GL_FLOAT, posData, 2);

    float w = width/2;
    float h = height/2;
    float texData[] = { 0, 0, w, 0, 0, h, w, h };
    program.setAttributeArray(1, GL_FLOAT, texData, 2);

    context->functions()->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_texture);

    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program.disableAttributeArray(0);
    program.disableAttributeArray(1);
    program.release();
}

} // end of namespace
