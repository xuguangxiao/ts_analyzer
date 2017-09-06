/* This file is from wireshark, just keep the render codes.
 * Other mouse trackings (click and hover) are removed
 * All wireshark's main skeletons are removed
 */

/* byte_view_text.cpp
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

// Some code based on QHexView by Even Teran
// https://code.google.com/p/qhexview/

#include "byte_view_text.h"

#include <QActionGroup>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

// To do:
// - Add recent settings and context menu items to show/hide the offset,
//   and ASCII/EBCDIC.
// - Add a UTF-8 and possibly UTF-xx option to the ASCII display.
// - Add "copy bytes as" context menu items.

// We don't obey the gui.hex_dump_highlight_style preference. If you
// would like to add support for this you'll probably have to call
// QPainter::drawText for each individual character.

ByteViewText::ByteViewText(QWidget *parent, uint8_t *raw_buf, uint buf_len, QTreeWidget *tree_widget) :
    QAbstractScrollArea(parent),
    raw_buf(raw_buf),
    buf_len(buf_len),
    tree_widget_(tree_widget),
    bold_highlight_(false),
    hovered_byte_offset_(-1),
    hovered_byte_lock_(false),
    p_bound_(0, 0),
    f_bound_(0, 0),
    fa_bound_(0, 0),
    show_offset_(true),
    show_hex_(true),
    show_ascii_(true),
    row_width_(32),
    one_em_(0),
    font_width_(0),
    line_spacing_(0),
    margin_(0)
{
    setMouseTracking(false);

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, true);
#endif
}

ByteViewText::~ByteViewText()
{
}

QSize ByteViewText::minimumSizeHint() const
{
    // Allow panel to be shrinked to any size
    return QSize();
}

void ByteViewText::setProtocolHighlight(int start, int end)
{
    p_bound_ = QPair<uint, uint>(qMax(0, start), qMax(0, end));
    p_bound_save_ = p_bound_;
    viewport()->update();
}

void ByteViewText::setFieldHighlight(int start, int end, uint32_t, int)
{
    f_bound_ = QPair<uint, uint>(qMax(0, start), qMax(0, end));
    f_bound_save_ = f_bound_;
    scrollToByte(start);
    viewport()->update();
}

void ByteViewText::setFieldAppendixHighlight(int start, int end)
{
    fa_bound_ = QPair<uint, uint>(qMax(0, start), qMax(0, end));
    fa_bound_save_ = f_bound_;
    viewport()->update();
}

const uint8_t *ByteViewText::dataAndLength(uint *data_len_ptr)
{
    if (!raw_buf) return NULL;

    if (buf_len) {
        *data_len_ptr = buf_len;
        return raw_buf;
    }
    return NULL;
}

void ByteViewText::setMonospaceFont(const QFont &mono_font)
{
    mono_font_ = mono_font;

    const QFontMetricsF fm(mono_font);
    font_width_  = fm.width('M');
    line_spacing_ = fm.lineSpacing() + 0.5;
    one_em_ = fm.height();
    margin_ = fm.height() / 2;

    setFont(mono_font);

    updateScrollbars();
    viewport()->update();
}

void ByteViewText::paintEvent(QPaintEvent *)
{
    QPainter painter(viewport());
    painter.translate(-horizontalScrollBar()->value() * font_width_, 0);
    painter.setFont(font());

    // Pixel offset of this row
    int row_y = 0;

    // Starting byte offset
    uint offset = (uint) verticalScrollBar()->value() * row_width_;

    // Clear the area
    painter.fillRect(viewport()->rect(), palette().base());

    // Offset background
    offset_normal_fg_.setColor(alphaBlend(palette().windowText(), palette().window(), 0.35));
    offset_field_fg_.setColor(alphaBlend(palette().windowText(), palette().window(), 0.65));
    if (show_offset_) {
        QRect offset_rect = QRect(viewport()->rect());
        offset_rect.setWidth(offsetPixels());
        painter.fillRect(offset_rect, palette().window());
    }

    if (!raw_buf) {
        return;
    }

    // Map window coordinates to byte offsets
    x_pos_to_column_.clear();
    for (uint i = 0; i < row_width_; i++) {
        int sep_width = (i / separator_interval_) * font_width_;
        if (show_hex_) {
            // Hittable pixels extend 1/2 space on either side of the hex digits
            int pixels_per_byte = 3 * font_width_;
            int hex_x = offsetPixels() + margin_ + sep_width + (i * pixels_per_byte) - (font_width_ / 2);
            for (int j = 0; j <= pixels_per_byte; j++) {
                x_pos_to_column_[hex_x + j] = i;
            }
        }
        if (show_ascii_) {
            int ascii_x = offsetPixels() + hexPixels() + margin_ + sep_width + (i * font_width_);
            for (int j = 0; j <= font_width_; j++) {
                x_pos_to_column_[ascii_x + j] = i;
            }
        }
    }

    // Data rows
    int widget_height = height();
    painter.save();
    while(row_y + line_spacing_ < widget_height && offset < buf_len) {
        drawOffsetLine(painter, offset, row_y);
        offset += row_width_;
        row_y += line_spacing_;
    }
    painter.restore();

    QStyleOptionFocusRect option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
}

void ByteViewText::resizeEvent(QResizeEvent *)
{
    updateScrollbars();
}

void ByteViewText::mousePressEvent (QMouseEvent *event) {
    QAbstractScrollArea::mousePressEvent(event);
    return;

#if 0
    if (!raw_buf || !event || event->button() != Qt::LeftButton) {
        return;
    }

    hovered_byte_lock_ = !hovered_byte_lock_;
    QPoint pos = event->pos();
    field_info *fi = fieldAtPixel(pos);

    if (fi && tree_widget_) {
        // XXX - This should probably be a ProtoTree method.
        QTreeWidgetItemIterator iter(tree_widget_);
        while (*iter) {
            if (fi == VariantPointer<field_info>::asPtr((*iter)->data(0, Qt::UserRole))) {
                tree_widget_->setCurrentItem((*iter));
                tree_widget_->scrollToItem((*iter));
            }

            ++iter;
        }
    }
#endif
}

void ByteViewText::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseMoveEvent(event);
    return;
#if 0
    if (hovered_byte_lock_) {
        return;
    }

    QString field_str;
    // XXX can the event really be NULL?
    if (!event) {
        emit byteFieldHovered(field_str);
        p_bound_ = p_bound_save_;
        f_bound_ = f_bound_save_;
        fa_bound_ = fa_bound_save_;
        viewport()->update();
        return;
    }
    QPoint pos = event->pos();
    hovered_byte_offset_ = byteOffsetAtPixel(pos);
    field_info *fi = fieldAtPixel(pos);
    if (fi) {
        if (fi->length < 2) {
            field_str = QString(tr("Byte %1"))
                    .arg(fi->start);
        } else {
            field_str = QString(tr("Bytes %1-%2"))
                    .arg(fi->start)
                    .arg(fi->start + fi->length - 1);
        }
        field_str += QString(": %1 (%2)")
                .arg(fi->hfinfo->name)
                .arg(fi->hfinfo->abbrev);
        f_bound_ = QPair<uint, uint>(fi->start, fi->start + fi->length);
        p_bound_ = QPair<uint, uint>(0, 0);
        fa_bound_ = QPair<uint, uint>(0, 0);
    } else {
        p_bound_ = p_bound_save_;
        f_bound_ = f_bound_save_;
        fa_bound_ = fa_bound_save_;
    }
    emit byteFieldHovered(field_str);
    viewport()->update();
#endif
}

void ByteViewText::leaveEvent(QEvent *event)
{
    QAbstractScrollArea::leaveEvent(event);
    return;
#if 0
    QString empty;
    emit byteFieldHovered(empty);
    if (!hovered_byte_lock_) {
        hovered_byte_offset_ = -1;
    }
    p_bound_ = p_bound_save_;
    f_bound_ = f_bound_save_;
    fa_bound_ = fa_bound_save_;
    viewport()->update();
    QAbstractScrollArea::leaveEvent(event);
#endif
}

void ByteViewText::contextMenuEvent(QContextMenuEvent *event)
{
    QAbstractScrollArea::contextMenuEvent(event);
    return;
    // ctx_menu_.exec(event->globalPos());
}

// Private

const int ByteViewText::separator_interval_ = 8; // Insert a space after this many bytes

// Draw a line of byte view text for a given offset.
// Text with different styles are split into fragments and passed to
// flushOffsetFragment. Font character widths aren't necessarily whole
// numbers so we track our X coordinate position using using floats.
void ByteViewText::drawOffsetLine(QPainter &painter, const uint32_t offset, const int row_y)
{
    if (!raw_buf) {
        return;
    }
    uint max_pos = qMin(offset + row_width_, buf_len);
    const uint8_t *pd = raw_buf;

    static const uchar hexchars[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    QString text;
    HighlightMode hl_mode = ModeNormal, offset_mode = ModeOffsetNormal;
    qreal hex_x = offsetPixels() + margin_;
    qreal ascii_x = offsetPixels() + hexPixels() + margin_;

    // Hex
    if (show_hex_) {
        for (uint tvb_pos = offset; tvb_pos < max_pos; tvb_pos++) {
            HighlightMode hex_state = ModeNormal;
            bool add_space = tvb_pos != offset;
            bool draw_hover = tvb_pos == hovered_byte_offset_;

            if ((tvb_pos >= f_bound_.first && tvb_pos < f_bound_.second) || (tvb_pos >= fa_bound_.first && tvb_pos < fa_bound_.second)) {
                hex_state = ModeField;
                offset_mode = ModeOffsetField;
            } else if (tvb_pos >= p_bound_.first && tvb_pos < p_bound_.second) {
                hex_state = ModeProtocol;
            }

            if (hex_state != hl_mode || draw_hover) {
                if ((hl_mode == ModeNormal || (hl_mode == ModeProtocol && hex_state == ModeField) || draw_hover) && add_space) {
                    add_space = false;
                    text += ' ';
                    /* insert a space every separator_interval_ bytes */
                    if ((tvb_pos % separator_interval_) == 0)
                        text += ' ';
                }
                hex_x += flushOffsetFragment(painter, hex_x, row_y, hl_mode, text);
                hl_mode = hex_state;
            }

            if (add_space) {
                text += ' ';
                /* insert a space every separator_interval_ bytes */
                if ((tvb_pos % separator_interval_) == 0)
                    text += ' ';
            }

            text += hexchars[(pd[tvb_pos] & 0xf0) >> 4];
            text += hexchars[pd[tvb_pos] & 0x0f];
            if (draw_hover) {
                hex_x += flushOffsetFragment(painter, hex_x, row_y, ModeHover, text);
            }
        }
    }
    if (text.length() > 0) {
        flushOffsetFragment(painter, hex_x, row_y, hl_mode, text);
    }
    hl_mode = ModeNormal;

    // ASCII
    if (show_ascii_) {
        for (uint tvb_pos = offset; tvb_pos < max_pos; tvb_pos++) {
            HighlightMode ascii_state = ModeNormal;
            bool add_space = tvb_pos != offset;
            bool highlight_text = tvb_pos == hovered_byte_offset_;

            if ((tvb_pos >= f_bound_.first && tvb_pos < f_bound_.second) || (tvb_pos >= fa_bound_.first && tvb_pos < fa_bound_.second)) {
                ascii_state = ModeField;
                offset_mode = ModeOffsetField;
            } else if (tvb_pos >= p_bound_.first && tvb_pos < p_bound_.second) {
                ascii_state = ModeProtocol;
            }

            if (ascii_state != hl_mode || highlight_text) {
                if ((hl_mode == ModeNormal || (hl_mode == ModeProtocol && ascii_state == ModeField) || highlight_text) && add_space) {
                    add_space = false;
                    /* insert a space every separator_interval_ bytes */
                    if ((tvb_pos % separator_interval_) == 0)
                        text += ' ';
                }
                ascii_x += flushOffsetFragment(painter, ascii_x, row_y, hl_mode, text);
                hl_mode = ascii_state;
            }

            if (add_space) {
                /* insert a space every separator_interval_ bytes */
                if ((tvb_pos % separator_interval_) == 0)
                    text += ' ';
            }

            uchar c = pd[tvb_pos];

            text += isprint(c) ? c : '.';
            if (highlight_text) {
                ascii_x += flushOffsetFragment(painter, ascii_x, row_y, ModeHover, text);
            }
        }
    }
    if (text.length() > 0) {
        flushOffsetFragment(painter, ascii_x, row_y, hl_mode, text);
    }

    // Offset. Must be drawn last in order for offset_state to be set.
    if (show_offset_) {
        text = QString("%1").arg(offset, offsetChars(), 16, QChar('0'));
        flushOffsetFragment(painter, margin_, row_y, offset_mode, text);
    }
}

