/* This file is from wireshark, just keep the render codes.
 * All wireshark's main skeletons are removed
 */
/* proto_tree.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>

#include "proto_tree.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QScrollBar>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QDebug>
#include "ts_parser.h"

QTreeWidgetItem* ProtoTree::proto_tree_draw_node(const QString& text, int type, QTreeWidgetItem* parent, int idx_buf_info)
{
    QTreeWidgetItem *item;

    item = new QTreeWidgetItem(parent, 0);

    QPalette pal = QApplication::palette();
    if (type == TREE_NODE_TYPE_FIELD) {
        item->setData(0, Qt::BackgroundRole, pal.window());
        item->setData(0, Qt::ForegroundRole, pal.windowText());
    }

    item->setText(0, text);
    item->setData(0, Qt::UserRole, QVariant::fromValue(idx_buf_info));
    return item;
}

void ProtoTree::draw_ffmpeg_packet(AVPacket* packet, uint8_t* raw_buf) {

}

void ProtoTree::draw_ts_packet(ts_packet_info* packet, uint8_t* raw_buf) {
    QString text;
    tree_item_buf_info* tibi = NULL;
    QTreeWidgetItem* item;

    // header
    text = tr("TS Header (0x%1%2%3%4)")
            .arg((int)raw_buf[0], 2, 16, QChar('0'))
            .arg((int)raw_buf[1], 2, 16, QChar('0'))
            .arg((int)raw_buf[2], 2, 16, QChar('0'))
            .arg((int)raw_buf[3], 2, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 0;
    tibi->byte_len = 4;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

    text = tr("%1 = Sync Byte 0x%2").arg(toBinaryStr(raw_buf, (uint8_t)0xFF)).arg((int)raw_buf[0], 2, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 0;
    tibi->byte_len = 1;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Transport Error Indicator: %2").arg(toBinaryStr(raw_buf + 1, (uint8_t)0x80)).arg(raw_buf[1] & 0x80 ? 1 : 0);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 1;
    tibi->byte_len = 0;
    tibi->start_bit = 0;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Payload Unit Start Indicator: %2").arg(toBinaryStr(raw_buf + 1, (uint8_t)0x40)).arg((int)packet->start_indicator);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 1;
    tibi->byte_len = 0;
    tibi->start_bit = 1;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Transport Priority: %2").arg(toBinaryStr(raw_buf + 1, (uint8_t)0x20)).arg((int)(raw_buf[1] & 0x20 ? 1 : 0));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 1;
    tibi->byte_len = 0;
    tibi->start_bit = 2;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = PID: 0x%2").arg(toBinaryStr(raw_buf + 1, (uint16_t)0x1FFF)).arg(packet->pid, 4, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 1;
    tibi->byte_len = 0;
    tibi->start_bit = 3;
    tibi->bit_len = 13;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Transport Scrambling Control: 0x%2").arg(toBinaryStr(raw_buf + 3, (uint8_t)0xC0)).arg((raw_buf[3] >> 6) & 0x03, 1, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 3;
    tibi->byte_len = 0;
    tibi->start_bit = 0;
    tibi->bit_len = 2;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Adaption Field Control: %2 (0x%3)").arg(toBinaryStr(raw_buf + 3, (uint8_t)0x30))
            .arg(packet->payload_flag && packet->adaption_flag ? "Both" : (packet->payload_flag ? "Payload Only" : (packet->adaption_flag ? "Adaption Field Only" : "None")))
            .arg((raw_buf[3] >> 4) & 0x03, 1, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 3;
    tibi->byte_len = 0;
    tibi->start_bit = 2;
    tibi->bit_len = 2;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = Continuity Counter: %2").arg(toBinaryStr(raw_buf + 3, (uint8_t)0x0F)).arg(raw_buf[3] & 0x0F);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 3;
    tibi->byte_len = 0;
    tibi->start_bit = 4;
    tibi->bit_len = 4;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    if (packet->adaption_flag) {
        text = tr("Adaption Field (%1 bytes)").arg(packet->adaption_length + 1);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = 4;
        tibi->byte_len = packet->adaption_length + 1;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        text = tr("%1 = Adaption Field Length: %2").arg(toBinaryStr(raw_buf + 4, (uint8_t)0xFF)).arg(packet->adaption_length);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = 4;
        tibi->byte_len = 1;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        text = tr("Adaption Data (%1 bytes)").arg(packet->adaption_length);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = 5;
        tibi->byte_len = packet->adaption_length;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);
    }

    if (packet->payload_flag) {
        int payload_len = TS_PACKET_LEN - (packet->adaption_flag ? (packet->adaption_length + 5) : 4);
        text = tr("Payload (%1 bytes)").arg(payload_len);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = 5 + packet->adaption_length;
        tibi->byte_len = payload_len;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);
    }
}

void ProtoTree::draw_pes_packet(pes_packet_info* packet, uint8_t* raw_buf) {
    QString text;
    tree_item_buf_info* tibi = NULL;
    QTreeWidgetItem* item;

    // Start code
    text = tr("packet_start_code_prefix (0x%1%2%3)")
            .arg((int)raw_buf[0], 2, 16, QChar('0'))
            .arg((int)raw_buf[1], 2, 16, QChar('0'))
            .arg((int)raw_buf[2], 2, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 0;
    tibi->byte_len = 3;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

    text = tr("stream_id 0x%1").arg((int)raw_buf[3], 2, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 3;
    tibi->byte_len = 1;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

    text = tr("PES_packet_length %1(0x%2)").arg(packet->len - 6).arg(packet->len - 6, 4, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 4;
    tibi->byte_len = 2;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

    text = tr("PES Header Extension 0x%1%2%3")
            .arg((int)raw_buf[6], 2, 16, QChar('0'))
            .arg((int)raw_buf[7], 2, 16, QChar('0'))
            .arg((int)raw_buf[8], 2, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 3;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

    text = tr("%1 = Fixed").arg(toBinaryStr(raw_buf + 6, (uint8_t)0xC0));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 0;
    tibi->bit_len = 2;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = PES_scrambling_control: 0x%2").arg(toBinaryStr(raw_buf + 6, (uint8_t)0x30))
            .arg((int)(raw_buf[6] >> 4) & 0x03, 1, 16, QChar('0'));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 2;
    tibi->bit_len = 2;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = PES_priority: %2").arg(toBinaryStr(raw_buf + 6, (uint8_t)0x08))
            .arg((raw_buf[6] & 0x08) ? '1' : '0');
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 4;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = data_alignment_indicator: %2").arg(toBinaryStr(raw_buf + 6, (uint8_t)0x04))
            .arg((raw_buf[6] & 0x04) ? '1' : '0');
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 5;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = copyright: %2").arg(toBinaryStr(raw_buf + 6, (uint8_t)0x02))
            .arg((raw_buf[6] & 0x02) ? '1' : '0');
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 6;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    text = tr("%1 = original_or_copy: %2").arg(toBinaryStr(raw_buf + 6, (uint8_t)0x01))
            .arg((raw_buf[6] & 0x01) ? '1' : '0');
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 6;
    tibi->byte_len = 0;
    tibi->start_bit = 7;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int pts_dts_flags = (int)(raw_buf[7] >> 6) & 0x03;
    text = tr("%1 = PTS_DTS_flags: %2, %3").arg(toBinaryStr(raw_buf + 7, (uint8_t)0xC0))
            .arg(pts_dts_flags, 2, 2, QChar('0'))
            .arg(pts_dts_flags == 0x0 ? "No PTS/DTS" : (pts_dts_flags == 0x2 ? "PTS Only" : "PTS and DTS"));
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 0;
    tibi->bit_len = 2;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int escr_flags = (int)((raw_buf[7] & 0x20) ? 1 : 0);
    text = tr("%1 = ESCR_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x20))
            .arg(escr_flags);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 2;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int es_rate_flags = (int)((raw_buf[7] & 0x10) ? 1 : 0);
    text = tr("%1 = ES_rate_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x10))
            .arg(es_rate_flags);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 3;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int dsm_trick_mode_flags = (int)((raw_buf[7] & 0x08) ? 1 : 0);
    text = tr("%1 = DSM_trick_mode_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x08))
            .arg(dsm_trick_mode_flags);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 4;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int add_copy_flags = (int)((raw_buf[7] & 0x04) ? 1 : 0);
    text = tr("%1 = additional_copy_info_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x04))
            .arg(add_copy_flags);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 5;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int pes_crc_flags = (int)((raw_buf[7] & 0x02) ? 1 : 0);
    text = tr("%1 = PES_CRC_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x02))
            .arg(pes_crc_flags);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 6;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int pes_ext_flag = (int)((raw_buf[7] & 0x01) ? 1 : 0);
    text = tr("%1 = PES_extension_flag: %2").arg(toBinaryStr(raw_buf + 7, (uint8_t)0x01))
            .arg(pes_ext_flag);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 7;
    tibi->byte_len = 0;
    tibi->start_bit = 7;
    tibi->bit_len = 1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int pes_header_data_len = (int)raw_buf[8];
    text = tr("%1 = PES_header_data_length: %2").arg(toBinaryStr(raw_buf + 8, (uint8_t)0xFF))
            .arg(raw_buf[8]);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = 8;
    tibi->byte_len = 1;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

    int cur_byte_index = 9;
    // rest of pes header data

    if (pts_dts_flags & 0x02) {
        // PTS
        double pts_value = (double)packet->pts;
        pts_value /= 90000.0;
        text = tr("PTS %1(%2)").arg(packet->pts).arg(pts_value, 0, 'f', 9, QChar('0'));
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 5;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // PTS Detail
        text = tr("%1 = PTS: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint64_t)0xEFFFEFFFEL)).arg(packet->pts);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 5;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 5;
    }

    if (pts_dts_flags & 0x01) {
        // DTS
        double dts_value = (double)packet->dts;
        dts_value /= 90000.0;
        text = tr("DTS %1(%2)").arg(packet->dts).arg(dts_value, 0, 'f', 9, QChar('0'));
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 5;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // DTS Detail
        text = tr("%1 = DTS: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint64_t)0xEFFFEFFFEL)).arg(packet->dts);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 5;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 5;
    }

    if (escr_flags) {
        // ESCR
        int64_t escr_base = 0;
        escr_base = ((int64_t)*(raw_buf + cur_byte_index) >> 3) & 0x7;
        escr_base <<= 30;
        escr_base |= ((int64_t)*(raw_buf + cur_byte_index) & 0x03) << 28;
        escr_base |= ((int64_t)*(raw_buf + cur_byte_index + 1) & 0xFF) << 20;
        escr_base |= (((int64_t)*(raw_buf + cur_byte_index + 2) >> 3) & 0x1F) << 15;
        escr_base |= ((int64_t)*(raw_buf + cur_byte_index + 2) & 0x03) << 13;
        escr_base |= ((int64_t)*(raw_buf + cur_byte_index + 3) & 0xFF) << 5;
        escr_base |= (((int64_t)*(raw_buf + cur_byte_index + 4) >> 3) & 0x1F);
        int16_t escr_ext = ((int16_t)*(raw_buf + cur_byte_index + 4) & 0x03) << 7;
        escr_ext |= ((int16_t)*(raw_buf + cur_byte_index + 5) & 0xFE) >> 1;
        text = tr("ESCR base:%1, extention: %2").arg(escr_base).arg(escr_ext);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 6;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // ESCR Detail
        text = tr("%1 = ESCR base: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint64_t)0x3BFFFBFFF8L)).arg(escr_base);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 5;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        text = tr("%1 = ESCR extention: %2").arg(toBinaryStr(raw_buf + cur_byte_index + 4, (uint16_t)0x3FE)).arg(escr_ext);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index + 4;
        tibi->byte_len = 2;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 6;
    }

    if (es_rate_flags) {
        // ES rate
        int32_t es_rate = 0;
        es_rate = ((int32_t)*(raw_buf + cur_byte_index) & 0x7F) << 15;
        es_rate |= ((int32_t)*(raw_buf + cur_byte_index + 1) & 0xFF) << 7;
        es_rate |= ((int32_t)*(raw_buf + cur_byte_index + 2) & 0xFE) >> 1;
        text = tr("ES_rate %1").arg(es_rate);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 3;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // ES rate Detail
        text = tr("%1 = ES_rate: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint64_t)0x7FFFFEL)).arg(es_rate);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 3;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 3;
    }

    if (dsm_trick_mode_flags) {
        // DSM_trick_mode
        int8_t trick_mode_control = ((int8_t)*(raw_buf + cur_byte_index) & 0xE0) >> 5;
        int8_t trick_mode_param = ((int8_t)*(raw_buf + cur_byte_index) & 0x1F);
        text = tr("DSM_trick_mode");
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 1;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // DSM_trick_mode Detail
        text = tr("%1 = trick_mode_control: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint8_t)0xE0)).arg(trick_mode_control);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 0;
        tibi->start_bit = 0;
        tibi->bit_len = 3;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        text = tr("%1 = trick_mode_param: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint8_t)0xE0)).arg(trick_mode_param, 2, 16, QChar('0'));
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 0;
        tibi->start_bit = 3;
        tibi->bit_len = 5;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 1;
    }

    if (add_copy_flags) {
        // additional_copy_info
        int8_t additional_copy_info = ((int8_t)*(raw_buf + cur_byte_index) & 0x7F);
        text = tr("additional_copy_info");
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 1;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        item = proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        // additional_copy_info Detail
        text = tr("%1 = additional_copy_info: %2").arg(toBinaryStr(raw_buf + cur_byte_index, (uint8_t)0x7F)).arg(additional_copy_info);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 0;
        tibi->start_bit = 1;
        tibi->bit_len = 7;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_DETAIL, item, tree_item_buf_infos.count() - 1);

        cur_byte_index += 1;
    }

    if (pes_crc_flags) {
        // previous_PES_packet_CRC
        uint16_t prev_pes_crc = ((uint16_t)*(raw_buf + cur_byte_index) << 8);
        prev_pes_crc |= ((uint16_t)*(raw_buf + cur_byte_index + 1));
        text = tr("previous_PES_packet_CRC (0x%1)").arg(prev_pes_crc, 4, 16, QChar('0'));
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = 2;
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        cur_byte_index += 2;
    }

    if (pes_ext_flag) {
        // PES Extention Data
        QString pes_ext_str;
        for (int i=cur_byte_index;i < pes_header_data_len + 9;i++) {
            pes_ext_str += tr("%1").arg(*(raw_buf + i), 2, 16, QChar('0'));
            if (pes_ext_str.length() >= 32) {
                pes_ext_str += "...";
                break;
            }
        }
        text = tr("PES_extention_data (0x%1)").arg(pes_ext_str);
        tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
        tibi->start_byte = cur_byte_index;
        tibi->byte_len = pes_header_data_len - (cur_byte_index - 9);
        tibi->start_bit = -1;
        tibi->bit_len = -1;

        tree_item_buf_infos.append(tibi);

        proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);

        cur_byte_index = pes_header_data_len + 9;
    }

    // PES Payload data
    text = tr("PES Payload Data (%1 bytes)").arg(packet->len - cur_byte_index);
    tibi = (tree_item_buf_info*)malloc(sizeof(tree_item_buf_info));
    tibi->start_byte = cur_byte_index;
    tibi->byte_len = packet->len - cur_byte_index;
    tibi->start_bit = -1;
    tibi->bit_len = -1;

    tree_item_buf_infos.append(tibi);

    proto_tree_draw_node(text, TREE_NODE_TYPE_FIELD, invisibleRootItem(), tree_item_buf_infos.count() - 1);
}

void ProtoTree::draw_nal_packet(nal_t* packet, uint8_t* raw_buf) {

}

ProtoTree::ProtoTree(int type, QWidget *parent) :
    QTreeWidget(parent),
    column_resize_timer_(0)
{
    this->type = type;
    setAccessibleName(tr("Packet details"));
    // Leave the uniformRowHeights property as-is (false) since items might
    // have multiple lines (e.g. packet comments). If this slows things down
    // too much we should add a custom delegate which handles SizeHintRole
    // similar to PacketListModel::data.
    setHeaderHidden(true);

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(updateSelectionStatus(QTreeWidgetItem*)));
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(expand(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(collapse(QModelIndex)));
    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
            this, SLOT(itemDoubleClick(QTreeWidgetItem*, int)));

    // resizeColumnToContents checks 1000 items by default. The user might
    // have scrolled to an area with a different width at this point.
    connect(verticalScrollBar(), SIGNAL(sliderReleased()),
            this, SLOT(updateContentWidth()));
}

ProtoTree::~ProtoTree() {
    foreach (tree_item_buf_info* ptr, tree_item_buf_infos) {
        free(ptr);
    }
}

void ProtoTree::clear() {
    QTreeWidget::clear();
    updateContentWidth();

    foreach (tree_item_buf_info* ptr, tree_item_buf_infos) {
        free(ptr);
    }

    tree_item_buf_infos.clear();
}

void ProtoTree::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == column_resize_timer_) {
        killTimer(column_resize_timer_);
        column_resize_timer_ = 0;
        resizeColumnToContents(0);
    } else {
        QTreeWidget::timerEvent(event);
    }
}

// resizeColumnToContents checks 1000 items by default. The user might
// have scrolled to an area with a different width at this point.
void ProtoTree::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    switch(event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
            updateContentWidth();
            break;
        default:
            break;
    }
}

void ProtoTree::updateContentWidth()
{
    if (column_resize_timer_ == 0) {
        column_resize_timer_ = startTimer(0);
    }
}

void ProtoTree::fillProtocolTree(void *packet_data, uint8_t* raw_buf) {
    clear();

    // proto_tree_children_foreach(packet_data, proto_tree_draw_node, invisibleRootItem());
    switch (type) {
    case PROTO_TREE_TYPE_FFMPEG_PACKET:
        draw_ffmpeg_packet((AVPacket*)packet_data, raw_buf);
        break;
    case PROTO_TREE_TYPE_TS:
        draw_ts_packet((ts_packet_info*)packet_data, raw_buf);
        break;
    case PROTO_TREE_TYPE_PES:
        draw_pes_packet((pes_packet_info*)packet_data, raw_buf);
        break;
    case PROTO_TREE_TYPE_NAL:
        draw_nal_packet((nal_t*)packet_data, raw_buf);
        break;
    default:
        return;
    }

    updateContentWidth();
}

// XXX We select the first match, which might not be the desired item.
void ProtoTree::goToField(int hf_id)
{
#if 0
    if (hf_id < 0) return;

    QTreeWidgetItemIterator iter(this);
    while (*iter) {
        field_info *fi = VariantPointer<field_info>::asPtr((*iter)->data(0, Qt::UserRole));

        if (fi && fi->hfinfo) {
            if (fi->hfinfo->id == hf_id) {
                setCurrentItem(*iter);
                break;
            }
        }
        ++iter;
    }
#endif
}

void ProtoTree::updateSelectionStatus(QTreeWidgetItem* item)
{
    if (item) {
        int idx_buf_info = item->data(0, Qt::UserRole).toInt();
        tree_item_buf_info* tibi = tree_item_buf_infos[idx_buf_info];
        emit protoItemSelected(tibi->start_byte, tibi->byte_len, tibi->start_bit, tibi->bit_len);
    }
#if 0
    if (item) {
        field_info *fi;
        QString item_info;

        fi = VariantPointer<field_info>::asPtr(item->data(0, Qt::UserRole));
        if (!fi || !fi->hfinfo) return;

        if (fi->hfinfo->blurb != NULL && fi->hfinfo->blurb[0] != '\0') {
            item_info.append(QString().fromUtf8(fi->hfinfo->blurb));
        } else {
            item_info.append(QString().fromUtf8(fi->hfinfo->name));
        }

        if (!item_info.isEmpty()) {
            int finfo_length;
            item_info.append(" (" + QString().fromUtf8(fi->hfinfo->abbrev) + ")");

            finfo_length = fi->length + fi->appendix_length;
            if (finfo_length == 1) {
                item_info.append(tr(", 1 byte"));
            } else if (finfo_length > 1) {
                item_info.append(QString(tr(", %1 bytes")).arg(finfo_length));
            }

            saveSelectedField(item);

            emit protoItemSelected("");
            emit protoItemSelected(NULL);
            emit protoItemSelected(item_info);
            emit protoItemSelected(fi);
        } // else the GTK+ version pushes an empty string as described below.
        /*
         * Don't show anything if the field name is zero-length;
         * the pseudo-field for text-only items is such
         * a field, and we don't want "Text (text)" showing up
         * on the status line if you've selected such a field.
         *
         * XXX - there are zero-length fields for which we *do*
         * want to show the field name.
         *
         * XXX - perhaps the name and abbrev field should be null
         * pointers rather than null strings for that pseudo-field,
         * but we'd have to add checks for null pointers in some
         * places if we did that.
         *
         * Or perhaps text-only items should have -1 as the field
         * index, with no pseudo-field being used, but that might
         * also require special checks for -1 to be added.
         */

    } else {
        emit protoItemSelected("");
        emit protoItemSelected(NULL);
    }
