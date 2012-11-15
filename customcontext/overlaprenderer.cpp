/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
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

#define GL_GLEXT_PROTOTYPES

#include "overlaprenderer.h"
#include "qsgmaterial.h"
#include "private/qsgrendernode_p.h"

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qpair.h>
#include <QtCore/QElapsedTimer>

#ifndef DESKTOP_BUILD
#include <arm_neon.h>
#endif

// #define ENABLE_PROFILING

namespace OverlapRenderer
{

#ifdef ENABLE_PROFILING

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QFile>
#include <QDataStream>

class Profiler
{
public:

    enum BinRecordType {
        FullFrameRecord = 1,
        SkippedFrameRecord = 2,
        TimerRecord = 3,
        CounterRecord = 4
    };

    struct ProfileBinaryWriter {
        ProfileBinaryWriter() {
            file = new QFile(QString::fromLatin1("/tmp/%1.fullprofile").arg(getpid()));
            file->open(QIODevice::WriteOnly);
            stream.setDevice(file);
        }

        ~ProfileBinaryWriter() {
            file->close();
            delete file;
        }

        QFile *file;
        QDataStream stream;
    };

    Profiler()
    {
        binaryProfile = NULL;

        enableProfiler = getenv("RENDERER_ENABLE_PROFILING") != NULL;
        takeScreenshot = false;
        fpsBars = false;
        mode = None;

        runningFrameTime.reset();
        runningRenderTime.reset();
        runningFrameCounter.reset();
        totalFrameCounter.reset();

        if (enableProfiler) {
            snprintf(pipename, sizeof(pipename)-1, "/tmp/render_cmd_%d", getpid());
            mkfifo(pipename, 0777);
            pipeFd = open(pipename, O_RDWR | O_NONBLOCK);
        }
    }

    ~Profiler()
    {
        if (enableProfiler) {
            close(pipeFd);
            unlink(pipename);
        }
    }

    enum CounterType
    {
        COUNTER_VerticesTransformed,
        COUNTER_DrawCalls,
        COUNTER_VerticesDrawn,
        COUNTER_IndicesDrawn,
        COUNTER_ClipScissor,
        COUNTER_ClipStencil,
        COUNTER_ClipCached,
        COUNTER_BatchCount,
        COUNTER_SlowElements,
        COUNTER_TotalElements,

        COUNTER_MAX
    };

    enum TimerType
    {
        TIMER_GpuBeginFrame,
        TIMER_UpdateRenderOrder,
        TIMER_BuildDrawLists,
        TIMER_BuildBatches,
        TIMER_GpuEndFrame,
        TIMER_DrawBatches,
        TIMER_ClearBatches,

        TIMER_MAX
    };

    void beginFrame()
    {
        if (enableProfiler) {
            // Check for profiler commands
            char cmd;
            int bytes = read(pipeFd, &cmd, 1);
            if (bytes == 1) {
                switch (cmd) {
                    case 's':
                        takeScreenshot = true;
                        break;
                    case 'p':
                        fpsBars = !fpsBars;
                        break;
                    case 'f':
                        closeBinaryProfile();
                        mode = Full;
                        break;
                    case 'm':
                        closeBinaryProfile();
                        mode = Minimal;
                        break;
                    case 'n':
                        closeBinaryProfile();
                        mode = None;
                        break;
                    case 'b':
                        mode = Binary;
                        break;
                    case 'r':
                        runningFrameCounter.reset();
                        runningFrameTime.reset();
                        runningRenderTime.reset();
                        break;
                    default:
                        printf("overlaprenderer: unknown profiler command\n");
                        break;
                }
            }

            for (int i=0 ; i < COUNTER_MAX ; ++i)
                counters[i].reset();

            renderTimer.start();
        }
    }

    void endFrame()
    {
        if (enableProfiler) {
            renderTimer.stop();
            frameTimer.stop();

            frameMs = nsToMs(frameTimer.ns);
            renderMs = nsToMs(renderTimer.ns);

            if (frameMs < 100.0f) {
                runningFrameTime.add(frameTimer.ns);
                runningRenderTime.add(renderTimer.ns);
                runningFrameCounter.add(1);
            }
            totalFrameCounter.add(1);

            float fRunningFrameCount = (float) runningFrameCounter.value;
            float frameMsAvg = nsToMs(runningFrameTime.value) / fRunningFrameCount;
            float renderMsAvg = nsToMs(runningRenderTime.value) / fRunningFrameCount;

            if (mode == Minimal || mode == Full) {
                printf("== Frame %lld ==\n", totalFrameCounter.value);

                printf("\tFrame:  %.2f ms [average %.2f ms]\n", frameMs, frameMsAvg);
                printf("\tRender: %.2f ms [average %.2f ms]\n", renderMs, renderMsAvg);
            }

            if (mode == Full) {
                printf("\n\tCounters\n");
                for (int i=0 ; i < COUNTER_MAX ; ++i) {
                    Counter *c = &counters[i];
                    printf("\t\t%-24s%5lld\n", counterNames[i], c->value);
                }

                printf("\n\tTimers\n");
                for (int i=0 ; i < TIMER_MAX ; ++i) {
                    Timer *t = &timers[i];
                    float ms = nsToMs(t->ns);
                    printf("\t\t%-20s%.2f ms\n", timerNames[i], ms);
                }
            }

            if (mode == Binary) {
                if (binaryProfile == 0)
                    binaryProfile = new ProfileBinaryWriter();

                binaryProfile->stream << (qint16)FullFrameRecord;
                binaryProfile->stream << (qint32)totalFrameCounter.value;
                binaryProfile->stream << (float)renderMs;
                binaryProfile->stream << (float)renderMsAvg;
                binaryProfile->stream << (float)frameMs;
                binaryProfile->stream << (float) (1000.0f / frameMs);

                for (int i=0 ; i < TIMER_MAX ; ++i) {
                    Timer *t = &timers[i];
                    binaryProfile->stream << (qint16)TimerRecord;
                    binaryProfile->stream.writeBytes(timerNames[i], ::strlen(timerNames[i]));
                    binaryProfile->stream << (float) nsToMs(t->ns);
                }

                for (int i=0 ; i < TIMER_MAX ; ++i) {
                    Counter *c = &counters[i];
                    binaryProfile->stream << (qint16)CounterRecord;
                    binaryProfile->stream.writeBytes(counterNames[i], ::strlen(counterNames[i]));
                    binaryProfile->stream << (quint32)c->value;
                }
            }

            if (takeScreenshot) {
                char filename[128];
                static int screenshotCount;
                snprintf(filename, sizeof(filename)-1, "/tmp/screenshot_%d_%d.tga", getpid(), screenshotCount++);
                writeTga(filename);
                printf("overlaprenderer: saved screenshot %s\n", filename); fflush(stdout);
                takeScreenshot = false;
            }

            if (fpsBars) {
                drawBars();
            }

            frameTimer.start();
        }
    }

    void incrementCounter(CounterType c, int v)
    {
        counters[c].add(v);
    }

