#include "LineEditMenu.h"


LineEditMenu::LineEditMenu(QWidget* parent) :
	QLabel(parent)
{
	setAttribute(Qt::WA_InputMethodEnabled);
}


void LineEditMenu::contextMenuEvent(QContextMenuEvent *evt)
{
	emit menuEvent(evt);
}