#endif
}

void ProtoTree::expand(const QModelIndex & index) {
#if 0
    field_info *fi;

    fi = VariantPointer<field_info>::asPtr(index.data(Qt::UserRole));
    if (!fi) return;

    if(prefs.gui_auto_scroll_on_expand) {
        ScrollHint scroll_hint = PositionAtTop;
        if (prefs.gui_auto_scroll_percentage > 66) {
            scroll_hint = PositionAtBottom;
        } else if (prefs.gui_auto_scroll_percentage >= 33) {
            scroll_hint = PositionAtCenter;
        }
        scrollTo(index, scroll_hint);
    }

    /*
     * Nodes with "finfo->tree_type" of -1 have no ett_ value, and
     * are thus presumably leaf nodes and cannot be expanded.
     */
    if (fi->tree_type != -1) {
        g_assert(fi->tree_type >= 0 &&
                 fi->tree_type < num_tree_types);
        tree_expanded_set(fi->tree_type, TRUE);
    }

    updateContentWidth();
#endif
}

void ProtoTree::collapse(const QModelIndex & index) {
#if 0
    field_info *fi;

    fi = VariantPointer<field_info>::asPtr(index.data(Qt::UserRole));
    if (!fi) return;

    /*
     * Nodes with "finfo->tree_type" of -1 have no ett_ value, and
     * are thus presumably leaf nodes and cannot be collapsed.
     */
    if (fi->tree_type != -1) {
        g_assert(fi->tree_type >= 0 &&
                 fi->tree_type < num_tree_types);
        tree_expanded_set(fi->tree_type, FALSE);
    }
    updateContentWidth();
#endif
}

