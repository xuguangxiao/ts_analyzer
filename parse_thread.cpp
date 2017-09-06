#include "parse_thread.h"
#include <QFile>
#include "mainwindow.h"
#include <QDebug>

parse_thread::parse_thread()
{
    tpc = NULL;
}

parse_thread::~parse_thread() {
    if (tpc) {
        close_ts_parser(tpc);
        free(tpc);
    }
}

void parse_thread::start_parse(void *arg) {
    this->main_window = (MainWindow*)arg;
    start();
}

void parse_thread::run() {
    int64_t file_len = 0;
    static uint8_t ts_buf[TS_PACKET_LEN * 16];
    int64_t len;

    // analyze ts stream first
    QFile stream_file;
    stream_file.setFileName(main_window->cur_filename);
    if (!stream_file.open(QFile::ReadOnly)) {
        qDebug() << "file open failed";
        return;
    }

    file_len = stream_file.size();

    init_ts_parser(&tpc);

    while (!stream_file.atEnd()) {
        len = stream_file.read((char*)ts_buf, TS_PACKET_LEN * 16);
        if (len < 0) {
            break;
        }

        emit update_parse_progress((stream_file.pos() - len) * 50 / file_len);
        parse_ts_stream(tpc, ts_buf, len, stream_file.pos() - len);
    }

    finish_ts_stream(tpc);

    stream_file.close();

    parse_ffmpeg_stream(tpc, main_window->cur_filename.toLocal8Bit().constData(), file_len, cb_update_parse_progress, this);

    connect_ts_to_stream_packet(tpc);

    emit parse_finished();
}

void parse_thread::cb_update_parse_progress(int progress, void *context) {
    parse_thread* thread = (parse_thread*)context;
    emit thread->update_parse_progress(50 + progress / 2);
}
