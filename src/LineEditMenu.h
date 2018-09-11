#ifndef LINEEDITMENU_H
#define LINEEDITMENU_H


#include <QtGui/QLabel>


class LineEditMenu : public QLabel
{
	Q_OBJECT

public:
	LineEditMenu(QWidget* parent = 0);
	void contextMenuEvent(QContextMenuEvent *event);

signals:
	void menuEvent(QContextMenuEvent*);
};


#endif