void ProtoTree::expandSubtrees()
{
    QTreeWidgetItem *top_sel;

    if (selectedItems().length() < 1) {
        return;
    }

    top_sel = selectedItems()[0];

    if (!top_sel) {
        return;
    }

    while (top_sel->parent()) {
        top_sel = top_sel->parent();
    }

    QTreeWidgetItemIterator iter(top_sel);
    while (*iter) {
        if ((*iter) != top_sel && (*iter)->parent() == NULL) {
            // We found the next top-level item
            break;
        }
        (*iter)->setExpanded(true);
        ++iter;
    }
    updateContentWidth();
}

void ProtoTree::expandAll()
{
#if 0
    int i;
    for(i=0; i < num_tree_types; i++) {
        tree_expanded_set(i, TRUE);
    }
    QTreeWidget::expandAll();
    updateContentWidth();
#endif
}

void ProtoTree::collapseAll()
{
#if 0
    int i;
    for(i=0; i < num_tree_types; i++) {
        tree_expanded_set(i, FALSE);
    }
    QTreeWidget::collapseAll();
    updateContentWidth();
#endif
}

void ProtoTree::itemDoubleClick(QTreeWidgetItem *item, int) {
#if 0
    field_info *fi;

    fi = VariantPointer<field_info>::asPtr(item->data(0, Qt::UserRole));
    if (!fi || !fi->hfinfo) return;

    if (fi->hfinfo->type == FT_FRAMENUM) {
        if (QApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
            emit openPacketInNewWindow(true);
        } else {
            emit goToPacket(fi->value.value.uinteger);
        }
    } else if (FI_GET_FLAG(fi, FI_URL) && IS_FT_STRING(fi->hfinfo->type)) {
        gchar *url;
        url = fvalue_to_string_repr(NULL, &fi->value, FTREPR_DISPLAY, fi->hfinfo->display);
        if(url){
//            browser_open_url(url);
            QDesktopServices::openUrl(QUrl(url));
            wmem_free(NULL, url);
        }
    }
#endif
}

