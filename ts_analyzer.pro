#-------------------------------------------------
#
# Project created by QtCreator 2017-08-23T14:36:28
#
#-------------------------------------------------

QT       += core gui avwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ts_analyzer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ts_parser.c \
    parse_thread.cpp \
    h264_avcc.c \
    h264_sei.c \
    h264_stream.c \
    playdialog.cpp \
    tsstreamdevice.cpp \
    rightclickabletreewidget.cpp \
    rightclickabletablewidget.cpp \
    proto_tree.cpp \
    byte_view_text.cpp

HEADERS  += mainwindow.h \
    ts_parser.h \
    parse_thread.h \
    bs.h \
    h264_avcc.h \
    h264_sei.h \
    h264_stream.h \
    playdialog.h \
    tsstreamdevice.h \
    rightclickabletreewidget.h \
    rightclickabletablewidget.h \
    proto_tree.h \
    byte_view_text.h

FORMS    += mainwindow.ui \
    playdialog.ui

INCLUDEPATH += $$_PRO_FILE_PWD_/ffmpeg/include $$_PRO_FILE_PWD_/SDL2/include

LIBS += -L$$_PRO_FILE_PWD_/ffmpeg/lib libgcc.a libmingwex.a libiconv.a \
    avformat.lib avcodec.lib avutil.lib swscale.lib swresample.lib avfilter.lib avdevice.lib postproc.lib \
    -L$$_PRO_FILE_PWD_/SDL2/lib libSDL2.dll.a libSDL2main.a

QMAKE_CXXFLAGS += /utf-8
QMAKE_CFLAGS += /utf-8

RESOURCES += \
    ts_analyzer.qrc
