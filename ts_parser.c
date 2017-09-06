#include "ts_parser.h"
#include "ffmpeg/include/libavutil/error.h"
#include "ffmpeg/include/libavutil/intreadwrite.h"
#include "ffmpeg/include/libavutil/avutil.h"
#include "ffmpeg/include/libavcodec/avcodec.h"

#define TS_PACKET_SIZE 188

/* return the 90kHz PCR and the extension for the 27MHz PCR. return
 * (-1) if not available */
static int parse_pcr(int64_t *ppcr_high, int *ppcr_low, const uint8_t *packet)
{
    int afc, len, flags;
    const uint8_t *p;
    unsigned int v;

    afc = (packet[3] >> 4) & 3;
    if (afc <= 1)
        return AVERROR_INVALIDDATA;
    p   = packet + 4;
    len = p[0];
    p++;
    if (len == 0)
        return AVERROR_INVALIDDATA;
    flags = *p++;
    len--;
    if (!(flags & 0x10))
        return AVERROR_INVALIDDATA;
    if (len < 6)
        return AVERROR_INVALIDDATA;
    v          = AV_RB32(p);
    *ppcr_high = ((int64_t) v << 1) | (p[4] >> 7);
    *ppcr_low  = ((p[4] & 1) << 8) | p[5];
    return 0;
}

typedef struct SectionHeader {
    uint8_t tid;
    uint16_t id;
    uint8_t version;
    uint8_t sec_num;
    uint8_t last_sec_num;
} SectionHeader;

static inline int get8(const uint8_t **pp, const uint8_t *p_end)
{
    const uint8_t *p;
    int c;

    p = *pp;
    if (p >= p_end)
        return AVERROR_INVALIDDATA;
    c   = *p++;
    *pp = p;
    return c;
}

static inline int get16(const uint8_t **pp, const uint8_t *p_end)
{
    const uint8_t *p;
    int c;

    p = *pp;
    if (1 >= p_end - p)
        return AVERROR_INVALIDDATA;
    c   = AV_RB16(p);
    p  += 2;
    *pp = p;
    return c;
}

/* read and allocate a DVB string preceded by its length */
static char *getstr8(const uint8_t **pp, const uint8_t *p_end)
{
    int len;
    const uint8_t *p;
    char *str;

    p   = *pp;
    len = get8(&p, p_end);
    if (len < 0)
        return NULL;
    if (len > p_end - p)
        return NULL;
    str = av_malloc(len + 1);
    if (!str)
        return NULL;
    memcpy(str, p, len);
    str[len] = '\0';
    p  += len;
    *pp = p;
    return str;
}

static int parse_section_header(SectionHeader *h,
                                const uint8_t **pp, const uint8_t *p_end)
{
    int val;

    val = get8(pp, p_end);
    if (val < 0)
        return val;
    h->tid = val;
    *pp += 2;
    val  = get16(pp, p_end);
    if (val < 0)
        return val;
    h->id = val;
    val = get8(pp, p_end);
    if (val < 0)
        return val;
    h->version = (val >> 1) & 0x1f;
    val = get8(pp, p_end);
    if (val < 0)
        return val;
    h->sec_num = val;
    val = get8(pp, p_end);
    if (val < 0)
        return val;
    h->last_sec_num = val;
    return 0;
}

