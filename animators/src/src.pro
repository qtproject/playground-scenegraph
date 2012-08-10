TARGET = animators
TEMPLATE = lib

QT = core gui v8 qml quick
QT += quick-private qml-private v8-private gui-private core-private

CONFIG += plugin

DEFINES += ANIMATORS_DEBUG

HEADERS += \
    qsganimatorcontroller.h \
    qsganimatedtransform.h \
    qsganimatedtranslate.h \
    qsganimatedscale.h \
    qsganimatedrotation.h \
    qsganimatoritem.h \
    qsgsequentialanimator.h \
    qsgparallelanimator.h \
    qsgtoplevelanimator.h \
    qsgpauseanimator.h \
    qsganimatedproperty.h \
    qsgpropertyanimator.h \
    qsgabstractanimator.h \
    qsgsequentialanimation.h \
    qsgpropertyanimation.h \
    qsgpauseanimation.h \
    qsgparallelanimation.h \
    qsgabstractanimation.h \
    qsgnumberanimation.h \
    qsgcoloranimation.h \
    qsgvector3danimation.h \
    qsganimatorshadereffect.h \
    qsganimatornode.h

SOURCES += \
    qsganimatoritem.cpp \
    qsganimatorplugin.cpp \
    qsganimatorcontroller.cpp \
    qsganimatedtransform.cpp \
    qsganimatedtranslate.cpp \
    qsganimatedscale.cpp \
    qsganimatedrotation.cpp \
    qsgsequentialanimator.cpp \
    qsgparallelanimator.cpp \
    qsgtoplevelanimator.cpp \
    qsgpauseanimator.cpp \
    qsganimatedproperty.cpp \
    qsgpropertyanimator.cpp \
    qsgabstractanimator.cpp \
    qsgsequentialanimation.cpp \
    qsgpropertyanimation.cpp \
    qsgpauseanimation.cpp \
    qsgparallelanimation.cpp \
    qsgabstractanimation.cpp \
    qsgvector3danimation.cpp \
    qsgnumberanimation.cpp \
    qsgcoloranimation.cpp \
    qsganimatorshadereffect.cpp \
    qsganimatornode.cpp

target.path = $$[QT_INSTALL_IMPORTS]/Animators

qmldir.files = qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/Animators

INSTALLS = target qmldir




