#include "tsstreamdevice.h"

TSStreamDevice::TSStreamDevice(ts_parse_context* tpc, QList<bool> &ts_packet_select, QString& filename, QObject *parent)
    : QIODevice(parent)
{
    this->tpc = tpc;
    this->ts_packet_select.clear();
    this->ts_packet_select.append(ts_packet_select);
    this->filename = filename;
    cur_ts_index = 0;
    cur_ts_buf_index = 0;
}

TSStreamDevice::~TSStreamDevice() {
    if (file_handle.isOpen()) {
        file_handle.close();
    }
}

bool TSStreamDevice::isOpen() const {
    return file_handle.isOpen();
}

bool TSStreamDevice::isReadable() const {
    return file_handle.isOpen();
}

bool TSStreamDevice::isWritable() const {
    return false;
}

bool TSStreamDevice::isSequential() const {
    return true;
}

qint64 TSStreamDevice::readData(char *data, qint64 maxlen) {
    int bytes_copied = 0;
    int64_t file_pos;
    int read_len;
    int ts_packet_remain;
    if (tpc == NULL) {
        return -1;
    }

    if (cur_ts_index >= tpc->nb_ts_packets) {
        return 0;
    }

    if (!file_handle.isOpen()) {
        return -1;
    }

    while (bytes_copied < maxlen) {
        if (cur_ts_index >= tpc->nb_ts_packets) {
            return 0;
        }

        if (!ts_packet_select[cur_ts_index]) {
            cur_ts_index++;
            continue;
        }

        ts_packet_remain = TS_PACKET_LEN - cur_ts_buf_index;
        if (maxlen - bytes_copied < ts_packet_remain) {
            read_len = maxlen - bytes_copied;
        } else {
            read_len = ts_packet_remain;
        }

        file_pos = tpc->ts_packets[cur_ts_index]->start_pos + cur_ts_buf_index;
        file_handle.seek(file_pos);
        file_handle.read((char*)data + bytes_copied, read_len);
        bytes_copied += read_len;

        if (read_len < TS_PACKET_LEN) {
            cur_ts_buf_index += read_len;
            if (cur_ts_buf_index == TS_PACKET_LEN) {
                cur_ts_buf_index = 0;
                cur_ts_index ++;
            }
        } else {
            cur_ts_index ++;
        }
    }

    if (cur_ts_index < tpc->nb_ts_packets) {
        if (tpc->ts_packets[cur_ts_index]->pes_packet) {
            emit update_stream_info(cur_ts_index, tpc->ts_packets[cur_ts_index]->pes_packet->index);
        }
    }

    return bytes_copied;
}

bool TSStreamDevice::open(OpenMode mode) {
    if (mode != ReadOnly) {
        return false;
    }

    cur_ts_index = 0;
    cur_ts_buf_index = 0;

    file_handle.setFileName(filename);
    if (file_handle.open(mode)) {
        QIODevice::open(mode);
        return true;
    }

    return false;
}

void TSStreamDevice::close() {
    file_handle.close();

    cur_ts_index = 0;
    cur_ts_buf_index = 0;
}

qint64 TSStreamDevice::writeData(const char*, qint64) {
    return -1;
}