void ProtoTree::selectField(int byte_index)
{
#if 0
    QTreeWidgetItemIterator iter(this);
    while (*iter) {
        if (byte_index == VariantPointer<field_info>::asPtr((*iter)->data(0, Qt::UserRole))) {
            setCurrentItem(*iter);
            scrollToItem(*iter);
            break;
        }
        ++iter;
    }
#endif
}

#if 0
// Finds position item at a level, counting only similar fields.
static unsigned indexOfField(QTreeWidgetItem *item, header_field_info *hfi)
{
    QTreeWidgetItem *parent = item->parent();
    unsigned pos = 0;
    if (!parent) {
        // In case multiple top-level layers are present for the same protocol,
        // try to find its position (this will likely be the first match, zero).
        QTreeWidget *tree = item->treeWidget();
        for (int i = 0; i < tree->topLevelItemCount(); i++) {
            QTreeWidgetItem *current = tree->topLevelItem(i);
            if (current == item) {
                return pos;
            }
            if (hfi == VariantPointer<field_info>::asPtr(current->data(0, Qt::UserRole))->hfinfo) {
                pos++;
            }
        }
    } else {
        QTreeWidgetItemIterator iter(parent);
        while (*iter) {
            QTreeWidgetItem *current = *iter;
            if (current == item) {
                return pos;
            }
            if (hfi == VariantPointer<field_info>::asPtr(current->data(0, Qt::UserRole))->hfinfo) {
                pos++;
            }
            ++iter;
        }
    }
    // should not happen (child is not found at parent?!)
    return 0;
}
#endif
#if 0
// Assume about 2^8 items in tree and 2^24 different registered fields.
// If there are more of each, then a collision may occur, but since the full
// path is matched this is unlikely to be a problem.
#define POS_SHIFT   24
#define POS_MASK    (((unsigned)-1) << POS_SHIFT)