static int parse_sdt(ts_parse_context* tpc, const uint8_t* section, int section_len) {
    SectionHeader h1, *h = &h1;
    const uint8_t *p, *p_end, *desc_list_end, *desc_end;
    int onid, val, sid, desc_list_len, desc_tag, desc_len, service_type;
    char *name, *provider_name;

    p_end = section + section_len - 4;
    p     = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return -1;
    if (h->tid != SDT_TID)
        return -1;

    onid = get16(&p, p_end);
    if (onid < 0)
        return -1;
    tpc->sdt.onid = onid;
    val = get8(&p, p_end);
    if (val < 0)
        return -1;
    for (;;) {
        sid = get16(&p, p_end);
        if (sid < 0)
            break;
        tpc->sdt.sid = sid;
        val = get8(&p, p_end);
        if (val < 0)
            break;
        desc_list_len = get16(&p, p_end);
        if (desc_list_len < 0)
            break;
        desc_list_len &= 0xfff;
        desc_list_end  = p + desc_list_len;
        if (desc_list_end > p_end)
            break;
        for (;;) {
            desc_tag = get8(&p, desc_list_end);
            if (desc_tag < 0)
                break;
            desc_len = get8(&p, desc_list_end);
            desc_end = p + desc_len;
            if (desc_len < 0 || desc_end > desc_list_end)
                break;

            switch (desc_tag) {
            case 0x48:
                service_type = get8(&p, p_end);
                if (service_type < 0)
                    break;
                provider_name = getstr8(&p, p_end);
                if (!provider_name)
                    break;
                name = getstr8(&p, p_end);
                if (name) {
                    if (tpc->sdt.provider) av_freep(&tpc->sdt.provider);
                    if (tpc->sdt.name) av_freep(&tpc->sdt.name);
                    tpc->sdt.provider = av_strdup(provider_name);
                    tpc->sdt.name = av_strdup(name);
                }
                break;
            default:
                break;
            }
            p = desc_end;
        }
        p = desc_list_end;
    }
    return 0;
}

static int parse_pat(ts_parse_context* tpc, const uint8_t* section, int section_len) {
    SectionHeader h1, *h = &h1;
    const uint8_t *p, *p_end;
    int sid, pmt_pid;

    p_end = section + section_len - 4;
    p     = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return -1;
    if (h->tid != PAT_TID)
        return -1;

    for (;;) {
        sid = get16(&p, p_end);
        if (sid < 0)
            break;
        pmt_pid = get16(&p, p_end);
        if (pmt_pid < 0)
            break;
        pmt_pid &= 0x1fff;

        if (pmt_pid == tpc->pmt_pid)
            break;

        tpc->pmt_pid = pmt_pid;
    }

    return 0;
}


typedef struct StreamType {
    uint32_t stream_type;
    enum AVMediaType codec_type;
    enum AVCodecID codec_id;
} StreamType;

static const StreamType ISO_types[] = {
    { 0x01, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MPEG2VIDEO },
    { 0x02, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MPEG2VIDEO },
    { 0x03, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_MP3        },
    { 0x04, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_MP3        },
    { 0x0f, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC        },
    { 0x10, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MPEG4      },
    /* Makito encoder sets stream type 0x11 for AAC,
     * so auto-detect LOAS/LATM instead of hardcoding it. */
#if !CONFIG_LOAS_DEMUXER
    { 0x11, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC_LATM   }, /* LATM syntax */
#endif
    { 0x1b, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H264       },
    { 0x1c, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC        },
    { 0x20, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H264       },
    { 0x21, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_JPEG2000   },
    { 0x24, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_HEVC       },
    { 0x42, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_CAVS       },
    { 0xd1, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_DIRAC      },
    { 0xea, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_VC1        },
    { 0 },
};

static enum AVMediaType mpegts_find_stream_type(uint32_t stream_type,
                                    const StreamType *types)
{
    for (; types->stream_type; types++) {
        if (stream_type == types->stream_type) {
            return types->codec_type;
        }
    }

    return AVMEDIA_TYPE_UNKNOWN;
}

