/*
* The information in this file is
* Copyright(c) 2007 Ball Aerospace & Technologies Corporation
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QCompleter>
#include <QtGui/QDirModel>
#include <QtGui/QFileDialog>
#include <QtGui/QFocusEvent>
#include <QtGui/QHBoxLayout>
#include "FileBrowser.h"

// Browse button
static const char* const FileOpenIcon[] =
{
	"16 16 5 1",
	"# c #000000",
	"c c #808000",
	". c #c0c0c0",
	"b c #ffff00",
	"a c #ffffff",
	"................",
	"..........###...",
	".........#...#.#",
	"..............##",
	"..###........###",
	".#aba#######....",
	".#babababab#....",
	".#ababababa#....",
	".#baba##########",
	".#aba#ccccccccc#",
	".#ba#ccccccccc#.",
	".#a#ccccccccc#..",
	".##ccccccccc#...",
	".###########....",
	"................",
	"................"
};

FileBrowser::FileBrowser(QWidget* pParent) :
	QWidget(pParent),
	mExistingFile(true),
	mBrowseType(FileBrowser::File)
{
	// Filename edit
	QCompleter* pCompleter = new QCompleter(this);
	QDirModel* pDirModel = new QDirModel(QStringList(), QDir::NoDotAndDotDot | QDir::AllDirs | QDir::AllEntries,
		QDir::Name | QDir::DirsFirst, pCompleter);
	pCompleter->setModel(pDirModel);

	mpFileEdit = new QLineEdit(this);
	mpFileEdit->setCompleter(pCompleter);
	mpFileEdit->installEventFilter(this);

	QPixmap pixOpen(FileOpenIcon);
	pixOpen.setMask(pixOpen.createHeuristicMask());
	QIcon icnBrowse(pixOpen);

	mpBrowseButton = new QPushButton(icnBrowse, "", this);
	mpBrowseButton->setFixedWidth(25);
	mpBrowseButton->installEventFilter(this);

	// Layout
	QHBoxLayout* pLayout = new QHBoxLayout(this);
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	pLayout->addWidget(mpFileEdit, 100);
	pLayout->addWidget(mpBrowseButton);

	// Initialization
	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(mpFileEdit);
	setAutoFillBackground(true);

	// Connections
	connect(mpFileEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(filenameChanged(const QString&)));
	connect(mpBrowseButton, SIGNAL(clicked()), this, SLOT(browse()));
}

FileBrowser::~FileBrowser()
{
}

void FileBrowser::setFilename(const QString& filename)
{
	if (filename != getFilename())
	{
		mpFileEdit->setText(filename);
	}
}

QString FileBrowser::getFilename() const
{
	return mpFileEdit->text();
}

void FileBrowser::setBrowseCaption(const QString& caption)
{
	mBrowseCaption = caption;
}

QString FileBrowser::getBrowseCaption() const
{
	return mBrowseCaption;
}

void FileBrowser::setBrowseDirectory(const QString& directory)
{
	mBrowseDirectory = directory;
}

QString FileBrowser::getBrowseDirectory() const
{
	return mBrowseDirectory;
}

void FileBrowser::setBrowseFileFilters(const QString& filters)
{
	if (filters != mBrowseFilters)
	{
		mBrowseFilters = filters;

		// Update the filters in the completer
		QStringList dirFilters;

		QStringList filterList = mBrowseFilters.split(";;", QString::SkipEmptyParts);
		if (filterList.empty() == false)
		{
			// Remove the All Files filter
			for (int i = 0; i < filterList.count(); ++i)
			{
				QString filter = filterList[i];
				if (filter.startsWith("All Files") == true)
				{
					filterList.removeAt(i);
					break;
				}
			}

			// Convert the filters to the directory name filter format
			if (filterList.empty() == false)
			{
				for (int i = 0; i < filterList.count(); ++i)
				{
					QString filter = filterList[i];
					int startPos = filter.indexOf("(") + 1;
					int numChars = filter.lastIndexOf(")") - startPos;

					filter = filter.mid(startPos, numChars);
					dirFilters += filter.split(' ', QString::SkipEmptyParts);
				}
			}
		}

		QCompleter* pCompleter = mpFileEdit->completer();
		if (pCompleter != NULL)
		{
			QDirModel* pDirModel = dynamic_cast<QDirModel*>(pCompleter->model());
			if (pDirModel != NULL)
			{
				pDirModel->setNameFilters(dirFilters);
			}
		}
	}
}

QString FileBrowser::getBrowseFileFilters() const
{
	return mBrowseFilters;
}

void FileBrowser::setBrowseExistingFile(bool bExistingFile)
{
	mExistingFile = bExistingFile;
}

bool FileBrowser::isBrowseExistingFile() const
{
	return mExistingFile;
}

void FileBrowser::setBrowseType(BrowseType browseType)
{
	mBrowseType = browseType;
}

FileBrowser::BrowseType FileBrowser::getBrowseType() const
{
	return mBrowseType;
}


bool FileBrowser::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (pEvent != NULL)
	{
		if (pEvent->type() == QEvent::FocusOut)
		{
			QWidget* pFocusWidget = QApplication::focusWidget();
			if ((pFocusWidget != mpFileEdit) && (pFocusWidget != mpBrowseButton))
			{
				QFocusEvent* pFocusEvent = static_cast<QFocusEvent*>(pEvent);
				QApplication::sendEvent(this, pFocusEvent);
			}
		}
	}

	return QWidget::eventFilter(pObject, pEvent);
}

void FileBrowser::browse()
{
	// Remove the event filter on the browse button to prevent the focus
	// out event from being sent when the browse dialog is invoked
	mpBrowseButton->removeEventFilter(this);

	// Get the initial browse directory
	QString browseDirectory = getFilename();
	if (browseDirectory.isEmpty() == true)
	{
		browseDirectory = mBrowseDirectory;
	}

	// Get the filename from the user
	QString filename;
	if (mBrowseType == FileBrowser::File)
	{
		if (mExistingFile == true)
		{
			filename = QFileDialog::getOpenFileName(this, mBrowseCaption, browseDirectory, mBrowseFilters);
		}
		else
		{
			filename = QFileDialog::getSaveFileName(this, mBrowseCaption, browseDirectory, mBrowseFilters);
		}
	}
	else
	{
		filename = QFileDialog::getExistingDirectory(this, mBrowseCaption, browseDirectory);
	}

	if (filename.isEmpty() == false)
	{
		// Set the edit box text
		setFilename(filename);
		setFocus();
	}

	// Reinstall the event filter
	mpBrowseButton->installEventFilter(this);
}