    void startTimer(TimerType t)
    {
        if (enableProfiler)
            timers[t].start();
    }

    void stopTimer(TimerType t)
    {
        if (enableProfiler)
            timers[t].stop();
    }

private:

    void closeBinaryProfile() {
        delete binaryProfile;
        binaryProfile = 0;
    }

    enum Mode
    {
        None = 0,
        Full,
        Minimal,
        Binary
    };

    void drawBars()
    {
        if (pb_vbo == 0) {
            GLfloat vboVerts[] =
            {
                 -1.0f, -0.90f, 0.0f,
                  1.0f, -0.90f, 0.0f,
                  1.0f, -0.92f, 0.0f,

                 -1.0f, -0.90f, 0.0f,
                 -1.0f, -0.92f, 0.0f,
                  1.0f, -0.92f, 0.0f,

                 -1.0f, -0.93f, 1.0f,
                  1.0f, -0.93f, 1.0f,
                  1.0f, -0.95f, 1.0f,

                 -1.0f, -0.93f, 1.0f,
                 -1.0f, -0.95f, 1.0f,
                  1.0f, -0.95f, 1.0f,
            };

            const char *vertexShader =
                    "attribute highp vec4 vCoord;                   \n"
                    "uniform float f0;                              \n"
                    "uniform float f1;                              \n"
                    "varying vec3 uv;                               \n"
                    "void main() {                                  \n"
                    "    float x = vCoord.x * 0.5 + 0.5;            \n"
                    "    float f = (vCoord.z == 0.0) ? f0 : f1;     \n"
                    "    uv = vec3(x, 0.0, f);                      \n"
                    "    gl_Position = vec4(vCoord.xy, 0, 1);       \n"
                    "}";

            const char *fragmentShader =
                    "uniform lowp vec4 color;                       \n"
                    "varying vec3 uv;                               \n"
                    "uniform sampler2D diffuse;                     \n"
                    "void main() {                                  \n"
                    "    float s = (uv.x > uv.z) ? 0.3 : 1.0;       \n"
                    "    vec4 texColor = texture2D(diffuse, uv.xy); \n"
                    "    gl_FragColor = vec4(texColor.xyz * s, 1);  \n"
                    "}";

            char texture[320*3];
            int m0 = 320/3;
            int m1 = 2*320/3;
            for (int i=0 ; i < 320 ; ++i) {
                int idx = i*3;
                if (i < m0) {
                    texture[idx+0] = 0;
                    texture[idx+1] = 255;
                    texture[idx+2] = 0;
                } else if (i == m0) {
                    texture[idx+0] = 0;
                    texture[idx+1] = 0;
                    texture[idx+2] = 255;
                } else if (i < m1) {
                    texture[idx+0] = 255;
                    texture[idx+1] = 255;
                    texture[idx+2] = 0;
                } else if (i == m1) {
                    texture[idx+0] = 0;
                    texture[idx+1] = 0;
                    texture[idx+2] = 255;
                } else {
                    texture[idx+0] = 255;
                    texture[idx+1] = 0;
                    texture[idx+2] = 0;
                }
            }

            glGenTextures(1, &pb_diffuse);
            glBindTexture(GL_TEXTURE_2D, pb_diffuse);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 320, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenBuffers(1, &pb_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, pb_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vboVerts), vboVerts, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            pb_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
            pb_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
            pb_shader.bindAttributeLocation("position", 0);
            pb_shader.link();

            pb_f0 = pb_shader.uniformLocation("f0");
            pb_f1 = pb_shader.uniformLocation("f1");
        }

        float f0 = renderMs / 50.0f;
        float f1 = frameMs / 50.0f;

        pb_shader.bind();
        pb_shader.enableAttributeArray(0);
        pb_shader.setUniformValue(pb_f0, f0);
        pb_shader.setUniformValue(pb_f1, f1);

        glBindTexture(GL_TEXTURE_2D, pb_diffuse);

        glBindBuffer(GL_ARRAY_BUFFER, pb_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);
        glDrawArrays(GL_TRIANGLES, 0, 12);

        pb_shader.disableAttributeArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    static void writeTga(const char *filename)
    {
        const int w = 320;
        const int h = 480;

        static unsigned char pixels[w*h*3];
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        // RGB -> BGR
        for (int y=0 ; y < h ; ++y) {
            for (int x=0 ; x < w ; ++x) {
                int index = 3 * (y*w+x);
                unsigned char r = pixels[index];
                pixels[index] = pixels[index+2];
                pixels[index+2] = r;
            }
        }

        FILE *tga = fopen(filename, "wb");
        unsigned char header[] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, w&0xff, w>>8, h&0xff, h>>8, 24, 0 };
        fwrite(header, 1, sizeof(header), tga);
        fwrite(pixels, 1, w*h*3, tga);
        fclose(tga);
    }

    static const char *counterNames[COUNTER_MAX];
    static const char *timerNames[TIMER_MAX];

    static float nsToMs(qint64 ns)
    {
        return ns / 1000000.0f;
    }

    struct Counter
    {
        void add(qint64 v)
        {
            value += v;
        }

        void reset()
        {
            value = 0;
        }

        qint64 value;
    };

    struct Timer
    {
        void start()
        {
            timer.invalidate();
            timer.start();
        }

        void stop()
        {
            ns = timer.nsecsElapsed();
        }

        QElapsedTimer timer;
        qint64 ns;
    };

    ProfileBinaryWriter *binaryProfile;

    bool enableProfiler;
    bool takeScreenshot;
    bool fpsBars;
    Mode mode;

    int pipeFd;
    char pipename[128];

    Counter counters[COUNTER_MAX];
    Timer timers[TIMER_MAX];

    Timer frameTimer;
    Timer renderTimer;

    Counter runningFrameTime;
    Counter runningRenderTime;
    Counter runningFrameCounter;
    Counter totalFrameCounter;

    float renderMs;
    float frameMs;

    GLuint pb_vbo;
    GLuint pb_diffuse;
    int pb_f0;
    int pb_f1;
    QOpenGLShaderProgram pb_shader;
};

const char *Profiler::counterNames[] =
{
    "Vertices Transformed",
    "Draw Calls",
    "Vertices Drawn",
    "Indices Drawn",
    "Clip [Scissor]",
    "Clip [Stencil]",
    "Clip [Cached]",
    "Batch Count",
    "Slow Elements",
    "Total Elements",
};

const char *Profiler::timerNames[] =
{
    "GpuBeginFrame",
    "UpdateRenderOrder",
    "BuildDrawLists",
    "BuildBatches",
    "GpuEndFrame",
    "DrawBatches",
    "ClearBatches",
};

static Profiler pf;

class ProfileScope
{
public:
    ProfileScope(Profiler::TimerType t_) : t(t_)
    {
        pf.startTimer(t);
    }