static int parse_pmt(ts_parse_context* tpc, const uint8_t* section, int section_len) {
    SectionHeader h1, *h = &h1;
    const uint8_t *p, *p_end, *desc_list_end;
    int program_info_length, pcr_pid, pid, stream_type;
    int desc_list_len;
    uint32_t prog_reg_desc = 0; /* registration descriptor */
    enum AVMediaType media_type;
    int mpeg2_desc_tag;
    int mpeg2_desc_len;
    const uint8_t *mpeg2_desc_end;
    int i;
    char language[128];

    p_end = section + section_len - 4;
    p = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return -1;

    if (h->tid != PMT_TID)
        return -1;

    pcr_pid = get16(&p, p_end);
    if (pcr_pid < 0)
        return -1;
    pcr_pid &= 0x1fff;

    tpc->cur_pmt.pcr_pid = pcr_pid;

    program_info_length = get16(&p, p_end);
    if (program_info_length < 0)
        return -1;
    program_info_length &= 0xfff;
#if 0
    while (program_info_length >= 2) {
        uint8_t tag, len;
        tag = get8(&p, p_end);
        len = get8(&p, p_end);

        if (len > program_info_length - 2)
            // something else is broken, exit the program_descriptors_loop
            break;
        program_info_length -= len + 2;
        if (tag == 0x1d) { // IOD descriptor
            get8(&p, p_end); // scope
            get8(&p, p_end); // label
            len -= 2;
            mp4_read_iods(ts->stream, p, len, mp4_descr + mp4_descr_count,
                          &mp4_descr_count, MAX_MP4_DESCR_COUNT);
        } else if (tag == 0x05 && len >= 4) { // registration descriptor
            prog_reg_desc = bytestream_get_le32(&p);
            len -= 4;
        }
        p += len;
    }
#endif
    p += program_info_length;
    if (p >= p_end)
        return -1;

    for (;;) {
        stream_type = get8(&p, p_end);
        if (stream_type < 0)
            break;
        pid = get16(&p, p_end);
        if (pid < 0)
            break;
        pid &= 0x1fff;
        if (pid == tpc->pmt_pid)
            break;

        media_type = mpegts_find_stream_type(stream_type, ISO_types);
        desc_list_len = get16(&p, p_end);
        if (desc_list_len < 0)
            break;
        desc_list_len &= 0xfff;
        desc_list_end  = p + desc_list_len;
        if (desc_list_end > p_end)
            break;

        if (media_type == AVMEDIA_TYPE_VIDEO) {
            tpc->cur_pmt.video_pid = pid;
        } else if (media_type == AVMEDIA_TYPE_AUDIO) {
            tpc->cur_pmt.audio_pid = pid;
        } else {
            p = desc_list_end;
            continue;
        }

        mpeg2_desc_tag = get8(&p, desc_list_end);
        if (mpeg2_desc_tag < 0) {
            p = desc_list_end;
            continue;
        }

        mpeg2_desc_len = get8(&p, desc_list_end);
        if (mpeg2_desc_len < 0) {
            p = desc_list_end;
            continue;
        }

        mpeg2_desc_end = p + mpeg2_desc_len;

        if (mpeg2_desc_tag == 0x0a) { /* ISO 639 language descriptor */
            for (i = 0; i + 4 <= mpeg2_desc_len; i += 4) {
                language[i + 0] = get8(&p, mpeg2_desc_end);
                language[i + 1] = get8(&p, mpeg2_desc_end);
                language[i + 2] = get8(&p, mpeg2_desc_end);
                language[i + 3] = ',';
                switch (get8(&p, mpeg2_desc_end)) {
                case 0x01:
                    break;
                case 0x02:
                    break;
                case 0x03:
                    break;
                }
            }
            if (i && language[0]) {
                language[i - 1] = 0;
                tpc->cur_pmt.language = av_strdup(language);
            }
        }

        p = desc_list_end;
    }

    return 0;
}

static void init_ts_packet_info(ts_packet_info* pkt) {
    pkt->pid = -1;
    pkt->cc_serial = -1;
    pkt->adaption_flag = 0;
    pkt->payload_flag = 0;
    pkt->start_indicator = 0;
    pkt->discontinuity = 0;
    pkt->adaption_length = 0;
    pkt->pcr = 0;
    pkt->type = -1;
    pkt->pes_packet = NULL;
}

static pes_packet_info* pending_pes_packet = NULL;

static int64_t get_ptsdts(const uint8_t* buf) {
    const uint8_t *p, *p_end;
    int64_t timestamp;
    p = buf;
    p_end = p + 5;
    timestamp = (get8(&p, p_end) & 0x0E) << 29;
    timestamp |= (get16(&p, p_end) & 0xFFFE) << 14;
    timestamp |= (get16(&p, p_end) & 0xFFFE) >> 1;
    return timestamp;
}

