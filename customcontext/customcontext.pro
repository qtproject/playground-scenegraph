TEMPLATE=lib
TARGET=customcontext

CONFIG += plugin

QT += gui-private core-private quick-private qml-private v8-private



verbose:{
    message("verbose: enabled")
    DEFINES+=CUSTOMCONTEXT_DEBUG
} else {
    message("verbose: disabled")
}


dither:{
    message("dither: enabled")
    DEFINES += CUSTOMCONTEXT_DITHER
    SOURCES += renderhooks/ordereddither2x2.cpp
    HEADERS += renderhooks/ordereddither2x2.h
} else {
    message("dither: disabled")
}



atlastexture:{
    message("atlastexture: enabled")
    DEFINES += CUSTOMCONTEXT_ATLASTEXTURE
    SOURCES += texture/atlastexture.cpp
    HEADERS += texture/atlastexture.h
} else {
    message("atlastexture: disabled")
}



animationdriver:{
    message("animationdriver: enabled")
    DEFINES += CUSTOMCONTEXT_ANIMATIONDRIVER
    SOURCES += animation/animationdriver.cpp
    HEADERS += animation/animationdriver.h
} else {
    message("animationdriver: disabled")
}



overlaprenderer:{
    message("overlaprenderer: enabled")
    DEFINES += CUSTOMCONTEXT_OVERLAPRENDERER
    SOURCES += renderer/overlaprenderer.cpp
    HEADERS += renderer/overlaprenderer.h
} else {
    message("overlaprenderer: disabled")
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

