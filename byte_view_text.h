/* This file is from wireshark, just keep the render codes.
 * Other mouse trackings (click and hover) are removed
 * All wireshark's main skeletons are removed
 */

/* byte_view_text.h
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

#ifndef BYTE_VIEW_TEXT_H
#define BYTE_VIEW_TEXT_H

#include "proto_tree.h"

#include <QAbstractScrollArea>
#include <QMenu>

class QActionGroup;

// XXX - Is there any reason we shouldn't add ByteViewImage, etc?

class ByteViewText : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit ByteViewText(QWidget *parent = 0, uint8_t *raw_buf = NULL, uint buf_len = 0, QTreeWidget *protoTree = NULL);
    ~ByteViewText();
    virtual QSize minimumSizeHint() const;

    void set_buf(uint8_t* raw_buf, uint buf_len);

    void setHighlightStyle(bool bold) { bold_highlight_ = bold; }
    void setProtocolHighlight(int start, int end);
    void setFieldHighlight(int start, int end, uint32_t mask = 0, int mask_le = 0);
    void setFieldAppendixHighlight(int start, int end);
    bool isEmpty() { return raw_buf == NULL;}
    const uint8_t *dataAndLength(uint *data_len_ptr);

    static QRgb alphaBlend(const QColor &color1, const QColor &color2, qreal alpha);
    static QRgb alphaBlend(const QBrush &brush1, const QBrush &brush2, qreal alpha);

    void clear();
signals:
    void byteFieldHovered(const QString &);

public slots:
    void setMonospaceFont(const QFont &mono_font);
    void protoItemSelected(int start_byte, int byte_len, int start_bit, int bit_len);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent (QMouseEvent * event);
    virtual void mouseMoveEvent (QMouseEvent * event);
    virtual void leaveEvent(QEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    // Text highlight modes.
    typedef enum {
        ModeNormal,
        ModeField,
        ModeProtocol,
        ModeOffsetNormal,
        ModeOffsetField,
        ModeHover
    } HighlightMode;

    void drawOffsetLine(QPainter &painter, const uint32_t offset, const int row_y);
    qreal flushOffsetFragment(QPainter &painter, qreal x, int y, HighlightMode mode, QString &text);
    void scrollToByte(int byte);
    int offsetChars();
    int offsetPixels();
    int hexPixels();
    int asciiPixels();
    int totalPixels();
    void updateScrollbars();
    int byteOffsetAtPixel(QPoint &pos);

    static const int separator_interval_;
    uint8_t* raw_buf;
    uint buf_len;
    QTreeWidget *tree_widget_;

    // Fonts and colors
    QFont mono_font_;
//    QFont mono_bold_font_;
    QBrush offset_normal_fg_;
    QBrush offset_field_fg_;

    bool bold_highlight_;

    // Data highlight
    uint hovered_byte_offset_;
    bool hovered_byte_lock_;
    QPair<uint,uint> p_bound_;
    QPair<uint,uint> f_bound_;
    QPair<uint,uint> fa_bound_;
    QPair<uint,uint> p_bound_save_;
    QPair<uint,uint> f_bound_save_;
    QPair<uint,uint> fa_bound_save_;

    bool show_offset_;          // Should we show the byte offset?
    bool show_hex_;             // Should we show the hex display?
    bool show_ascii_;           // Should we show the ASCII display?
    uint row_width_;           // Number of bytes per line
    int one_em_;                // Font character height
    qreal font_width_;          // Font character width
    int line_spacing_;          // Font line spacing
    int margin_;                // Text margin

    // Data selection
    QMap<int,int> x_pos_to_column_;

};

#endif // BYTE_VIEW_TEXT_H

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