static int parse_pes_packet(ts_parse_context* tpc, pes_packet_info* pes_packet) {
    const uint8_t *p, *p_end;
    int pes_ptsdts_flags;
    int pes_header_len;
    int start_code_len;
    int nal_len;
    const uint8_t* nal_start;
    void* nal_parsed;

    if (pes_packet->len < 9) {
        return -1;
    }

    p = pes_packet->data;
    p_end = pes_packet->data + pes_packet->len;
    p += 7;
    pes_ptsdts_flags = get8(&p, p_end);
    pes_header_len = get8(&p, p_end);
    if (p_end - p < pes_header_len) {
        return -1;
    }

    if (pes_ptsdts_flags & 0x80) {
        pes_packet->pts = get_ptsdts(p);
        p += 5;
    }

    if (pes_ptsdts_flags & 0x40) {
        pes_packet->dts = get_ptsdts(p);
        p += 5;
    }

    if (pes_packet->ts_pid != tpc->cur_pmt.video_pid) {
        return 0 ;
    }

    // nal start here
    while (p < p_end) {
        start_code_len = 0;
        nal_len = 0;
        while (p < p_end) {
            if (p[0] == 0 && p[1] == 0 && p[2] == 1) {
                start_code_len = 3;
                break;
            }

            if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) {
                start_code_len = 4;
                break;
            }

            p ++;
        }

        if (start_code_len) {
            p += start_code_len;
            nal_start = p;
            while (p < p_end) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) {
                    break;
                }

                if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) {
                    break;
                }

                p ++;
            }

            nal_len = p - nal_start;
            read_nal_unit(tpc->h264_stream, (uint8_t*)nal_start, nal_len);

            pes_packet->nb_nals++;
            pes_packet->nals = (nal_t**)realloc(pes_packet->nals, sizeof(nal_t*) * pes_packet->nb_nals);
            pes_packet->nals[pes_packet->nb_nals - 1] = (nal_t*)malloc(sizeof(nal_t));
            memcpy(pes_packet->nals[pes_packet->nb_nals - 1], tpc->h264_stream->nal, sizeof(nal_t));
            nal_parsed = pes_packet->nals[pes_packet->nb_nals - 1]->parsed;
            pes_packet->nals[pes_packet->nb_nals - 1]->parsed = malloc(pes_packet->nals[pes_packet->nb_nals - 1]->sizeof_parsed);
            memcpy(pes_packet->nals[pes_packet->nb_nals - 1]->parsed, nal_parsed, pes_packet->nals[pes_packet->nb_nals - 1]->sizeof_parsed);
            pes_packet->nals[pes_packet->nb_nals - 1]->len = nal_len;
        }
    }

    return 0;
}

