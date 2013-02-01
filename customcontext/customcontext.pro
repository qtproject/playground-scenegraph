TEMPLATE=lib
TARGET=customcontext

CONFIG += plugin

QT += gui-private core-private quick-private qml-private v8-private

SOURCES += \
    animationdriver.cpp \
    atlastexture.cpp \
    context.cpp \
    overlaprenderer.cpp \
    pluginmain.cpp

HEADERS += \
    animationdriver.h \
    atlastexture.h \
    context.h \
    overlaprenderer.h \
    pluginmain.h

OTHER_FILES += customcontext.json

target.path +=  $$[QT_INSTALL_PLUGINS]/scenegraph

files.path += $$[QT_INSTALL_PLUGINS]/scenegraph

INSTALLS += target files

!win*:DEFINES+=ENABLE_PROFILING

arm_build {
    DEFINES += USE_HALF_FLOAT
    QMAKE_CXXFLAGS += -mfp16-format=ieee -mfpu=neon
} else {
    DEFINES += DESKTOP_BUILD
}

verbose:DEFINES+=CUSTOMCONTEXT_DEBUG
