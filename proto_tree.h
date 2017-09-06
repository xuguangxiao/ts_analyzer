/* This file is from wireshark, just keep the render codes.
 * All wireshark's main skeletons are removed
 */

/* proto_tree.h
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

#ifndef PROTO_TREE_H
#define PROTO_TREE_H

#include <QVariant>
#include <QTreeWidget>
#include <QMenu>
#include "ts_parser.h"

typedef enum {
    PROTO_TREE_TYPE_FFMPEG_PACKET,
    PROTO_TREE_TYPE_TS,
    PROTO_TREE_TYPE_PES,
    PROTO_TREE_TYPE_NAL,
}tree_type;

enum {
    TREE_NODE_TYPE_FIELD,
    TREE_NODE_TYPE_DETAIL,
};

typedef struct tree_item_buf_info {
    int start_byte;
    int byte_len;
    int start_bit;
    int bit_len;
}tree_item_buf_info;

class ProtoTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ProtoTree(int type, QWidget *parent = 0);
    ~ProtoTree();
    void fillProtocolTree(void* packet_data, uint8_t *raw_buf = NULL);
    void goToField(int hf_id);
    void selectField(int byte_index);
    void clear();
    void saveSelectedField(QTreeWidgetItem *);
    void restoreSelectedField();

    int type;
    QTreeWidgetItem* proto_tree_draw_node(const QString& text, int type, QTreeWidgetItem* parent, int idx_buf_info);
    void draw_ffmpeg_packet(AVPacket* packet, uint8_t* raw_buf);
    void draw_ts_packet(ts_packet_info* packet, uint8_t* raw_buf);
    void draw_pes_packet(pes_packet_info* packet, uint8_t* raw_buf);
    void draw_nal_packet(nal_t* packet, uint8_t* raw_buf);
    QList<tree_item_buf_info*> tree_item_buf_infos;

    QString toBinaryStr(uint8_t* bytes, uint8_t mask);
    QString toBinaryStr(uint8_t* bytes, uint16_t mask);
    QString toBinaryStr(uint8_t* bytes, uint32_t mask);
    QString toBinaryStr(uint8_t* bytes, uint64_t mask);

protected:
    virtual void timerEvent(QTimerEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

private:
    int column_resize_timer_;
    QList<int> selected_field_path_;

signals:
    void protoItemSelected(int byte_start, int byte_len, int bit_start, int bit_len);
    void openPacketInNewWindow(bool);
    void goToPacket(int);

public slots:
    void updateSelectionStatus(QTreeWidgetItem*);
    void expand(const QModelIndex & index);
    void collapse(const QModelIndex & index);
    void expandSubtrees();
    void expandAll();
    void collapseAll();
    void itemDoubleClick(QTreeWidgetItem *item, int column);

private slots:
    void updateContentWidth();
};

#endif // PROTO_TREE_H
