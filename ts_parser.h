#ifndef TS_PARSER_H
#define TS_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavutil/intreadwrite.h"
#include "h264_stream.h"

typedef struct pes_packet_info {
    int64_t ts_start_pos;
    int index;
    int start_ts_index;
    int end_ts_index;
    uint8_t stream_id;
    int ts_pid;
    int len;
    int data_index;
    int64_t pts;
    int64_t dts;
    uint8_t* data;
    int nb_nals;
    nal_t** nals;
    // AVPacket* related_stream_packet;
}pes_packet_info;

#define PES_TRAIL_PAD_LEN 32
#define PES_TRAIL_PAD_FILL_BYTE 0xFF

typedef struct ts_packet_info {
    int64_t start_pos;
    int index;
    int len;
    int pid;
    int cc_serial;
    char adaption_flag;
    char payload_flag;
    char start_indicator;
    char discontinuity;
    int adaption_length;
    int64_t pcr;
    int type;
    pes_packet_info* pes_packet;
}ts_packet_info;

typedef struct ts_sdt {
    int onid;
    int sid;
    char* provider;
    char* name;
}ts_sdt;

typedef struct ts_pmt {
    int pcr_pid;
    int video_pid;
    int audio_pid;
    char* language;
}ts_pmt;

typedef struct stream_packet_info {
    AVPacket* packet;
    int64_t start_ts_index;
    int64_t end_ts_index;
}stream_packet_info;

typedef struct ts_parse_context {
    int pmt_pid;  // from PAT
    ts_sdt sdt;
    ts_pmt cur_pmt;
    int nb_ts_packets;
    ts_packet_info** ts_packets;
    int nb_pes_packets;
    pes_packet_info** pes_packets;
    h264_stream_t* h264_stream;
    AVFormatContext *ic;
    int nb_stream_packets;
    stream_packet_info* stream_packets;
}ts_parse_context;

/* pids */
#define PAT_PID                 0x0000
#define SDT_PID                 0x0011

/* table ids */
#define PAT_TID   0x00
#define PMT_TID   0x02
#define M4OD_TID  0x05
#define SDT_TID   0x42

enum MpegTSFilterType {
    MPEGTS_PES,
    MPEGTS_SECTION,
    MPEGTS_PCR,
};

#define TS_PACKET_LEN 188

#define TS_STREAM_TAG 0x47

void init_ts_parser(ts_parse_context** tpc);

void close_ts_parser(ts_parse_context* tpc);

int parse_ts_stream(ts_parse_context* tpc, uint8_t* buf, int len, int64_t cur_pos);

void finish_ts_stream(ts_parse_context* tpc);

typedef void (*pfn_update_progress)(int progress, void* context);

int parse_ffmpeg_stream(ts_parse_context* tpc, const char* file_name, int64_t file_len, pfn_update_progress pfn_progress, void *progress_context);

void connect_ts_to_stream_packet(ts_parse_context* tpc);

const char* get_slice_type(slice_header_t* sh);

#ifdef __cplusplus
}
#endif

#endif // TS_PARSER_H