    ~ProfileScope()
    {
        pf.stopTimer(t);
    }

private:
    Profiler::TimerType t;
};

#define PROFILE_SCOPE(x)        ProfileScope _ps(Profiler::TIMER_##x)
#define PROFILE_COUNTER(x,c)    pf.incrementCounter(Profiler::COUNTER_##x, c)
#define PROFILE_BEGINFRAME()    pf.beginFrame()
#define PROFILE_ENDFRAME()      pf.endFrame()

#else   // ENABLE_PROFILING

#define PROFILE_SCOPE(x)
#define PROFILE_COUNTER(x,c)
#define PROFILE_BEGINFRAME()
#define PROFILE_ENDFRAME()

#endif  // (!) ENABLE_PROFILING

struct GpuMemoryPool
{
private:
    enum {
        BYTES_PER_VERTEX_BUFFER = 48 * 1024,
        BYTES_PER_INDEX_BUFFER = 16 * 1024
    };

public:

    GpuMemoryPool() : lockedVB(0), lockedIB(0) {}

    ~GpuMemoryPool()
    {
        std::vector<Buffer>::iterator it = vertexBuffers.begin();
        std::vector<Buffer>::iterator end = vertexBuffers.end();

        while (it != end) {
            free(it->mapped);
            ++it;
        }

        it = indexBuffers.begin();
        end = indexBuffers.end();

        while (it != end) {
            free(it->mapped);
            ++it;
        }
    }

    void lock(size_t vertexByteCount, size_t indexByteCount, unsigned char **vb, unsigned char **ib)
    {
        Q_ASSERT(lockedVB == 0 && lockedIB == 0);

        size_t vbCount = vertexBuffers.size();
        for (size_t i=0 ; i < vbCount ; ++i) {
            const Buffer &b = vertexBuffers[i];
            if (b.allocatedBytes + vertexByteCount <= b.bufferSizeInBytes) {
                lockedVB = &vertexBuffers[i];
                break;
            }
        }

        size_t ibCount = indexBuffers.size();
        for (size_t i=0 ; i < ibCount ; ++i) {
            const Buffer &b = indexBuffers[i];
            if (b.allocatedBytes + indexByteCount <= b.bufferSizeInBytes) {
                lockedIB = &indexBuffers[i];
                break;
            }
        }

        if (lockedVB == 0) {
            Buffer buffer;
            buffer.allocatedBytes = 0;
            buffer.bufferSizeInBytes = qMax((size_t)BYTES_PER_VERTEX_BUFFER, vertexByteCount);
            buffer.mapped = (unsigned char *) malloc(buffer.bufferSizeInBytes);
            vertexBuffers.push_back(buffer);
            lockedVB = &vertexBuffers.back();
        }
        if (lockedIB == 0) {
            Buffer buffer;
            buffer.allocatedBytes = 0;
            buffer.bufferSizeInBytes = qMax((size_t)BYTES_PER_INDEX_BUFFER, indexByteCount);
            buffer.mapped = (unsigned char *) malloc(buffer.bufferSizeInBytes);
            indexBuffers.push_back(buffer);
            lockedIB = &indexBuffers.back();
        }

        Q_ASSERT(lockedVB);
        Q_ASSERT(lockedIB);

        *vb = lockedVB->mapped + lockedVB->allocatedBytes;
        *ib = lockedIB->mapped + lockedIB->allocatedBytes;

        lockedVB->allocatedBytes += vertexByteCount;
        lockedIB->allocatedBytes += indexByteCount;
    }

    void unlock()
    {
        Q_ASSERT(lockedVB != 0 && lockedIB != 0);

        lockedVB = 0;
        lockedIB = 0;
    }

    void beginFrame()
    {
        std::vector<Buffer>::iterator it = vertexBuffers.begin();
        std::vector<Buffer>::iterator end = vertexBuffers.end();

        while (it != end) {
            it->allocatedBytes = 0;
            ++it;
        }

        it = indexBuffers.begin();
        end = indexBuffers.end();

        while (it != end) {
            it->allocatedBytes = 0;
            ++it;
        }
    }

    void endFrame()
    {
    }

private:

    struct Buffer
    {
        unsigned char *mapped;
        size_t bufferSizeInBytes;
        size_t allocatedBytes;
    };

    std::vector<Buffer> vertexBuffers;
    std::vector<Buffer> indexBuffers;

    Buffer *lockedVB;
    Buffer *lockedIB;
};

static GpuMemoryPool gpuMemory;
static const QMatrix4x4 dummyIdentity;

void RenderBatch::add(RenderElement *e)
{
    PROFILE_COUNTER(TotalElements, 1);
    PROFILE_COUNTER(SlowElements, e->usesFullMatrix ? 1 : 0);

    elements.push_back(e);
    indexCount += e->indexCount;
    vertexCount += e->vertexCount;
}

void RenderBatch::build()
{
    if (vertexCount) {
        gpuMemory.lock(vertexCount * cfg->vf->stride, indexCount * sizeof(short), &vb, &ib);

        int currentVertexOffset = 0;
        int currentIndexOffset = 0;

        for (size_t i=0 ; i < elements.size() ; ++i) {
            RenderElement *e = elements[i];

            memcpy(vb + currentVertexOffset * cfg->vf->stride, e->vertices, e->vertexCount * cfg->vf->stride);

            short *indexIt = (short *) (ib + currentIndexOffset * sizeof(short));
            for (int j=0 ; j < e->indexCount ; ++j)
                *indexIt++ = e->indices[j] + currentVertexOffset;

            currentVertexOffset += e->vertexCount;
            currentIndexOffset += e->indexCount;
        }

        gpuMemory.unlock();
    }
}

Renderer::Renderer(QSGContext *context)
    : QSGRenderer(context)
{
}

Renderer::~Renderer()
{
}

void Renderer::addNode(QSGNode *node)
{
    QSGNode::NodeType nodeType = node->type();

    if (nodeType == QSGNode::GeometryNodeType || nodeType == QSGNode::RenderNodeType) {
        RenderElement *e = new RenderElement;
        e->indexCount = e->vertexCount = 0;
        e->indices = 0;
        e->vertices = 0;
        e->currentVertexBufferSize = 0;
        e->currentIndexBufferSize = 0;
        e->transformDirty = true;
        e->geometryDirty = true;
        e->addedToBatch = false;
        e->usesFullMatrix = false;
        e->usesDeterminant = false;
        e->basenode = node;
        e->complexTransform = false;
        e->primType = RenderElement::PT_Invalid;
        e->vf = 0;

        if (nodeType == QSGNode::GeometryNodeType) {
            const QSGGeometry *g = static_cast<QSGGeometryNode *>(node)->geometry();
            e->vf = getVertexFormat(g->attributes(), g->attributeCount(), g->sizeOfVertex());
        }

        Q_ASSERT(m_elementHash.value(node, 0) == 0);
        m_elementHash.insert(node, e);
    }

    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        addNode(c);
}

void Renderer::removeNode(QSGNode *node)
{
    if (node->type() == QSGNode::GeometryNodeType || node->type() == QSGNode::RenderNodeType) {
        RenderElement *e = m_elementHash.value(node, 0);
        Q_ASSERT(e);

        std::vector<RenderElement *>::iterator it = m_elementsInRenderOrder.begin();
        std::vector<RenderElement *>::iterator end = m_elementsInRenderOrder.end();

        m_elementHash.remove(node);

        if (e->vertices)
            free(e->vertices);
        if (e->indices)
            free(e->indices);
        delete e;
    }

    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        removeNode(c);
}

void Renderer::dirtyChildNodes_Transform(QSGNode *node)
{
    if (node->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);
        RenderElement *e = m_elementHash.value(gn, 0);
        if (e)
            e->transformDirty = true;
    }

    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        dirtyChildNodes_Transform(c);
}