static int submit_pes_packet(ts_parse_context* tpc, ts_packet_info* ts_packet, const uint8_t* buf, int buf_len) {
    int pes_len;
    const uint8_t *p, *p_end;
    int pes_stream_id;
    p = buf;
    p_end = buf + buf_len;
    if (ts_packet->start_indicator) {
        if (p[0] != 0 || p[1] != 0 || p[2] != 1) {
            return -1;
        }

        pes_stream_id = p[3];

        p += 4;
        pes_len = get16(&p, p_end);
        if (pes_len < 0) {
            return -1;
        }

        if (pending_pes_packet) {
            if (pending_pes_packet->len == 0) {
                pending_pes_packet->len = pending_pes_packet->data_index;
            }
            pending_pes_packet->end_ts_index = ts_packet->index;
            parse_pes_packet(tpc, pending_pes_packet);
            tpc->nb_pes_packets++;
            tpc->pes_packets = (pes_packet_info**)realloc(tpc->pes_packets, sizeof(pes_packet_info*) * tpc->nb_pes_packets);
            tpc->pes_packets[tpc->nb_pes_packets - 1] = pending_pes_packet;
            pending_pes_packet = NULL;
        }

        if (pes_len > 0) {
            pending_pes_packet = (pes_packet_info*)calloc(1, sizeof(pes_packet_info));
            pending_pes_packet->ts_start_pos = ts_packet->start_pos;
            pending_pes_packet->index = tpc->nb_pes_packets;
            pending_pes_packet->len = pes_len + 6;
            pending_pes_packet->ts_pid = ts_packet->pid;
            pending_pes_packet->stream_id = pes_stream_id;
            pending_pes_packet->start_ts_index = ts_packet->index;
            pending_pes_packet->data = (uint8_t*)malloc(pes_len + 6 + PES_TRAIL_PAD_LEN);
            memcpy(pending_pes_packet->data, buf, buf_len);
            memset(pending_pes_packet->data + pes_len + 6, PES_TRAIL_PAD_FILL_BYTE, PES_TRAIL_PAD_LEN);
            pending_pes_packet->data_index = buf_len;

            ts_packet->pes_packet = pending_pes_packet;

            if (pending_pes_packet->len == buf_len) {
                pending_pes_packet->end_ts_index = ts_packet->index;
                parse_pes_packet(tpc, pending_pes_packet);
                tpc->nb_pes_packets++;
                tpc->pes_packets = (pes_packet_info**)realloc(tpc->pes_packets, sizeof(pes_packet_info*) * tpc->nb_pes_packets);
                tpc->pes_packets[tpc->nb_pes_packets - 1] = pending_pes_packet;
                pending_pes_packet = NULL;
            }
        } else {
            pending_pes_packet = (pes_packet_info*)calloc(1, sizeof(pes_packet_info));
            pending_pes_packet->ts_start_pos = ts_packet->start_pos;
            pending_pes_packet->index = tpc->nb_pes_packets;
            pending_pes_packet->len = 0;
            pending_pes_packet->ts_pid = ts_packet->pid;
            pending_pes_packet->stream_id = pes_stream_id;
            pending_pes_packet->start_ts_index = ts_packet->index;
            pending_pes_packet->data = (uint8_t*)malloc(buf_len + PES_TRAIL_PAD_LEN);
            memcpy(pending_pes_packet->data, buf, buf_len);
            memset(pending_pes_packet->data + buf_len, PES_TRAIL_PAD_FILL_BYTE, PES_TRAIL_PAD_LEN);
            pending_pes_packet->data_index = buf_len;
            ts_packet->pes_packet = pending_pes_packet;
        }
    } else {
        if (!pending_pes_packet) {
            return 0;
        }

        ts_packet->pes_packet = pending_pes_packet;

        if (pending_pes_packet->len == 0) {
            pending_pes_packet->data = realloc(pending_pes_packet->data, pending_pes_packet->data_index + buf_len);
            memcpy(pending_pes_packet->data + pending_pes_packet->data_index, buf, buf_len);
            pending_pes_packet->data_index += buf_len;
        } else {
            if (pending_pes_packet->len - pending_pes_packet->data_index < buf_len) {
                return -1;
            }

            if (pending_pes_packet->len - pending_pes_packet->data_index == buf_len) {
                // pes last packet
                memcpy(pending_pes_packet->data + pending_pes_packet->data_index, buf, buf_len);
                pending_pes_packet->data_index += buf_len;
                pending_pes_packet->end_ts_index = ts_packet->index;
                parse_pes_packet(tpc, pending_pes_packet);
                tpc->nb_pes_packets++;
                tpc->pes_packets = (pes_packet_info**)realloc(tpc->pes_packets, sizeof(pes_packet_info*) * tpc->nb_pes_packets);
                tpc->pes_packets[tpc->nb_pes_packets - 1] = pending_pes_packet;
                pending_pes_packet = NULL;
            } else {
                memcpy(pending_pes_packet->data + pending_pes_packet->data_index, buf, buf_len);
                pending_pes_packet->data_index += buf_len;
            }
        }
    }

    return 0;
}