// Draws a fragment of byte view text at the specifiec location using colors
// for the specified state. Clears the text and returns the pixel width of the
// drawn text.
qreal ByteViewText::flushOffsetFragment(QPainter &painter, qreal x, int y, HighlightMode mode, QString &text)
{
    if (text.length() < 1) {
        return 0;
    }
    QFontMetricsF fm(mono_font_);
    qreal width = fm.width(text);
    QRectF area(x, y, width, line_spacing_);
    // Background
    switch (mode) {
    case ModeField:
        painter.fillRect(area, palette().highlight());
        break;
    case ModeProtocol:
        painter.fillRect(area, palette().window());
        break;
    case ModeHover:
        // painter.fillRect(area, ColorUtils::byteViewHoverColor(true));
        break;
    default:
        break;
    }

    // Text
    QBrush text_brush;
    switch (mode) {
    case ModeNormal:
    case ModeProtocol:
    default:
        text_brush = palette().windowText();
        break;
    case ModeField:
        text_brush = palette().highlightedText();
        break;
    case ModeOffsetNormal:
        text_brush = offset_normal_fg_;
        break;
    case ModeOffsetField:
        text_brush = offset_field_fg_;
        break;
    case ModeHover:
        // text_brush = ColorUtils::byteViewHoverColor(false);
        break;
    }

    painter.setPen(QPen(text_brush.color()));
    painter.drawText(area, Qt::AlignTop, text);
    text.clear();
    return width;
}