void Renderer::nodeChanged(QSGNode *node, QSGNode::DirtyState flags)
{
    QSGRenderer::nodeChanged(node, flags);

    if (flags & QSGNode::DirtyNodeAdded)
        addNode(node);
    else if (flags & QSGNode::DirtyNodeRemoved)
        removeNode(node);

    if (flags & (QSGNode::DirtyGeometry | QSGNode::DirtyForceUpdate)) {
        if (node->type() == QSGNode::GeometryNodeType) {
            QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);
            RenderElement *e = m_elementHash.value(gn, 0);
            Q_ASSERT(e);
            e->geometryDirty = true;
        }
    }

    if (flags & (QSGNode::DirtyMatrix | QSGNode::DirtyMaterial | QSGNode::DirtyOpacity | QSGNode::DirtyForceUpdate)) {
        dirtyChildNodes_Transform(node);
    }
}

void Renderer::render()
{
    PROFILE_BEGINFRAME();

    if (!clipProgram.isLinked()) {
        clipProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                            "attribute highp vec4 vCoord;       \n"
                                            "uniform highp mat4 matrix;         \n"
                                            "void main() {                      \n"
                                            "    gl_Position = matrix * vCoord; \n"
                                            "}");
        clipProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                            "void main() {                                   \n"
                                            "    gl_FragColor = vec4(0.81, 0.83, 0.12, 1.0); \n" // Trolltech green ftw!
                                            "}");
        clipProgram.bindAttributeLocation("vCoord", 0);
        clipProgram.link();
        clipMatrixID = clipProgram.uniformLocation("matrix");
    }

    batchConfigs.clear();
    minBatchConfig = 0;

    {
        PROFILE_SCOPE(GpuBeginFrame);
        gpuMemory.beginFrame();
    }

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glFrontFace(isMirrored() ? GL_CW : GL_CCW);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glDepthRangef(0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glClearColor(m_clear_color.redF(), m_clear_color.greenF(), m_clear_color.blueF(), m_clear_color.alphaF());
    glClearDepthf(1.0f);

    bindable()->clear(clearMode() & (ClearColorBuffer | ClearStencilBuffer | ClearDepthBuffer));

    QRect r = viewportRect();
    glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
    m_current_projection_matrix = projectionMatrix();
    m_current_model_view_matrix.setToIdentity();

    {
        PROFILE_SCOPE(UpdateRenderOrder);
        m_elementsInRenderOrder.clear();
        buildRenderOrderList(rootNode());
    }

    {
        PROFILE_SCOPE(BuildDrawLists);
        buildDrawLists();
    }

    {
        PROFILE_SCOPE(BuildBatches);
        buildBatches();
    }

    {
        PROFILE_SCOPE(GpuEndFrame);
        gpuMemory.endFrame();
    }

    {
        PROFILE_SCOPE(DrawBatches);
        drawBatches();
    }

    {
        PROFILE_SCOPE(ClearBatches);
        clearBatches();
    }

    PROFILE_ENDFRAME();
}

VertexFormat *Renderer::getVertexFormat(const QSGGeometry::Attribute *attributes, int attrCount, int stride)
{
    VertexFormat *v = 0;

    for (size_t i=0 ; i < m_vertexFormats.size() ; ++i) {
        v = m_vertexFormats[i];
        if (v->matches(attributes, attrCount, stride)) {
            ++v->refCount;
            return v;
        }
    }

    v = new VertexFormat(attributes, attrCount);
    m_vertexFormats.push_back(v);

    return v;
}

void Renderer::releaseVertexFormat(VertexFormat *vf)
{
    Q_ASSERT(vf->refCount > 0);
    if (--vf->refCount == 0) {
        std::vector<VertexFormat *>::iterator it = m_vertexFormats.begin();
        std::vector<VertexFormat *>::iterator end = m_vertexFormats.end();

        while (it != end) {
            if (*it == vf) {
                m_vertexFormats.erase(it);
                return;
            }
        }
    }

    Q_ASSERT(false);        // should be unreachable
}

void Renderer::addElementToBatchConfig(RenderElement *e)
{
    if (e->basenode->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *node = static_cast<QSGGeometryNode *>(e->basenode);
        qreal opacity = node->inheritedOpacity();
        QSGMaterial *mat = node->activeMaterial();
        QSGMaterialType *type = mat->type();
        const QSGClipNode *clip = node->clipList();
        RenderElement::PrimitiveType pt = e->primType;

        size_t batchConfigCount = batchConfigs.size();

        for (size_t i=minBatchConfig ; i < batchConfigCount ; ++i) {
            BatchConfig &cfg = batchConfigs[i];
            if (cfg.renderNode == 0 && cfg.useMatrix == e->usesFullMatrix && cfg.primType == pt && cfg.opacity == opacity &&
                cfg.vf == e->vf && cfg.type == type && cfg.clip == clip && cfg.material->compare(mat) == 0) {

                bool listMatch = true;
                if (e->usesDeterminant || e->usesFullMatrix) {
                    const QMatrix4x4 *m = node->matrix();
                    m = m ? m : &dummyIdentity;

                    listMatch = ((e->usesFullMatrix == false) || (cfg.transform == *m)) && ((e->usesDeterminant == false) || (cfg.determinant == m->determinant()));
                }
                if (listMatch) {
                    cfg.prevElement->nextInBatchConfig = m_elementsInRenderOrder.size();
                    cfg.prevElement = e;
                    e->batchConfig = i;
                    break;
                }
            }
        }

        if (e->batchConfig == -1) {
            BatchConfig cfg;
            cfg.opacity = opacity;
            cfg.vf = e->vf;
            cfg.material = mat;
            cfg.type = type;
            cfg.clip = clip;
            cfg.prevElement = e;
            cfg.primType = pt;

            const QMatrix4x4 *m = node->matrix();
            m = m ? m : &dummyIdentity;

            cfg.transform = *m;
            cfg.determinant = m->determinant();
            cfg.useMatrix = e->usesFullMatrix;

            e->batchConfig = batchConfigs.size();
            batchConfigs.push_back(cfg);
        }
    } else {
        // RenderNodes always get their own batch config for simplicity.
        BatchConfig cfg;
        Q_ASSERT(e->basenode->type() == QSGNode::RenderNodeType);
        cfg.renderNode = static_cast<QSGRenderNode *>(e->basenode);
        cfg.clip = cfg.renderNode->clipList();
        e->batchConfig = batchConfigs.size();
        minBatchConfig = e->batchConfig;
        batchConfigs.push_back(cfg);
    }
}

