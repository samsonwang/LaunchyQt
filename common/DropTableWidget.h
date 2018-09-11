#ifndef DROPTABLEWIDGET_H
#define DROPTABLEWIDGET_H


#include <QtGui/QTableWidget>


class DropTableWidget : public QTableWidget
{
   Q_OBJECT

public:

    DropTableWidget(QWidget* pParent = NULL);
   ~DropTableWidget();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

signals:
    void dragEnter(QDragEnterEvent *event);
    void dragMove(QDragMoveEvent *event);
    void drop(QDropEvent *event);
};


#endif
