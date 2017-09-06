#ifndef TSSTREAMDEVICE_H
#define TSSTREAMDEVICE_H

#include <QObject>
#include <QIODevice>
#include <QFile>
#include "ts_parser.h"

class TSStreamDevice : public QIODevice
{
    Q_OBJECT
public:
    TSStreamDevice(ts_parse_context* tpc, QList<bool> &ts_packet_select, QString& filename, QObject *parent = Q_NULLPTR);
    ~TSStreamDevice();

    QFile file_handle;
    ts_parse_context* tpc;
    QList<bool> ts_packet_select;
    QString filename;

    qint64 cur_ts_index;
    qint64 cur_ts_buf_index;

    bool isOpen() const;
    bool isReadable() const;
    bool isWritable() const;
    virtual bool isSequential() const;
    virtual qint64 readData(char *data, qint64 maxlen);
    virtual qint64 writeData(const char *, qint64);
    virtual bool open(OpenMode mode);
    virtual void close();
public:
signals:
    void update_stream_info(int ts_index, int pes_index);
};

#endif // TSSTREAMDEVICE_H