void Renderer::buildRenderOrderList(QSGNode *node)
{
    if (node->isSubtreeBlocked()) {
        return;
    }

    QSGNode::NodeType nodeType = node->type();

    if (nodeType == QSGNode::GeometryNodeType) {
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);
        RenderElement *e = m_elementHash.value(gn, 0);

        const QSGMaterial *material = gn->activeMaterial();
        const QMatrix4x4 *__restrict matrix = gn->matrix();

        bool needsMatrixForTransform = false;
        bool needsMatrixForMaterial = (material->flags() & QSGMaterial::RequiresFullMatrix) == QSGMaterial::RequiresFullMatrix;
        bool needsMatrixForAttributes = e->vf->positionAttribute == -1;

        if (matrix == 0) {
            matrix = &dummyIdentity;
        } else {
            const float *__restrict md = matrix->constData();
            needsMatrixForTransform = (md[3] != 0 || md[7] != 0 || md[15] != 1);
        }

        bool needsDeterminant = (material->flags() & QSGMaterial::RequiresDeterminant) == QSGMaterial::RequiresDeterminant;
        bool needsMatrix = needsMatrixForTransform || needsMatrixForMaterial || needsMatrixForAttributes;

        if (e->usesFullMatrix != needsMatrix)
            e->geometryDirty = true;

        e->complexTransform = needsMatrixForTransform;
        e->usesDeterminant = needsDeterminant;
        e->usesFullMatrix = needsMatrix;

        if (e->geometryDirty)
            updateElementGeometry(e);
        if (e->geometryDirty || e->transformDirty)
            updateElementTransform(e, matrix);

        e->transformDirty = false;
        e->geometryDirty = false;
        e->addedToBatch = false;

        e->batchConfig = -1;
        e->nextInBatchConfig = -1;
        addElementToBatchConfig(e);

        m_elementsInRenderOrder.push_back(e);
        Q_ASSERT(e->batchConfig != -1);
    } else if (nodeType == QSGNode::RenderNodeType) {
        RenderElement *e = m_elementHash.value(node, 0);
        e->batchConfig = -1;
        e->nextInBatchConfig = -1;
        e->addedToBatch = false;
        addElementToBatchConfig(e);
        m_elementsInRenderOrder.push_back(e);
        Q_ASSERT(e->batchConfig != -1);
    }

    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        buildRenderOrderList(c);
}

void Renderer::updateElementGeometry(RenderElement *__restrict e)
{
    const QSGGeometry *g = static_cast<QSGGeometryNode *>(e->basenode)->geometry();
    Q_ASSERT(g);
    GLenum drawingMode = g->drawingMode();

    e->vf = getVertexFormat(g->attributes(), g->attributeCount(), g->sizeOfVertex());

    if (drawingMode != GL_TRIANGLE_STRIP && drawingMode != GL_TRIANGLES &&
        drawingMode != GL_LINES && drawingMode != GL_LINE_STRIP &&
        drawingMode != GL_POINTS) {
        qDebug() << "RENDERER: Only supports triangles, lines and points. Skipping element with primitive type " << drawingMode;
        return;
    }
    if (g->indexType() != GL_UNSIGNED_SHORT) {
        qDebug() << "RENDERER: Only support 16 bit indices currently. Skipping element with 32 bit indices.";
        return;
    }

    e->localRect.minPoint.set(9e9, 9e9);
    e->localRect.maxPoint.set(-9e9, -9e9);

    int srcVertexCount = g->vertexCount();
    const char *srcVertexData = reinterpret_cast<const char *>(g->vertexData());
    int srcIndexCount = g->indexCount();

    if (e->vf->positionAttribute != -1 && e->usesFullMatrix == false) {
        const VertexFormat::Attribute &posAttr = e->vf->attributes[e->vf->positionAttribute];

        if (posAttr.componentCount != 2) {
            qDebug() << "RENDERER: Only supports position attributes with 2 components currently. Skipping element with " << posAttr.componentCount << " components.";
            return;
        }

        // Calculate local rect.
#ifdef USE_HALF_FLOAT
        Q_ASSERT(posAttr.dataType == GL_HALF_FLOAT_OES);
#else
        Q_ASSERT(posAttr.dataType == GL_FLOAT);
        Q_ASSERT(g->sizeOfVertex() == e->vf->stride);
#endif

        for (int i=0 ; i < srcVertexCount ; ++i) {
            const float *pos = (const float *) &srcVertexData[i * g->sizeOfVertex() + posAttr.byteOffset];

            e->localRect.minPoint.x = qMin(e->localRect.minPoint.x, pos[0]);
            e->localRect.minPoint.y = qMin(e->localRect.minPoint.y, pos[1]);
            e->localRect.maxPoint.x = qMax(e->localRect.maxPoint.x, pos[0]);
            e->localRect.maxPoint.y = qMax(e->localRect.maxPoint.y, pos[1]);
        }
    }

    int destVertexCount = srcVertexCount;
    int destVertexBufferSize = destVertexCount * e->vf->stride;
    e->vertexCount = destVertexCount;

    int destIndexCount = 0;

    switch (drawingMode) {
        case GL_TRIANGLES:
            e->primType = RenderElement::PT_Triangles;
            destIndexCount = srcIndexCount ? srcIndexCount : srcVertexCount;
            break;
        case GL_TRIANGLE_STRIP:
            e->primType = RenderElement::PT_Triangles;
            destIndexCount = srcIndexCount ? (srcIndexCount-2) * 3 : (srcVertexCount-2) * 3;
            break;
        case GL_LINES:
            e->primType = RenderElement::PT_Lines;
            destIndexCount = srcIndexCount ? srcIndexCount : srcVertexCount;
            break;
        case GL_LINE_STRIP:
            e->primType = RenderElement::PT_Lines;
            destIndexCount = srcIndexCount ? (srcIndexCount-1) * 2 : (srcVertexCount-1) * 2;
            break;
        case GL_POINTS:
            e->primType = RenderElement::PT_Points;
            destIndexCount = 0;
            break;
        default:
            Q_ASSERT(false);
            break;
    }

    int destIndexBufferSize = destIndexCount * sizeof(short);
    e->indexCount = destIndexCount;

    if (e->currentVertexBufferSize != destVertexBufferSize) {
        if (e->vertices) {
            free(e->vertices);
        }
        e->currentVertexBufferSize = destVertexBufferSize;
        e->vertices = (char *) malloc(e->currentVertexBufferSize);
    }
    if (e->currentIndexBufferSize != destIndexBufferSize) {
        if (e->indices) {
            free(e->indices);
        }
        e->currentIndexBufferSize = destIndexBufferSize;
        e->indices = (short *) malloc(e->currentIndexBufferSize);
    }

    // Copy vertex data
#ifdef USE_HALF_FLOAT
    char *srcVertex = (char *) srcVertexData;
    char *destVertex = (char *) e->vertices;

    for (int i=0 ; i < srcVertexCount ; ++i) {
        int srcVertexOffset = 0;

        for (int j=0 ; j < e->vf->attributeCount ; ++j) {
            const VertexFormat::Attribute &destAttr = e->vf->attributes[j];
            const QSGGeometry::Attribute &srcAttr = g->attributes()[j];

            int srcAttributeSize = srcAttr.tupleSize * VertexFormat::componentSize(srcAttr.type);
            if (destAttr.dataType == GL_HALF_FLOAT_OES) {
                // convert to float16
                float *__restrict s = (float *) &srcVertex[srcVertexOffset];
                __fp16 *__restrict t = (__fp16 *) &destVertex[destAttr.byteOffset];
                for (int k=0 ; k < destAttr.componentCount ; ++k) {
                    t[k] = s[k];
                }
            } else {
                memcpy(&destVertex[destAttr.byteOffset], &srcVertex[srcVertexOffset], srcAttributeSize);
            }
            srcVertexOffset += srcAttributeSize;
        }
        srcVertex += g->sizeOfVertex();
        destVertex += e->vf->stride;
    }
#else
    memcpy(e->vertices, srcVertexData, destVertexBufferSize);
#endif

    // Copy index data
    short *indexIt = e->indices;

    if (srcIndexCount) {
        const short *srcIndexData = reinterpret_cast<const short *>(g->indexData());

        switch (drawingMode) {
            case GL_TRIANGLES:
            case GL_LINES:
                memcpy(e->indices, srcIndexData, destIndexBufferSize);
                break;
            case GL_LINE_STRIP:
                for (int i=1 ; i < srcIndexCount ; ++i) {
                    int i0 = i-1;
                    int i1 = i-0;
                    *indexIt++ = srcIndexData[i0];
                    *indexIt++ = srcIndexData[i1];
                }
                break;
            case GL_TRIANGLE_STRIP:
                // TODO: It's highly likely there are degen triangles in this case. Find them and remove.
                for (int i=2 ; i < srcIndexCount ; ++i) {

                    int i0 = (i&1) ? (i-0) : (i-2);
                    int i1 = i-1;
                    int i2 = (i&1) ? (i-2) : (i-0);

                    *indexIt++ = srcIndexData[i0];
                    *indexIt++ = srcIndexData[i1];
                    *indexIt++ = srcIndexData[i2];
                }
                Q_ASSERT(indexIt == e->indices + destIndexCount);
                break;
            case GL_POINTS:
                Q_ASSERT(false);
                break;
        }

    } else {
        switch (drawingMode) {
            case GL_TRIANGLES:
                for (int i=0 ; i < srcVertexCount ; ++i)
                    *indexIt++ = i;
                break;
            case GL_LINES:
                for (int i=0 ; i < srcVertexCount ; ++i)
                    *indexIt++ = i;
                break;
            case GL_TRIANGLE_STRIP:
                for (int i=2 ; i < srcVertexCount ; ++i) {
                    *indexIt++ = i-2;
                    *indexIt++ = i-1;
                    *indexIt++ = i;
                }
                break;
            case GL_LINE_STRIP:
                for (int i=1 ; i < srcVertexCount ; ++i) {
                    *indexIt++ = i-1;
                    *indexIt++ = i-0;
                }
                break;
        }
        Q_ASSERT(indexIt == e->indices + destIndexCount);
    }
}

