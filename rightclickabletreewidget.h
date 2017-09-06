#ifndef RIGHTCLICKABLETREEWIDGET_H
#define RIGHTCLICKABLETREEWIDGET_H

#include <QTreeWidget>

class RightClickableTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    RightClickableTreeWidget(QWidget* parent = NULL);

    QTreeWidgetItem* right_click_item;

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

signals:
    void item_right_clicked(QTreeWidgetItem* item, QPoint& pos);
};

#endif // RIGHTCLICKABLETREEWIDGET_H