// Remember the currently focussed field based on:
// - current hf_id (obviously)
// - parent items (to avoid selecting a text item in a different tree)
// - position within a tree if there are multiple items
static QList<int> serializeAsPath(QTreeWidgetItem *item)
{
    QList<int> path;
    do {
        field_info *fi = VariantPointer<field_info>::asPtr(item->data(0, Qt::UserRole));
        unsigned pos = indexOfField(item, fi->hfinfo);
        path.prepend((pos << POS_SHIFT) | (fi->hfinfo->id & ~POS_MASK));
    } while ((item = item->parent()));
    return path;
}
#endif
void ProtoTree::saveSelectedField(QTreeWidgetItem *item)
{
    // selected_field_path_ = serializeAsPath(item);
}

// Try to focus a tree item which was previously also visible
void ProtoTree::restoreSelectedField()
{
#if 0
    if (selected_field_path_.isEmpty()) {
        return;
    }
    int last_hf_id = selected_field_path_.last() & ~POS_MASK;
    QTreeWidgetItemIterator iter(this);
    while (*iter) {
        field_info *fi = VariantPointer<field_info>::asPtr((*iter)->data(0, Qt::UserRole));
        if (last_hf_id == fi->hfinfo->id &&
            serializeAsPath(*iter) == selected_field_path_) {
            // focus the first item, but do not expand collapsed trees.
            QTreeWidgetItem *item = *iter, *target = item;
            do {
                if (!item->isExpanded()) {
                    target = item;
                }
            } while ((item = item->parent()));
            setCurrentItem(target);
            scrollToItem(target);
            break;
        }
        ++iter;
    }
#endif
}

