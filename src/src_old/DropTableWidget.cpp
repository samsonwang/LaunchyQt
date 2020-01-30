#include "DropTableWidget.h"
#include <QDragEnterEvent>


DropTableWidget::DropTableWidget(QWidget* pParent) :
	QTableWidget(pParent)
{
	setAcceptDrops(true);
}


DropTableWidget::~DropTableWidget()
{
}


void DropTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
	emit dragEnter(event);
}


void DropTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
	emit dragMove(event);
}


void DropTableWidget::dropEvent(QDropEvent *event)
{
	emit drop(event);
}
