TEMPLATE=lib
TARGET=customcontext

CONFIG += plugin

contains(QT_VERSION, ^5\\.[2-9]\\..*) {
    message("Using Qt 5.2 or later")
    CONFIG += customcontext_qt520
}

QT += gui-private core-private quick-private qml-private
!customcontext_qt520:{
    QT += v8-private
}

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

!customcontext_qt520:{
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
        message("mactexture ...............: no")
    }
}

eglgralloctexture:{
    message("eglgralloctexture ........: yes (remember: ANDROID_SDK_INCLUDE)")
    DEFINES += CUSTOMCONTEXT_EGLGRALLOCTEXTURE
    SOURCES += texture/eglgralloctexture.cpp
    HEADERS += texture/eglgralloctexture.h

    INCLUDEPATH += texture $(ANDROID_SDK_INCLUDE) $(ANDROID_SDK_INCLUDE)/android
    LIBS += -lhardware

} else {
    message("eglgralloctexure .........: no")
}

hybristexture :{
    message("hybristexture ............: yes")
    DEFINES += CUSTOMCONTEXT_HYBRISTEXTURE
    SOURCES += texture/hybristexture.cpp
    HEADERS += texture/hybristexture.h
    INCLUDEPATH += texture

} else {
    message("hybristexture ............: no")
}

nonpreservedtexture:{
    message("nonpreservedtexture ......: yes")
    DEFINES += CUSTOMCONTEXT_NONPRESERVEDTEXTURE
    SOURCES += texture/nonpreservedtexture.cpp
    HEADERS += texture/nonpreservedtexture.h
    INCLUDEPATH += texture
} else {
    message("nonpreservedtexture ......: no")
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

swaplistenanimationdriver:{
    message("swaplistenanimationdriver : yes")
    DEFINES += CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
    SOURCES += animation/swaplisteninganimationdriver.cpp
    HEADERS += animation/swaplisteninganimationdriver.h
} else {
    message("swaplistenanimationdriver : no")
}



############################################################
#
# Renderers
#

overlaprenderer:{
    message("overlaprenderer ..........: yes")
    customcontext_qt520:{
        message("  *** WARNING: you probably want to be using the default renderer in Qt 5.2 and higher")
        message("  as it incorporates the optimizations of the overlaprenderer along with other optimizations ***")

        materialpreload:{
            message("  materialpreload ........: yes")
            DEFINES += CUSTOMCONTEXT_MATERIALPRELOAD
        } else {
            message("  materialpreload ........: no")
        }
    }
    DEFINES += CUSTOMCONTEXT_OVERLAPRENDERER
    SOURCES += renderer/overlaprenderer.cpp
    HEADERS += renderer/overlaprenderer.h
} else {
    message("overlaprenderer ..........: no")
}

simplerenderer:{
    message("simplerenderer ...........: yes")
    DEFINES += CUSTOMCONTEXT_SIMPLERENDERER
    SOURCES += renderer/simplerenderer.cpp
    HEADERS += renderer/simplerenderer.h renderer/qsgbasicshadermanager_p.h renderer/qsgbasicclipmanager_p.h
} else {
    message("simplerenderer ...........: no")
}


############################################################
#
# Other stuff
#

!customcontext_qt520:{
    materialpreload:{
        message("materialpreload ..........: yes")
        DEFINES += CUSTOMCONTEXT_MATERIALPRELOAD
    } else {
        message("materialpreload ..........: no")
    }

    nodfglyphs:{
        message("nodfglyphs ...............: yes")
        DEFINES += CUSTOMCONTEXT_NO_DFGLYPHS
    } else {
        message("nodfglyphs ...............: no")
    }

    msaa:{
        message("msaa .....................: yes")
        DEFINES += CUSTOMCONTEXT_MSAA
    } else {
        message("msaa .....................: no")
    }
} else {
    programbinary: {
        message("programbinary ............: yes")
        DEFINES += PROGRAM_BINARY
        SOURCES += programbinary.cpp
    } else {
        message("programbinary ............: no")
    }
}

surfaceformat:{
    message("surfaceformat ............: yes")
    DEFINES += CUSTOMCONTEXT_SURFACEFORMAT
} else {
    message("surfaceformat ............: no")
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