void ByteViewText::scrollToByte(int byte)
{
    verticalScrollBar()->setValue(byte / row_width_);
}

// Offset character width
int ByteViewText::offsetChars()
{
    if (raw_buf && buf_len > 0xffff) {
        return 8;
    }
    return 4;
}

// Offset pixel width
int ByteViewText::offsetPixels()
{
    if (show_offset_) {
        return offsetChars() * font_width_ + one_em_;
    }
    return 0;
}

// Hex pixel width
int ByteViewText::hexPixels()
{
    if (show_hex_) {
        int digits_per_byte = 3;
        return (((row_width_ * digits_per_byte) + ((row_width_ - 1) / separator_interval_)) * font_width_) + one_em_;
    }
    return 0;
}

int ByteViewText::asciiPixels()
{
    if (show_ascii_) {
        return ((row_width_ + ((row_width_ - 1) / separator_interval_)) * font_width_) + one_em_;
    }
    return 0;
}

int ByteViewText::totalPixels()
{
    return offsetPixels() + hexPixels() + asciiPixels();
}

void ByteViewText::updateScrollbars()
{
    const int length = raw_buf ? buf_len : 0;
    if (raw_buf) {
    }

    qint64 maxval = length / row_width_ + ((length % row_width_) ? 1 : 0) - viewport()->height() / line_spacing_;

    verticalScrollBar()->setRange(0, int(qMax((qint64)0, maxval)));
    horizontalScrollBar()->setRange(0, qMax(0, static_cast<int>((totalPixels() - viewport()->width()) / font_width_)));
}