QString ProtoTree::toBinaryStr(uint8_t* bytes, uint8_t mask) {
    QString str;
    for (int i=7;i>=0;i--) {
        str.append(mask & (1 << i) ? (bytes[0] & (1 << i) ? '1' : '0') : '.');
        if (i == 4) {
            str.append(' ');
        }
    }

    return str;
}

QString ProtoTree::toBinaryStr(uint8_t *bytes, uint16_t mask) {
    QString str;
    str = toBinaryStr(bytes, (uint8_t)((mask >> 8) & 0xFF));
    str += ' ';
    str += toBinaryStr(bytes + 1, (uint8_t)(mask & 0xFF));
    return str;
}

QString ProtoTree::toBinaryStr(uint8_t *bytes, uint32_t mask) {
    QString str;
    for (int i=0;i<4;i++) {
        str += toBinaryStr(bytes + i, (uint8_t)((mask >> (32 - 8 * (i + 1))) & 0xFF));
        if (i < 3) {
            str += ' ';
        }
    }
    return str;
}

QString ProtoTree::toBinaryStr(uint8_t *bytes, uint64_t mask) {
    QString str;
    int byte_index = 0;
    for (int i=0;i<8;i++) {
        uint8_t byte_mask = (uint8_t)((mask >> (64 - 8 * (i + 1))) & 0xFF);
        if (byte_mask == 0) continue;
        str += toBinaryStr(bytes + byte_index, byte_mask);
        if (i < 7) {
            str += ' ';
        }

        byte_index ++;
    }
    return str;
}