// this code from ffmpeg mpegts.c handle_packet() function and
static int parse_ts_packet(ts_parse_context* tpc, const uint8_t *packet, ts_packet_info *ts_packet) {
    int len, pid, cc, afc, is_start, is_discontinuity,
        has_adaptation, has_payload;
    const uint8_t *p, *p_end;

    init_ts_packet_info(ts_packet);

    pid = AV_RB16(packet + 1) & 0x1fff;
    ts_packet->pid = pid;
    is_start = packet[1] & 0x40;

    ts_packet->start_indicator = !!is_start;

    afc = (packet[3] >> 4) & 3;
    if (afc == 0) /* reserved value */
        return 0;

    has_adaptation   = afc & 2;

    ts_packet->adaption_flag = !!has_adaptation;

    has_payload      = afc & 1;

    ts_packet->payload_flag = !!has_payload;

    is_discontinuity = has_adaptation &&
                       packet[4] != 0 && /* with length > 0 */
                       (packet[5] & 0x80); /* and discontinuity indicated */

    ts_packet->discontinuity = is_discontinuity;

    /* continuity check (currently not used) */
    cc = (packet[3] & 0xf);

    ts_packet->cc_serial = cc;

    p = packet + 4;
    if (has_adaptation) {
        int64_t pcr_h;
        int pcr_l;
        if (parse_pcr(&pcr_h, &pcr_l, packet) == 0)
            ts_packet->pcr = pcr_h * 300 + pcr_l;
        /* skip adaptation field */
        ts_packet->adaption_length = p[0];
        p += p[0] + 1;
    }
    /* if past the end of packet, ignore */
    p_end = packet + TS_PACKET_SIZE;
    if (p >= p_end || !has_payload)
        return 0;

    // parse sdt/pat/pmt
    if (pid == SDT_PID) {
        // parse sdt
        /* pointer field present */
        len = *p++;
        if (len > p_end - p)
            return 0;
        if (parse_sdt(tpc, p, (AV_RB16(p + 1) & 0xfff) + 3) < 0) {
            return -1;
        }

        ts_packet->type = MPEGTS_SECTION;
    } else if (pid == PAT_PID) {
        // parse pat
        /* pointer field present */
        len = *p++;
        if (len > p_end - p)
            return 0;
        if (parse_pat(tpc, p, (AV_RB16(p + 1) & 0xfff) + 3) < 0) {
            return -1;
        }

        ts_packet->type = MPEGTS_SECTION;
    } else if (tpc->pmt_pid >= 0 && pid == tpc->pmt_pid) {
        // parse pmt
        /* pointer field present */
        len = *p++;
        if (len > p_end - p)
            return 0;
        if (parse_pmt(tpc, p, (AV_RB16(p + 1) & 0xfff) + 3) < 0) {
            return -1;
        }

        ts_packet->type = MPEGTS_SECTION;
    } else if (pid == tpc->cur_pmt.video_pid) {
        // video pes packet
        ts_packet->type = MPEGTS_PES;
        submit_pes_packet(tpc, ts_packet, p, p_end - p);
    } else if (pid == tpc->cur_pmt.audio_pid) {
        // audio pes packet
        ts_packet->type = MPEGTS_PES;
        submit_pes_packet(tpc, ts_packet, p, p_end - p);
    }

    return 0;
}

void init_ts_parser(ts_parse_context** tpc) {
    ts_parse_context* p;
    if (*tpc == NULL) {
        *tpc = (ts_parse_context*)malloc(sizeof(ts_parse_context));
    } else {
        close_ts_parser(*tpc);
    }

    memset(*tpc, 0, sizeof(ts_parse_context));

    p = *tpc;

    p->pmt_pid = -1;
    p->sdt.onid = -1;
    p->sdt.sid = -1;
    p->cur_pmt.pcr_pid = -1;
    p->cur_pmt.video_pid = -1;
    p->cur_pmt.audio_pid = -1;
    p->nb_ts_packets = 0;
    p->ts_packets = NULL;
    p->nb_pes_packets = 0;
    p->pes_packets = NULL;
    p->h264_stream = h264_new();
    p->nb_stream_packets = 0;
    p->stream_packets = NULL;
}

void close_ts_parser(ts_parse_context *tpc) {
    int i,j;
    if (tpc->sdt.provider) {
        av_freep(&tpc->sdt.provider);
    }

    if (tpc->sdt.name) {
        av_freep(&tpc->sdt.name);
    }

    if (tpc->cur_pmt.language) {
        av_freep(&tpc->cur_pmt.language);
    }

    if (tpc->nb_ts_packets > 0) {
        for (i=0;i<tpc->nb_ts_packets;i++) {
            free(tpc->ts_packets[i]);
        }
    }

    if (tpc->nb_pes_packets > 0) {
        for (i=0;i<tpc->nb_pes_packets;i++) {
            if (tpc->pes_packets[i]->data) {
                free(tpc->pes_packets[i]->data);
                if (tpc->pes_packets[i]->nb_nals > 0) {
                    for (j=0;j<tpc->pes_packets[i]->nb_nals;j++) {
                        if (tpc->pes_packets[i]->nals[j]->parsed) {
                            free(tpc->pes_packets[i]->nals[j]->parsed);
                        }
                        free(tpc->pes_packets[i]->nals[j]);
                    }
                }
            }
            free(tpc->pes_packets[i]);
        }
    }

    h264_free(tpc->h264_stream);

    if (tpc->nb_stream_packets > 0) {
        for (i=0;i<tpc->nb_stream_packets;i++) {
            av_packet_free(&tpc->stream_packets[i].packet);
        }

        free(tpc->stream_packets);
    }

    if (tpc->ic) {
        avformat_close_input(&tpc->ic);
    }

    if (pending_pes_packet) {
        free(pending_pes_packet);
    }
}

