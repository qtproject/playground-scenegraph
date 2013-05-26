TEMPLATE = lib
TARGET = shapes

QT += quick
QT += v8-private qml-private quick-private gui-private core-private # to get access to QPainter's triangulator

CONFIG += qt plugin

SOURCES += \
    shapesplugin.cpp \
    qsgmaskedimage.cpp \
    qsgtriangleset.cpp \
    qsgpolygon.cpp

HEADERS += \
    qsgmaskedimage.h \
    qsgtriangleset.h \
    qsgpolygon.h

TARGETPATH=Qt/labs/shapes

OTHER_FILES = qmldir

target.path = $$[QT_INSTALL_QML]/$$TARGETPATH

qmldir.files = qmldir
qmldir.path = $$[QT_INSTALL_QML]/$$TARGETPATH

INSTALLS = target qmldir
