/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

class Filename;

/**
 *  A widget to allow the user to browse for files.
 *
 *  The FileBrowser extends the QWidget class to provide a single QWidget
 *  capable of performing typical file-browsing tasks. This class associates an
 *  editable text-entry field with a button which brings up a file selection
 *  dialog by calling either the QFileDialog::getOpenFileName() method or the
 *  QFileDialog::getSaveFileName() method.  Selecting a file through the file
 *  selection dialog will populate the text field.
 */
class FileBrowser : public QWidget
{
   Q_OBJECT

public:
	enum BrowseType
	{
		File,
		Directory
	};

   /**
    *  Creates a FileBrowser with no file selected.
    *
    *  @param   pParent
    *           The parent widget.
    */
   FileBrowser(QWidget* pParent = NULL);

   /**
    *  Destroys the widget and all child widget items.
    */
   ~FileBrowser();

   /**
    *  Sets the name of the file.
    *
    *  This method sets the name of the file to be displayed in the FileBrowser.
    *
    *  @param   filename
    *           The name of the file.
    */
   void setFilename(const QString& filename);

   /**
    *  Sets the name of the file.
    *
    *  This method sets the name of the file to be displayed in the FileBrowser.
    *
    *  @param   filename
    *           The name of the file.
    */
   void setFilename(const Filename& filename);

   /**
    *  Gets the name of the file.
    *
    *  This method gets the name of the file displayed in the FileBrowser.
    *
    *  @return  The name of the file.
    */
   QString getFilename() const;

   /**
    *  Sets the browse caption.
    *
    *  This method sets the browse caption of the FileBrowser. For more information on browse captions,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @param   caption
    *           The browse caption.
    */
   void setBrowseCaption(const QString& caption);

   /**
    *  Gets the browse caption.
    *
    *  This method gets the browse caption of the FileBrowser. For more information on browse captions,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @return  The browse caption.
    *
    */
   QString getBrowseCaption() const;

   /**
    *  Sets the browse directory.
    *
    *  This method sets the browse directory of the FileBrowser. For more information on browse directories,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @param   directory
    *           The browse directory.
    */
   void setBrowseDirectory(const QString& directory);

   /**
    *  Gets the browse directory.
    *
    *  This method gets the browse directory of the FileBrowser. For more information on browse directories,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @return  The browse directory.
    */
   QString getBrowseDirectory() const;

   /**
    *  Sets the browse file filters.
    *
    *  This method sets the browse file filters of the FileBrowser. For more information on browse file filters,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @param   filters
    *           The browse file filters.
    */
   void setBrowseFileFilters(const QString& filters);

   /**
    *  Gets the browse file filters.
    *
    *  This method gets the browse file filters of the FileBrowser. For more information on browse file filters,
    *  please refer to the QFileDialog::getOpenFileName documentation.
    *
    *  @return  The browse file filters.
    */
   QString getBrowseFileFilters() const;

   /**
    *  Sets the file selection dialog to only browse for existing files.
    *
    *  By default if this method is not called, isBrowseExistingFile() returns
    *  \c true.
    *
    *  @param   bExistingFile
    *           Set this value to \c true to browse only for existing files.
    *           Set the value to \c false to allow non-existent files to be
    *           selected.
    *
    *  @see     QFileDialog::getOpenFileName(), QFileDialog::getSaveFileName()
    */
   void setBrowseExistingFile(bool bExistingFile);

   /**
    *  Queries whether the file selection dialog only browses for existing
    *  files.
    *
    *  @return  Returns \c true if the file selection dialog only browses for
    *           existing files.  Returns \c false if non-existent files can be
    *           selected.  Returns \c true by default if setBrowseExistingFile()
    *           has not been called on this instance of the file browser.
    *
    *  @see     QFileDialog::getOpenFileName(), QFileDialog::getSaveFileName()
    */
   bool isBrowseExistingFile() const;

   void setBrowseType(BrowseType browseType);
   BrowseType getBrowseType() const;

signals:
   /**
    *  This signal is emitted when the filename is changed.
    */
   void filenameChanged(const QString& filename);

protected:
   /**
    *  Sends a focus out event if needed based on the current editing status.
    *
    *  @param   pObject
    *           The object prompting the event.
    *  @param   pEvent
    *           The event invoked by the object.
    *
    *  @return  Returns the value returned by the default QWidget
    *           implementation.
    */
   bool eventFilter(QObject* pObject, QEvent* pEvent);

protected slots:
   /**
    *  Calls QFileDialog::getOpenFileName to allow the user to specify a file.
    *  If no browse directory has been set, then the current file's directory will be used as the initial directory.
    */
   void browse();

private:
   QLineEdit* mpFileEdit;
   QPushButton* mpBrowseButton;
   QString mBrowseCaption;
   QString mBrowseDirectory;
   QString mBrowseFilters;
   bool mExistingFile;
   BrowseType mBrowseType;
};


#endif