void connect_ts_to_stream_packet(ts_parse_context* tpc) {
    int i;
    int cur_ts_index = 0;
    int last_ts_index = -1;
    if (tpc->nb_ts_packets == 0) {
        return;
    }

    if (tpc->nb_stream_packets == 0) {
        return;
    }

    for (i=0;i<tpc->nb_stream_packets;i++) {
        cur_ts_index = 0;
        if (tpc->stream_packets[i].packet->pos >= 0) {
            while (cur_ts_index < tpc->nb_ts_packets) {
                if (tpc->ts_packets[cur_ts_index]->start_pos <= tpc->stream_packets[i].packet->pos
                        && tpc->ts_packets[cur_ts_index]->start_pos + TS_PACKET_LEN > tpc->stream_packets[i].packet->pos) {
                    break;
                }

                cur_ts_index ++;
            }

            if (cur_ts_index < tpc->nb_ts_packets) {
                tpc->stream_packets[i].start_ts_index = cur_ts_index;
                if (tpc->ts_packets[cur_ts_index]->pes_packet) {
                    tpc->stream_packets[i].end_ts_index = tpc->ts_packets[cur_ts_index]->pes_packet->end_ts_index;
                }

                last_ts_index = cur_ts_index;
            }
        } else {
            if (last_ts_index >= 0 && last_ts_index < tpc->nb_ts_packets) {
                tpc->stream_packets[i].start_ts_index = last_ts_index;
                if (tpc->ts_packets[last_ts_index]->pes_packet) {
                    tpc->stream_packets[i].end_ts_index = tpc->ts_packets[last_ts_index]->pes_packet->end_ts_index;
                }
            }
        }
    }
}

static uint8_t pending_ts_stream_data[TS_PACKET_LEN];
static int pending_ts_stream_data_len = 0;

int parse_ts_stream(ts_parse_context* tpc, uint8_t* buf, int len, int64_t cur_pos) {
    int buf_index = 0;
    ts_packet_info* ts_packet;
    int64_t ts_packet_start_pos = cur_pos;
    uint8_t* ts_buf;
    int i, j;

    if (len + pending_ts_stream_data_len < TS_PACKET_LEN) {
        memcpy(&pending_ts_stream_data[pending_ts_stream_data_len], buf, len);
        pending_ts_stream_data_len += len;
        return 0;
    }

    while (len - buf_index >= TS_PACKET_LEN) {
        if (pending_ts_stream_data_len > 0) {
            memcpy(&pending_ts_stream_data[pending_ts_stream_data_len], buf, TS_PACKET_LEN - pending_ts_stream_data_len);
            buf_index += TS_PACKET_LEN - pending_ts_stream_data_len;
            if (pending_ts_stream_data[0] != TS_STREAM_TAG) {
                i = 1;
                while (i < TS_PACKET_LEN) {
                    if (pending_ts_stream_data[i] == TS_STREAM_TAG) {
                        break;
                    }
                    i ++;
                }

                if (i >= TS_PACKET_LEN) {
                    ts_buf = buf + buf_index;
                } else {
                    for (j=0;j<TS_PACKET_LEN - i - 1;j++) {
                        pending_ts_stream_data[j] = pending_ts_stream_data[i + j];
                    }

                    if (len - buf_index - i - 1 < TS_PACKET_LEN - j) {
                        memcpy(&pending_ts_stream_data[j], buf + buf_index, len - buf_index - i - 1);
                        pending_ts_stream_data_len += len - buf_index - i - 1;
                        return 0;
                    } else {
                        memcpy(&pending_ts_stream_data[j], buf + buf_index, TS_PACKET_LEN - j);
                        buf_index += TS_PACKET_LEN - j;
                    }
                }
            }
            ts_buf = pending_ts_stream_data;
            ts_packet_start_pos = cur_pos - (TS_PACKET_LEN - buf_index);
        } else {
            i = buf_index;
            if (buf[i] != TS_STREAM_TAG) {
                while (i < len) {
                    if (buf[i] == TS_STREAM_TAG) {
                        break;
                    }
                }

                if (i >= len) {
                    return 0;
                }
            }
            ts_buf = buf + i;
            ts_packet_start_pos = cur_pos + i;
            buf_index += TS_PACKET_LEN;
        }

        ts_packet = (ts_packet_info*)malloc(sizeof(ts_packet_info));
        ts_packet->start_pos = ts_packet_start_pos;
        ts_packet->len = TS_PACKET_LEN;
        ts_packet->index = tpc->nb_ts_packets;
        parse_ts_packet(tpc, ts_buf, ts_packet);
        tpc->nb_ts_packets++;
        tpc->ts_packets = (ts_packet_info**)realloc(tpc->ts_packets, sizeof(ts_packet_info*) * tpc->nb_ts_packets);
        tpc->ts_packets[tpc->nb_ts_packets - 1] = ts_packet;
        if (pending_ts_stream_data_len > 0) {
            pending_ts_stream_data_len = 0;
        }
    }

    if (len - buf_index > 0) {
        memcpy(pending_ts_stream_data, buf + buf_index, len - buf_index);
        pending_ts_stream_data_len = len - buf_index;
    }

    return 0;
}