int ByteViewText::byteOffsetAtPixel(QPoint &pos)
{
    int byte = (verticalScrollBar()->value() + (pos.y() / line_spacing_)) * row_width_;
    int x = (horizontalScrollBar()->value() * font_width_) + pos.x();
    int col = x_pos_to_column_.value(x, -1);

    if (col < 0) {
        return -1;
    }

    byte += col;
    if ((uint) byte > buf_len) {
        return -1;
    }
    return byte;
}

QRgb ByteViewText::alphaBlend(const QColor &color1, const QColor &color2, qreal alpha)
{
    alpha = qBound(qreal(0.0), alpha, qreal(1.0));

    int r1 = color1.red() * alpha;
    int g1 = color1.green() * alpha;
    int b1 = color1.blue() * alpha;
    int r2 = color2.red() * (1 - alpha);
    int g2 = color2.green() * (1 - alpha);
    int b2 = color2.blue() * (1 - alpha);

    QColor alpha_color(r1 + r2, g1 + g2, b1 + b2);
    return alpha_color.rgb();
}

QRgb ByteViewText::alphaBlend(const QBrush &brush1, const QBrush &brush2, qreal alpha)
{
    return alphaBlend(brush1.color(), brush2.color(), alpha);
}

void ByteViewText::set_buf(uint8_t *raw_buf, uint buf_len) {
    this->raw_buf = raw_buf;
    this->buf_len = buf_len;
    updateScrollbars();
}

void ByteViewText::protoItemSelected(int start_byte, int byte_len, int start_bit, int bit_len) {
    if (!raw_buf) {
        return;
    }

    int end_byte = start_byte + byte_len;

    if (byte_len <= 0) {
        if (start_bit < 0) {
            return;
        }

        int total_bits = start_bit + bit_len;
        end_byte = start_byte + (total_bits / 8 + ((total_bits % 8) ? 1 : 0));
    }

    // Field bytes
    setFieldHighlight(start_byte, end_byte);
}

void ByteViewText::clear() {
    raw_buf = NULL;
    buf_len = 0;
    updateScrollbars();
    viewport()->update();
}