void Renderer::updateElementTransform(RenderElement *__restrict e, const QMatrix4x4 *mat)
{
    if (e->vertexCount && e->usesFullMatrix == false) {
        const QSGGeometry *__restrict g = static_cast<QSGGeometryNode *>(e->basenode)->geometry();
        int srcVertexCount = g->vertexCount();
        const char *__restrict srcVertexData = reinterpret_cast<const char *>(g->vertexData());
        Q_ASSERT(srcVertexCount == e->vertexCount);

        int srcStride = g->sizeOfVertex();
        int destStride = e->vf->stride;

#ifdef DESKTOP_BUILD
        if (true) {
#else
        const qreal *__restrict md = mat->constData();
        float m[8] = { md[0], md[1], md[4], md[5], md[12], md[13], md[12], md[13] };

        if (e->complexTransform) {
#endif
            QPointF localMin(e->localRect.minPoint.x, e->localRect.minPoint.y);
            QPointF localMax(e->localRect.maxPoint.x, e->localRect.maxPoint.y);
            QPointF worldP0 = (*mat) * localMin;
            QPointF worldP1 = (*mat) * localMax;

            if (worldP0.x() < worldP1.x()) {
                e->worldRect.minPoint.x = worldP0.x();
                e->worldRect.maxPoint.x = worldP1.x();
            } else {
                e->worldRect.minPoint.x = worldP1.x();
                e->worldRect.maxPoint.x = worldP0.x();
            }
            if (worldP0.y() < worldP1.y()) {
                e->worldRect.minPoint.y = worldP0.y();
                e->worldRect.maxPoint.y = worldP1.y();
            } else {
                e->worldRect.minPoint.y = worldP1.y();
                e->worldRect.maxPoint.y = worldP0.y();
            }

#ifndef DESKTOP_BUILD
        } else {

            // Transform localRect -> worldRect
            asm volatile (
                "vld1.32    {d0, d1, d2, d3}, [%0]  \n\t"
                "vld1.32    d4, [%1]                \n\t"
                "vld1.32    d5, [%2]                \n\t"

                "vmla.f32   d2, d0, d4[0]           \n\t"
                "vmla.f32   d2, d1, d4[1]           \n\t"

                "vmla.f32   d3, d0, d5[0]           \n\t"
                "vmla.f32   d3, d1, d5[1]           \n\t"

                "vmin.f32   d0, d2, d3              \n\t"
                "vmax.f32   d1, d2, d3              \n\t"

                "vst1.32    d0, [%3]                \n\t"
                "vst1.32    d1, [%4]                \n\t"

                :: "r"(m), "r"(&e->localRect.minPoint), "r"(&e->localRect.maxPoint), "r"(&e->worldRect.minPoint), "r"(&e->worldRect.maxPoint)
                : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
            );
#endif
        }

        // Transform the vertices themselves
        Q_ASSERT(e->vf->positionAttribute != -1);
        const VertexFormat::Attribute &posAttr = e->vf->attributes[e->vf->positionAttribute];
        const char *__restrict src = srcVertexData + posAttr.byteOffset;
        char *__restrict dest = e->vertices + posAttr.byteOffset;

        for (int i=0 ; i < srcVertexCount ; ++i) {
#ifdef DESKTOP_BUILD
            float *sv = (float *) src;
            float *dv = (float *) dest;
            QPointF v(sv[0], sv[1]);
            QPointF t = (*mat) * v;
            dv[0] = t.x();
            dv[1] = t.y();
#else
#ifdef USE_HALF_FLOAT
            asm volatile (
                "vld1.32        {d0, d1, d2}, [%0]              \n\t"
                "vld1.32        d3, [%1]                        \n\t"

                "vmla.f32       d2, d0, d3[0]                   \n\t"
                "vmla.f32       d2, d1, d3[1]                   \n\t"

                "vcvt.f16.f32   d2, q1                          \n\t"

                "vst1.32        d2[0], [%2]                     \n\t"
                :: "r"(m), "r"(src), "r"(dest)
                : "d0", "d1", "d2", "d3", "q1", "memory"
            );
#else
            asm volatile (
                "vld1.32        {d0, d1, d2}, [%0]              \n\t"
                "vld1.32        d3, [%1]                        \n\t"

                "vmla.f32       d2, d0, d3[0]                   \n\t"
                "vmla.f32       d2, d1, d3[1]                   \n\t"

                "vst1.32        d2, [%2]                        \n\t"
                :: "r"(m), "r"(src), "r"(dest)
                : "d0", "d1", "d2", "d3", "memory"
            );
#endif
#endif

            src += srcStride;
            dest += destStride;
        }

        PROFILE_COUNTER(VerticesTransformed, srcVertexCount);
    } else {
        e->localRect.minPoint.x = e->worldRect.minPoint.x = -9e9f;
        e->localRect.minPoint.y = e->worldRect.minPoint.y = -9e9f;
        e->localRect.maxPoint.x = e->worldRect.maxPoint.x =  9e9f;
        e->localRect.maxPoint.y = e->worldRect.maxPoint.y =  9e9f;
    }
}

class BoundingArea
{
public:
    BoundingArea()
        : m_count(0)
    {
    }

    static float area(const Rect &rect)
    {
        return (rect.maxPoint.x - rect.minPoint.x) * (rect.maxPoint.y - rect.minPoint.y);
    }

    void unite(const Rect &rect)
    {
        if (m_count == 0) {
            m_count = 1;
            m_ra = rect;
            return;
        }

        if (m_count == 1) {
            m_count = 2;
            m_rb = rect;
            return;
        }

        Rect ca = m_ra;
        ca.unite(m_rb);

        Rect cb = m_rb;
        cb.unite(rect);

        Rect ia = ca;
        ia.intersect(rect);

        Rect ib = cb;
        ib.intersect(m_ra);

        if (area(ca) + area(rect) - area(ia) > area(m_ra) + area(cb) - area(ib)) {
            m_rb = cb;
        } else {
            m_ra = ca;
            m_rb = rect;
        }
    }

    bool intersects(const Rect &rect)
    {
        return (m_count > 0 && m_ra.intersects(rect)) || (m_count > 1 && m_rb.intersects(rect));
    }

private:
    Rect m_ra;
    Rect m_rb;
    int m_count;
};

bool Renderer::isOverlap(int i, int j, const Rect &r)
{
    for (int c=i ; c < j ; ++c) {
        RenderElement *e = m_elementsInRenderOrder[c];
        if (e->addedToBatch)
            continue;

        if (e->worldRect.intersects(r))
            return true;
    }

    return false;
}

void Renderer::buildDrawLists()
{
    size_t elementCount = m_elementsInRenderOrder.size();

    for (size_t i=0 ; i < elementCount ; ++i) {
        RenderElement *ei = m_elementsInRenderOrder[i];

        Q_ASSERT(ei->batchConfig != -1);
        const BatchConfig &cfg = batchConfigs[ei->batchConfig];

        if ((ei->addedToBatch) || (ei->vertexCount == 0 && cfg.renderNode == 0))
            continue;

        RenderBatch tmpBatch(&cfg);
        m_batches.push_back(tmpBatch);
        RenderBatch &rb = m_batches.back();

        rb.indexCount = 0;
        rb.vertexCount = 0;

        rb.add(ei);
        PROFILE_COUNTER(BatchCount, 1);
        ei->addedToBatch = true;

        BoundingArea united;

        int current = i + 1;

        short nextIndex = ei->nextInBatchConfig;
        while (nextIndex != -1) {
            for (int j = current; j < nextIndex; ++j) {
                RenderElement *ej = m_elementsInRenderOrder[j];
                if (!ej->addedToBatch)
                    united.unite(ej->worldRect);
            }

            current = nextIndex + 1;
            RenderElement *ej = m_elementsInRenderOrder[nextIndex];

            Q_ASSERT(ej->batchConfig != -1 && batchConfigs[ej->batchConfig].renderNode == 0);
            if (ej->addedToBatch == false && ej->vertexCount && (!united.intersects(ej->worldRect) || !isOverlap(i + 1, nextIndex, ej->worldRect))) {
                rb.add(ej);
                ej->addedToBatch = true;
            }
            nextIndex = ej->nextInBatchConfig;
        }
    }
}

void Renderer::buildBatches()
{
    for (size_t i=0 ; i < m_batches.size() ; ++i) {
        m_batches[i].build();
    }
}

void Renderer::drawBatches()
{
    // All vertices are pre-transformed on CPU
    const QSGClipNode *currentClip = 0;
    QSGMaterialShader *currentShader = 0;
    ClipType currentClipType = NoClip;

    for (size_t i=0 ; i < m_batches.size() ; ++i) {
        const RenderBatch &rb = m_batches[i];
        const BatchConfig *bc = rb.cfg;

        if (bc->useMatrix)
            m_current_model_view_matrix = bc->transform;
        else
            m_current_model_view_matrix.setToIdentity();
        m_current_opacity = bc->opacity;

        if (bc->clip) {
            if (bc->clip != currentClip) {
                if (currentShader)
                    currentShader->deactivate();
                currentShader = 0;
                glDisable(GL_STENCIL_TEST);
                glDisable(GL_SCISSOR_TEST);
                currentClipType = 0;

                const QRect &dr = deviceRect();

                const QSGClipNode *clip = bc->clip;
                m_current_stencil_value = 0;

                while (clip) {
                    QMatrix4x4 m = m_current_projection_matrix;
                    if (clip->matrix())
                        m *= *clip->matrix();

                    // TODO: Check for multisampling and pixel grid alignment.
                    bool isRectangleWithNoPerspective = clip->isRectangular()
                            && qFuzzyIsNull(m(3, 0)) && qFuzzyIsNull(m(3, 1));
                    bool noRotate = qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(1, 0));
                    bool isRotate90 = qFuzzyIsNull(m(0, 0)) && qFuzzyIsNull(m(1, 1));

                    if (isRectangleWithNoPerspective && (noRotate || isRotate90)) {
                        QRectF bbox = clip->clipRect();
                        qreal invW = 1 / m(3, 3);
                        qreal fx1, fy1, fx2, fy2;
                        if (noRotate) {
                            fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
                            fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
                            fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
                            fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;
                        } else {
                            Q_ASSERT(isRotate90);
                            fx1 = (bbox.bottom() * m(0, 1) + m(0, 3)) * invW;
                            fy1 = (bbox.left() * m(1, 0) + m(1, 3)) * invW;
                            fx2 = (bbox.top() * m(0, 1) + m(0, 3)) * invW;
                            fy2 = (bbox.right() * m(1, 0) + m(1, 3)) * invW;
                        }

                        if (fx1 > fx2)
                            qSwap(fx1, fx2);
                        if (fy1 > fy2)
                            qSwap(fy1, fy2);


                        GLint ix1 = qRound((fx1 + 1) * dr.width() * qreal(0.5));
                        GLint iy1 = qRound((fy1 + 1) * dr.height() * qreal(0.5));
                        GLint ix2 = qRound((fx2 + 1) * dr.width() * qreal(0.5));
                        GLint iy2 = qRound((fy2 + 1) * dr.height() * qreal(0.5));

                        if ((currentClipType & ScissorClip) == 0) {
                            m_current_scissor_rect = QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
                            glEnable(GL_SCISSOR_TEST);
                            currentClipType |= ScissorClip;
                        } else {
                            m_current_scissor_rect &= QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
                        }

                        glScissor(m_current_scissor_rect.x(), m_current_scissor_rect.y(), m_current_scissor_rect.width(), m_current_scissor_rect.height());
                        PROFILE_COUNTER(ClipScissor, 1);
                    } else {
                        if ((currentClipType & StencilClip) == 0) {
                            glStencilMask(0xff); // write mask
                            glClearStencil(0);
                            glClear(GL_STENCIL_BUFFER_BIT);
                            glEnable(GL_STENCIL_TEST);
                            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                            glDepthMask(GL_FALSE);

                            clipProgram.bind();
                            clipProgram.enableAttributeArray(0);

                            currentClipType |= StencilClip;
                        }

                        glStencilFunc(GL_EQUAL, m_current_stencil_value, 0xff); // stencil test, ref, test mask
                        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // stencil fail, z fail, z pass

                        const QSGGeometry *g = clip->geometry();
                        Q_ASSERT(g->attributeCount() > 0);
                        const QSGGeometry::Attribute *a = g->attributes();
                        glVertexAttribPointer(0, a->tupleSize, a->type, GL_FALSE, g->sizeOfVertex(), g->vertexData());

                        clipProgram.setUniformValue(clipMatrixID, m);
                        if (g->indexCount())
                            glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), g->indexData());
                        else
                            glDrawArrays(g->drawingMode(), 0, g->vertexCount());
                        PROFILE_COUNTER(DrawCalls, 1);
                        PROFILE_COUNTER(ClipStencil, 1);

                        ++m_current_stencil_value;
                    }

                    clip = clip->clipList();

                    if (currentClipType & StencilClip) {
                        clipProgram.disableAttributeArray(0);
                        glStencilFunc(GL_EQUAL, m_current_stencil_value, 0xff); // stencil test, ref, test mask
                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // stencil fail, z fail, z pass
                        glStencilMask(0); // write mask
                        bindable()->reactivate();
                    }
                }
            } else {
                PROFILE_COUNTER(ClipCached, 1);
            }
        } else if (currentClip) {
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_SCISSOR_TEST);
            currentClipType = 0;
        }
        currentClip = bc->clip;

        if (bc->renderNode) {
            if (currentShader)
                currentShader->deactivate();
            currentShader = 0;
            glDisable(GL_DEPTH_TEST);
            glDepthMask(false);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            QSGRenderNode::RenderState state;
            state.projectionMatrix = &m_current_projection_matrix;
            state.scissorEnabled = currentClipType & ScissorClip;
            state.stencilEnabled = currentClipType & StencilClip;
            state.scissorRect = m_current_scissor_rect;
            state.stencilValue = m_current_stencil_value;

            bc->renderNode->render(state);

            QSGRenderNode::StateFlags changes = bc->renderNode->changedStates();
            if (changes & QSGRenderNode::ViewportState) {
                QRect r = viewportRect();
                glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
            }
            if (changes & QSGRenderNode::StencilState) {
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                glStencilMask(0xff);
                glDisable(GL_STENCIL_TEST);
            }
            if (changes & (QSGRenderNode::StencilState | QSGRenderNode::ScissorState)) {
                glDisable(GL_SCISSOR_TEST);
                currentClip = 0;
                currentClipType = NoClip;
            }
            if (changes & QSGRenderNode::DepthState) {
#if defined(QT_OPENGL_ES)
                glClearDepthf(0);
#else
                glClearDepth(0);
#endif
                if (m_clear_mode & QSGRenderer::ClearDepthBuffer) {
                    glDepthMask(true);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
                glDepthMask(false);
                glDepthFunc(GL_GREATER);
            }
            if (changes & QSGRenderNode::ColorState)
                bindable()->reactivate();
            if (changes & QSGRenderNode::BlendState) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (changes & QSGRenderNode::CullState) {
                glFrontFace(isMirrored() ? GL_CW : GL_CCW);
                glDisable(GL_CULL_FACE);
            }
        } else {
            QSGMaterialShader::RenderState::DirtyStates updates;
            updates |= QSGMaterialShader::RenderState::DirtyMatrix;
            updates |= QSGMaterialShader::RenderState::DirtyOpacity;
            m_current_determinant = bc->determinant;

            QSGMaterialShader *shader = m_context->prepareMaterial(bc->material);
            if (shader != currentShader) {
                Q_ASSERT(shader->program()->isLinked());
                if (currentShader)
                    currentShader->deactivate();
                shader->activate();
                currentShader = shader;
            }
            shader->updateState(state(updates), bc->material, 0);

            char const *const *attrNames = shader->attributeNames();
            for (int j = 0; attrNames[j]; ++j) {
                if (!*attrNames[j])
                    continue;
                Q_ASSERT_X(j < bc->vf->attributeCount, "QSGRenderer::bindGeometry()", "Geometry lacks attribute required by material");
                const VertexFormat::Attribute &a = bc->vf->attributes[j];
                Q_ASSERT_X(j == a.inputRegister, "QSGRenderer::bindGeometry()", "Geometry does not have continuous attribute positions");

#ifdef USE_HALF_FLOAT
                GLboolean normalize = a.dataType != GL_FLOAT && a.dataType != GL_HALF_FLOAT_OES;
#else
                GLboolean normalize = a.dataType != GL_FLOAT;
#endif

                glVertexAttribPointer(a.inputRegister, a.componentCount, a.dataType, normalize, bc->vf->stride, rb.vb + a.byteOffset);
            }

            if (bc->primType == RenderElement::PT_Points)
                glDrawArrays(GL_POINTS, 0, rb.vertexCount);
            else
                glDrawElements(bc->primType, rb.indexCount, GL_UNSIGNED_SHORT, rb.ib);

            PROFILE_COUNTER(DrawCalls, 1);
            PROFILE_COUNTER(VerticesDrawn, rb.vertexCount);
            PROFILE_COUNTER(IndicesDrawn, rb.indexCount);
        }
    }

    if (currentShader)
        currentShader->deactivate();

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
}

void Renderer::clearBatches()
{
    m_batches.clear();
}

}
