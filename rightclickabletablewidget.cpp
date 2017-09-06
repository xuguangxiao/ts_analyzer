#include "rightclickabletablewidget.h"
#include <QMouseEvent>
#include <QDebug>

RightClickableTableWidget::RightClickableTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    cell_row = -1;
    cell_col = -1;
}

void RightClickableTableWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::RightButton) {
        QTableWidget::mousePressEvent(event);
        return;
    }

    QTableWidgetItem* item = this->itemAt(event->pos());
    if (item) {
        cell_row = item->row();
        cell_col = item->column();
    }
}

void RightClickableTableWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::RightButton) {
        QTableWidget::mouseReleaseEvent(event);
        return;
    }

    QTableWidgetItem* item = this->itemAt(event->pos());
    if (item) {
        if (item->row() == cell_row && item->column() == cell_col) {
            emit cell_right_clicked(cell_row, cell_col, event->pos());
            cell_row = -1;
            cell_col = -1;
        }
    }
}