// if we have pending pes packet, commit it here
void finish_ts_stream(ts_parse_context* tpc) {
    if (pending_pes_packet) {
        if (pending_pes_packet->len == 0) {
            pending_pes_packet->len = pending_pes_packet->data_index;
        }
        pending_pes_packet->end_ts_index = tpc->ts_packets[tpc->nb_ts_packets - 1]->index;
        parse_pes_packet(tpc, pending_pes_packet);
        tpc->nb_pes_packets++;
        tpc->pes_packets = (pes_packet_info**)realloc(tpc->pes_packets, sizeof(pes_packet_info*) * tpc->nb_pes_packets);
        tpc->pes_packets[tpc->nb_pes_packets - 1] = pending_pes_packet;
        pending_pes_packet = NULL;
    }
}

int parse_ffmpeg_stream(ts_parse_context* tpc, const char* file_name, int64_t file_len, pfn_update_progress pfn_progress, void* progress_context) {
    int err;
    AVPacket* pkt;
    AVInputFormat *file_iformat;

    file_iformat = av_find_input_format("mpegts");

    err = avformat_open_input(&tpc->ic, file_name, file_iformat, NULL);
    if (err) {
        return -1;
    }

    err = avformat_find_stream_info(tpc->ic, NULL);
    if (err) {
        return -1;
    }

    while (1) {
        pkt = av_packet_alloc();
        err = av_read_frame(tpc->ic, pkt);
        if (err == AVERROR_EOF) {
            av_packet_free(&pkt);
            break;
        }

        if (pkt->pos > 0) {
            if (pfn_progress) {
                pfn_progress((int)(pkt->pos * 100 / file_len), progress_context);
            }
        }

        tpc->nb_stream_packets++;
        tpc->stream_packets = (stream_packet_info*)realloc(tpc->stream_packets, sizeof(stream_packet_info) * tpc->nb_stream_packets);
        tpc->stream_packets[tpc->nb_stream_packets - 1].packet = pkt;
        tpc->stream_packets[tpc->nb_stream_packets - 1].start_ts_index = -1;
        tpc->stream_packets[tpc->nb_stream_packets - 1].end_ts_index = -1;
    }

    return 0;
}

const char* get_slice_type(slice_header_t *sh) {
    switch (sh->slice_type) {
    case 0:
        return "P";
    case 1:
        return "B";
    case 2:
        return "I";
    case 3:
        return "SP";
    case 4:
        return "SI";
    case 5:
        return "P";
    case 6:
        return "B";
    case 7:
        return "I";
    case 8:
        return "SP";
    case 9:
        return "SI";
    default:
        return "";
    }
}
