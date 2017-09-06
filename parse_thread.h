#ifndef PARSE_THREAD_H
#define PARSE_THREAD_H

#include <QObject>
#include <QThread>
#include <QList>
#include "ts_parser.h"
extern "C" {
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavutil/intreadwrite.h"
}

class MainWindow;

class parse_thread : public QThread
{
    Q_OBJECT
public:
    parse_thread();
    ~parse_thread();

    void start_parse(void* arg);

    MainWindow* main_window;
    ts_parse_context* tpc;
    static void cb_update_parse_progress(int progress, void* context);

protected:
    void run();

public:
signals:
    void parse_finished();
    void update_parse_progress(int progress);
};

#endif // PARSE_THREAD_H
