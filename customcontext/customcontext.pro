TEMPLATE=lib
TARGET=customcontext

CONFIG += plugin

QT += gui-private core-private quick-private qml-private v8-private

message("")

verbose:{
    message("verbose ..................: yes")
    DEFINES+=CUSTOMCONTEXT_DEBUG
} else {
    message("verbose ..................: no")
}



############################################################
#
# Rendering hooks
#

dither:{
    message("dither ...................: yes")
    DEFINES += CUSTOMCONTEXT_DITHER
    SOURCES += renderhooks/ordereddither2x2.cpp
    HEADERS += renderhooks/ordereddither2x2.h
} else {
    message("dither ...................: no")
}



############################################################
#
# Textures
#

atlastexture:{
    message("atlastexture .............: yes")
    DEFINES += CUSTOMCONTEXT_ATLASTEXTURE
    SOURCES += texture/atlastexture.cpp
    HEADERS += texture/atlastexture.h
    INCLUDEPATH += texture
} else {
    message("atlastexture .............: no")
}

threaduploadtexture:{
    message("threaduploadtexture ......: yes")
    DEFINES += CUSTOMCONTEXT_THREADUPLOADTEXTURE
    SOURCES += texture/threaduploadtexture.cpp
    HEADERS += texture/threaduploadtexture.h
    INCLUDEPATH += texture
} else {
    message("threaduploadtexture ......: no")
}

mactexture:{
    message("mactexture ...............: yes")
    DEFINES += CUSTOMCONTEXT_MACTEXTURE
    SOURCES += texture/mactexture.cpp
    HEADERS += texture/mactexture.h
    INCLUDEPATH += texture
} else {
    message("mactexture ...............: no");
}



############################################################
#
# Animation Drivers
#

animationdriver:{
    message("animationdriver ..........: yes")
    DEFINES += CUSTOMCONTEXT_ANIMATIONDRIVER
    SOURCES += animation/animationdriver.cpp
    HEADERS += animation/animationdriver.h
} else {
    message("animationdriver ..........: no")
}



############################################################
#
# Renderers
#

overlaprenderer:{
    message("overlaprenderer ..........: yes")
    DEFINES += CUSTOMCONTEXT_OVERLAPRENDERER
    SOURCES += renderer/overlaprenderer.cpp
    HEADERS += renderer/overlaprenderer.h
} else {
    message("overlaprenderer ..........: no")
}



message("");
message("Enable the above features by adding them to the qmake config, for instance:")
message(" > qmake \"CONFIG+=verbose atlastexture dither\"");
message("");



SOURCES += \
    context.cpp \
    pluginmain.cpp

HEADERS += \
    context.h \
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

