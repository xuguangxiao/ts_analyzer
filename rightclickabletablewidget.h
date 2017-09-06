#ifndef RIGHTCLICKABLETABLEWIDGET_H
#define RIGHTCLICKABLETABLEWIDGET_H

#include <QTableWidget>

class RightClickableTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    RightClickableTableWidget(QWidget* parent = NULL);

    int cell_row;
    int cell_col;

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

signals:
    void cell_right_clicked(int row, int col, QPoint &pos);
};

#endif // RIGHTCLICKABLETABLEWIDGET_H
