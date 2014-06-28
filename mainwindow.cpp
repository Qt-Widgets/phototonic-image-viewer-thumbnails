/*
 *  Copyright (C) 2013 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "thumbview.h"
#include "dialogs.h"
#include "global.h"

#define THUMB_SIZE_MIN	50
#define THUMB_SIZE_MAX	300

Phototonic::Phototonic(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("phototonic", "phototonic_099_git_03");
	readSettings();
	createThumbView();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createFSTree();
	createImageView();
	updateExternalApps();
	loadShortcuts();
	addDockWidget(Qt::LeftDockWidgetArea, iiDock);

	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), 
				this, SLOT(updateActions(QWidget*, QWidget*)));

	restoreGeometry(GData::appSettings->value("Geometry").toByteArray());
	restoreState(GData::appSettings->value("WindowState").toByteArray());
	setWindowIcon(QIcon(":/images/phototonic.png"));

	stackedWidget = new QStackedWidget;
	stackedWidget->addWidget(thumbView);
	stackedWidget->addWidget(imageView);
	setCentralWidget(stackedWidget);

	cliImageLoaded = handleArgs();
	if (cliImageLoaded)
		loadImagefromCli();

	initComplete = true;
	thumbViewBusy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;
	refreshThumbs(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
		thumbView->setFocus(Qt::OtherFocusReason);
}

bool Phototonic::handleArgs()
{
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			selectCurrentViewDir();
			return false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
			selectCurrentViewDir();
			cliFileName = thumbView->currentViewDir + QDir::separator() + cliArg.fileName();
			return true;
		}
	}
	
	return false;
}

static bool removeDirOp(QString dirToDelete)
{
	bool ok = true;
	QDir dir(dirToDelete);

	Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
					QDir::AllDirs | QDir::Files, QDir::DirsFirst))
	{
		if (info.isDir())
			ok = removeDirOp(info.absoluteFilePath());
		else 
			ok = QFile::remove(info.absoluteFilePath());

		if (!ok)
			return ok;
	}
	ok = dir.rmdir(dirToDelete);

	return ok;
}

void Phototonic::unsetBusy()
{	
	thumbViewBusy = false;
}

void Phototonic::createThumbView()
{
	thumbView = new ThumbView(this);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();

	connect(thumbView, SIGNAL(unsetBusy()), this, SLOT(unsetBusy()));
	connect(thumbView, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));

	iiDock = new QDockWidget("Image Info", this);
	iiDock->setObjectName("Image Info");
	iiDock->setWidget(thumbView->infoView);
	connect(iiDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setToolBarsVisibility()));	
}

void Phototonic::addMenuSeparator(QWidget *widget)
{
	QAction *separator = new QAction(this);
	separator->setSeparator(true);
	widget->addAction(separator);
}

void Phototonic::createImageView()
{
	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
	imageView = new ImageView(this);
	connect(saveAction, SIGNAL(triggered()), imageView, SLOT(saveImage()));
	connect(saveAsAction, SIGNAL(triggered()), imageView, SLOT(saveImageAs()));
	connect(copyImageAction, SIGNAL(triggered()), imageView, SLOT(copyImage()));
	connect(pasteImageAction, SIGNAL(triggered()), imageView, SLOT(pasteImage()));
	imageView->ImagePopUpMenu = new QMenu();

	// Widget actions
	imageView->addAction(slideShowAction);
	imageView->addAction(nextImageAction);
	imageView->addAction(prevImageAction);
	imageView->addAction(firstImageAction);
	imageView->addAction(lastImageAction);
	imageView->addAction(randomImageAction);
	imageView->addAction(zoomInAct);
	imageView->addAction(zoomOutAct);
	imageView->addAction(origZoomAct);
	imageView->addAction(resetZoomAct);
	imageView->addAction(rotateRightAct);
	imageView->addAction(rotateLeftAct);
	imageView->addAction(freeRotateRightAct);
	imageView->addAction(freeRotateLeftAct);
	imageView->addAction(flipHAct);
	imageView->addAction(flipVAct);
	imageView->addAction(cropAct);
	imageView->addAction(saveAction);
	imageView->addAction(saveAsAction);
	imageView->addAction(copyImageAction);
	imageView->addAction(pasteImageAction);
	imageView->addAction(deleteAction);
	imageView->addAction(closeImageAct);
	imageView->addAction(fullScreenAct);
	imageView->addAction(settingsAction);
	imageView->addAction(exitAction);
	imageView->addAction(mirrorDisabledAct);
	imageView->addAction(mirrorDualAct);
	imageView->addAction(mirrorTripleAct);
	imageView->addAction(mirrorVDualAct);
	imageView->addAction(mirrorQuadAct);
	imageView->addAction(keepTransformAct);
	imageView->addAction(keepZoomAct);
	imageView->addAction(refreshAction);
	imageView->addAction(colorsAct);
	imageView->addAction(moveRightAct);
	imageView->addAction(moveLeftAct);
	imageView->addAction(moveUpAct);
	imageView->addAction(moveDownAct);
	imageView->addAction(newImageAction);

	// Actions
	imageView->ImagePopUpMenu->addAction(nextImageAction);
	imageView->ImagePopUpMenu->addAction(prevImageAction);
	imageView->ImagePopUpMenu->addAction(firstImageAction);
	imageView->ImagePopUpMenu->addAction(lastImageAction);
	imageView->ImagePopUpMenu->addAction(randomImageAction);
	imageView->ImagePopUpMenu->addAction(slideShowAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	zoomSubMenu = new QMenu("Zoom");
	zoomSubMenuAct = new QAction("Zoom", this);
	zoomSubMenuAct->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom.png")));
	zoomSubMenuAct->setMenu(zoomSubMenu);
	imageView->ImagePopUpMenu->addAction(zoomSubMenuAct);
	zoomSubMenu->addAction(zoomInAct);
	zoomSubMenu->addAction(zoomOutAct);
	zoomSubMenu->addAction(origZoomAct);
	zoomSubMenu->addAction(resetZoomAct);
	addMenuSeparator(zoomSubMenu);
	zoomSubMenu->addAction(keepZoomAct);

	transformSubMenu = new QMenu("Transform");
	transformSubMenuAct = new QAction("Transform", this);
	transformSubMenuAct->setMenu(transformSubMenu);
	imageView->ImagePopUpMenu->addAction(transformSubMenuAct);
	transformSubMenu->addAction(rotateRightAct);
	transformSubMenu->addAction(rotateLeftAct);
	transformSubMenu->addAction(freeRotateRightAct);
	transformSubMenu->addAction(freeRotateLeftAct);
	transformSubMenu->addAction(flipHAct);
	transformSubMenu->addAction(flipVAct);
	transformSubMenu->addAction(cropAct);

	MirroringSubMenu = new QMenu("Mirror");
	mirrorSubMenuAct = new QAction("Mirror", this);
	mirrorSubMenuAct->setMenu(MirroringSubMenu);
	imageView->ImagePopUpMenu->addAction(mirrorSubMenuAct);
	mirroringGroup = new QActionGroup(this);
	mirroringGroup->addAction(mirrorDisabledAct);
	mirroringGroup->addAction(mirrorDualAct);
	mirroringGroup->addAction(mirrorTripleAct);
	mirroringGroup->addAction(mirrorVDualAct);
	mirroringGroup->addAction(mirrorQuadAct);
	MirroringSubMenu->addActions(mirroringGroup->actions());
	addMenuSeparator(transformSubMenu);
	transformSubMenu->addAction(keepTransformAct);

	imageView->ImagePopUpMenu->addAction(colorsAct);
	
	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(newImageAction);
	imageView->ImagePopUpMenu->addAction(copyImageAction);
	imageView->ImagePopUpMenu->addAction(pasteImageAction);
	imageView->ImagePopUpMenu->addAction(saveAction);
	imageView->ImagePopUpMenu->addAction(saveAsAction);
	imageView->ImagePopUpMenu->addAction(deleteAction);
	imageView->ImagePopUpMenu->addAction(openWithMenuAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(fullScreenAct);
	imageView->ImagePopUpMenu->addAction(refreshAction);
	imageView->ImagePopUpMenu->addAction(closeImageAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(settingsAction);
	imageView->ImagePopUpMenu->addAction(exitAction);

	imageView->setContextMenuPolicy(Qt::DefaultContextMenu);
	GData::isFullScreen = GData::appSettings->value("isFullScreen").toBool();
	fullScreenAct->setChecked(GData::isFullScreen); 
}

void Phototonic::createActions()
{
	thumbsGoTopAct = new QAction("Top", this);
	thumbsGoTopAct->setIcon(QIcon::fromTheme("go-top", QIcon(":/images/top.png")));
	connect(thumbsGoTopAct, SIGNAL(triggered()), this, SLOT(goTop()));

	thumbsGoBottomAct = new QAction("Bottom", this);
	thumbsGoBottomAct->setIcon(QIcon::fromTheme("go-bottom", QIcon(":/images/bottom.png")));
	connect(thumbsGoBottomAct, SIGNAL(triggered()), this, SLOT(goBottom()));

	closeImageAct = new QAction("Close Image", this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(closeImage()));

	fullScreenAct = new QAction("Full Screen", this);
	fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	
	settingsAction = new QAction("Preferences", this);
	settingsAction->setIcon(QIcon::fromTheme("document-properties", QIcon(":/images/settings.png")));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction("Exit", this);
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction("Enlarge Thumbnails", this);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MAX)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction("Shrink Thumbnails", this);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MIN)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction("Cut", this);
	cutAction->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png")));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutImages()));
	cutAction->setEnabled(false);

	copyAction = new QAction("Copy", this);
	copyAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png")));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyImages()));
	copyAction->setEnabled(false);
	
	deleteAction = new QAction("Delete", this);
	deleteAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	saveAction = new QAction("Save", this);
	saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

	saveAsAction = new QAction("Save As", this);
	copyImageAction = new QAction("Copy Image", this);
	pasteImageAction = new QAction("Paste Image", this);

	renameAction = new QAction("Rename", this);
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction("Select All", this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction("About", this);
	aboutAction->setIcon(QIcon::fromTheme("help-about", QIcon(":/images/about.png")));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	// Sort actions
	actName = new QAction("Name", this);
	actTime = new QAction("Time", this);
	actSize = new QAction("Size", this);
	actType = new QAction("Type", this);
	actReverse = new QAction("Reverse", this);
	actName->setCheckable(true);
	actTime->setCheckable(true);
	actSize->setCheckable(true);
	actType->setCheckable(true);
	actReverse->setCheckable(true);
	connect(actName, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actTime, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actSize, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actType, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actReverse, SIGNAL(triggered()), this, SLOT(sortThumbnains()));

	actName->setChecked(thumbView->thumbsSortFlags == QDir::Name || 
						thumbView->thumbsSortFlags == QDir::Reversed); 
	actTime->setChecked(thumbView->thumbsSortFlags & QDir::Time); 
	actSize->setChecked(thumbView->thumbsSortFlags & QDir::Size); 
	actType->setChecked(thumbView->thumbsSortFlags & QDir::Type); 
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	actShowHidden = new QAction("Show Hidden Files", this);;
	actShowHidden->setCheckable(true);
	actShowHidden->setChecked(GData::showHiddenFiles);
	connect(actShowHidden, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

	actClassic = new QAction("Classic Thumbs", this);
	actCompact = new QAction("Compact", this);
	actSquarish = new QAction("Squarish", this);
	connect(actClassic, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));
	connect(actCompact, SIGNAL(triggered()), this, SLOT(setCompactThumbs()));
	connect(actSquarish, SIGNAL(triggered()), this, SLOT(setSquarishThumbs()));
	actClassic->setCheckable(true);
	actCompact->setCheckable(true);
	actSquarish->setCheckable(true);
	actClassic->setChecked(GData::thumbsLayout == ThumbView::Classic); 
	actCompact->setChecked(GData::thumbsLayout == ThumbView::Compact); 
	actSquarish->setChecked(GData::thumbsLayout == ThumbView::Squares); 

	refreshAction = new QAction("Reload", this);
	refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

	subFoldersAction = new QAction("Include Sub-folders", this);
	subFoldersAction->setIcon(QIcon(":/images/tree.png"));
	subFoldersAction->setCheckable(true);
	connect(subFoldersAction, SIGNAL(triggered()), this, SLOT(setIncludeSubFolders()));

	pasteAction = new QAction("Paste Here", this);
	pasteAction->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteThumbs()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction("New Folder", this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));
	
	manageDirAction = new QAction("Manage", this);
	connect(manageDirAction, SIGNAL(triggered()), this, SLOT(manageDir()));

	goBackAction = new QAction("Back", this);
	goBackAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);

	goFrwdAction = new QAction("Forward", this);
	goFrwdAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);

	goUpAction = new QAction("Up", this);
	goUpAction->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/up.png")));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction("Home", this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon::fromTheme("go-home", QIcon(":/images/home.png")));

	slideShowAction = new QAction("Slide Show", this);
	connect(slideShowAction, SIGNAL(triggered()), this, SLOT(slideShow()));

	nextImageAction = new QAction("Next", this);
	nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));
	
	prevImageAction = new QAction("Previous", this);
	prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

	firstImageAction = new QAction("First", this);
	firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
	connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

	lastImageAction = new QAction("Last", this);
	lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
	connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

	randomImageAction = new QAction("Random", this);
	connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

	openAction = new QAction("Open", this);
	openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	connect(openAction, SIGNAL(triggered()), this, SLOT(openOp()));

	newImageAction = new QAction("New Image", this);
	newImageAction->setIcon(QIcon::fromTheme("list-add", QIcon(":/images/new.png")));
	connect(newImageAction, SIGNAL(triggered()), this, SLOT(newImage()));

	openWithSubMenu = new QMenu("Open With");
	openWithMenuAct = new QAction("Open With", this);
	openWithMenuAct->setMenu(openWithSubMenu);
	chooseAppAct = new QAction("Manage External Applications", this);
	connect(chooseAppAct, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));

	zoomOutAct = new QAction("Zoom Out", this);
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
	zoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

	zoomInAct = new QAction("Zoom In", this);
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
	zoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

	resetZoomAct = new QAction("Reset Zoom", this);
	resetZoomAct->setIcon(QIcon::fromTheme("zoom-fit-best"));
	connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

	origZoomAct = new QAction("Original Size", this);
	origZoomAct->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
	connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

	keepZoomAct = new QAction("Keep Zoom", this);
	keepZoomAct->setCheckable(true);
	connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

	rotateLeftAct = new QAction(QString::fromUtf8("Rotate 90\u00b0 CCW"), this);
	rotateLeftAct->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
	connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

	rotateRightAct = new QAction(QString::fromUtf8("Rotate 90\u00b0 CW"), this);
	rotateRightAct->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
	connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

	flipHAct = new QAction("Flip Horizontally", this);
	flipHAct->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
	connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));

	flipVAct = new QAction("Flip Vertically", this);
	flipVAct->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
	connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));

	cropAct = new QAction("Cropping", this);
	connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));

	freeRotateLeftAct = new QAction(QString::fromUtf8("Rotate 1\u00b0 CCW"), this);
	connect(freeRotateLeftAct, SIGNAL(triggered()), this, SLOT(freeRotateLeft()));

	freeRotateRightAct = new QAction(QString::fromUtf8("Rotate 1\u00b0 CW"), this);
	connect(freeRotateRightAct, SIGNAL(triggered()), this, SLOT(freeRotateRight()));

	colorsAct = new QAction("Colors", this);
	connect(colorsAct, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
	colorsAct->setIcon(QIcon(":/images/colors.png"));

	mirrorDisabledAct = new QAction("Disable", this);
	mirrorDualAct = new QAction("Dual", this);
	mirrorTripleAct = new QAction("Triple", this);
	mirrorVDualAct = new QAction("Dual Vertical", this);
	mirrorQuadAct = new QAction("Quad", this);

	mirrorDisabledAct->setCheckable(true);
	mirrorDualAct->setCheckable(true);
	mirrorTripleAct->setCheckable(true);
	mirrorVDualAct->setCheckable(true);
	mirrorQuadAct->setCheckable(true);
	connect(mirrorDisabledAct, SIGNAL(triggered()), this, SLOT(setMirrorDisabled()));
	connect(mirrorDualAct, SIGNAL(triggered()), this, SLOT(setMirrorDual()));
	connect(mirrorTripleAct, SIGNAL(triggered()), this, SLOT(setMirrorTriple()));
	connect(mirrorVDualAct, SIGNAL(triggered()), this, SLOT(setMirrorVDual()));
	connect(mirrorQuadAct, SIGNAL(triggered()), this, SLOT(setMirrorQuad()));
	mirrorDisabledAct->setChecked(true); 

	keepTransformAct = new QAction("Keep Transformations", this);
	keepTransformAct->setCheckable(true);
	connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

	moveLeftAct = new QAction("Move Left", this);
	connect(moveLeftAct, SIGNAL(triggered()), this, SLOT(moveLeft()));
	moveRightAct = new QAction("Move Right", this);
	connect(moveRightAct, SIGNAL(triggered()), this, SLOT(moveRight()));
	moveUpAct = new QAction("Move Up", this);
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveUp()));
	moveDownAct = new QAction("Move Down", this);
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveDown()));

	invertSelectionAct = new QAction("Invert Selection", this);
	connect(invertSelectionAct, SIGNAL(triggered()), thumbView, SLOT(invertSelection()));
}

void Phototonic::createMenus()
{
	fileMenu = menuBar()->addMenu("File");
	fileMenu->addAction(newImageAction);
	fileMenu->addAction(openAction);
	fileMenu->addAction(subFoldersAction);
	fileMenu->addAction(createDirAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu("Edit");
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
	editMenu->addSeparator();
	editMenu->addAction(pasteAction);
	editMenu->addSeparator();
	editMenu->addAction(selectAllAction);
	editMenu->addAction(invertSelectionAct);
	editMenu->addSeparator();
	editMenu->addAction(settingsAction);

	goMenu = menuBar()->addMenu("Go");
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);
	goMenu->addSeparator();
	goMenu->addAction(thumbsGoTopAct);
	goMenu->addAction(thumbsGoBottomAct);

	viewMenu = menuBar()->addMenu("View");
	viewMenu->addAction(thumbsZoomInAct);
	viewMenu->addAction(thumbsZoomOutAct);
	sortMenu = viewMenu->addMenu("Sort By");
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
	sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
	viewMenu->addSeparator();

	viewMenu->addAction(slideShowAction);
	viewMenu->addSeparator();

	thumbLayoutsGroup = new QActionGroup(this);
	thumbLayoutsGroup->addAction(actClassic);
	thumbLayoutsGroup->addAction(actCompact);
	thumbLayoutsGroup->addAction(actSquarish);
	viewMenu->addActions(thumbLayoutsGroup->actions());
	viewMenu->addSeparator();
	viewMenu->addAction(actShowHidden);
	viewMenu->addSeparator();
	viewMenu->addAction(refreshAction);

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu("Help");
	helpMenu->addAction(aboutAction);

	// thumbview context menu
	thumbView->addAction(openAction);
	thumbView->addAction(openWithMenuAct);
	thumbView->addAction(cutAction);
	thumbView->addAction(copyAction);
	thumbView->addAction(renameAction);
	thumbView->addAction(deleteAction);
	QAction *sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(pasteAction);
	sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(selectAllAction);
	thumbView->addAction(invertSelectionAct);
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::createToolBars()
{
	/* Edit */
	editToolBar = addToolBar("Edit");
	editToolBar->setObjectName("Edit");
	editToolBar->addAction(newImageAction);
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	connect(editToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setToolBarsVisibility()));

	/* Navigation */
	goToolBar = addToolBar("Navigation");
	goToolBar->setObjectName("Navigation");
	goToolBar->addAction(goBackAction);
	goToolBar->addAction(goFrwdAction);
	goToolBar->addAction(goUpAction);
	goToolBar->addAction(goHomeAction);

	/* path bar */
	pathBar = new QLineEdit;
	pathComplete = new QCompleter(this);
	QDirModel *pathCompleteDirMod = new QDirModel;
	pathCompleteDirMod->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	pathComplete->setModel(pathCompleteDirMod);
	pathBar->setCompleter(pathComplete);
	pathBar->setMinimumWidth(200);
	pathBar->setMaximumWidth(300);
	connect(pathBar, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
	goToolBar->addWidget(pathBar);
	goToolBar->addAction(refreshAction);
	goToolBar->addAction(subFoldersAction);
	connect(goToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setToolBarsVisibility()));

	/* View */
	viewToolBar = addToolBar("View");
	viewToolBar->setObjectName("View");
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);

	/* filter bar */ 
	QAction *filterAct = new QAction("Filter", this);
	filterAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
	connect(filterAct, SIGNAL(triggered()), this, SLOT(setThumbsFilter()));
	filterBar = new QLineEdit;
	filterBar->setMinimumWidth(100);
	filterBar->setMaximumWidth(200);
	connect(filterBar, SIGNAL(returnPressed()), this, SLOT(setThumbsFilter()));
	connect(filterBar, SIGNAL(textChanged(const QString&)), this, SLOT(clearThumbsFilter()));
	filterBar->setClearButtonEnabled(true);
	filterBar->addAction(filterAct, QLineEdit::LeadingPosition);

	viewToolBar->addSeparator();
	viewToolBar->addWidget(filterBar);
	connect(viewToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setToolBarsVisibility()));	
}

void Phototonic::createStatusBar()
{
	stateLabel = new QLabel("Initializing...");
	statusBar()->addWidget(stateLabel);
}

void Phototonic::setfsModelFlags()
{
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	if (GData::showHiddenFiles)
		fsModel->setFilter(fsModel->filter() | QDir::Hidden);
}

void Phototonic::createFSTree()
{
	fsDock = new QDockWidget("File System", this);
	fsDock->setObjectName("File System");

	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	setfsModelFlags();

	fsTree = new FSTree(fsDock);
	fsDock->setWidget(fsTree);
	connect(fsDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setToolBarsVisibility()));	
	addDockWidget(Qt::LeftDockWidgetArea, fsDock);

	// Context menu
	fsTree->addAction(openAction);
	fsTree->addAction(createDirAction);
	fsTree->addAction(renameAction);
	fsTree->addAction(deleteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(pasteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(manageDirAction);
	fsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

	fsTree->setModel(fsModel);
	for (int i = 1; i <= 3; ++i)
		fsTree->hideColumn(i);
	fsTree->setHeaderHidden(true);
	connect(fsTree, SIGNAL(clicked(const QModelIndex&)),
				this, SLOT(goSelectedDir(const QModelIndex &)));

	connect(fsModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
				this, SLOT(checkDirState(const QModelIndex &, int, int)));

	connect(fsTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

	fsTree->setCurrentIndex(fsModel->index(QDir::currentPath()));
}

void Phototonic::setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag)
{
	if (act->isChecked())
		thumbView->thumbsSortFlags |= SortFlag;
}

void Phototonic::sortThumbnains()
{
	thumbView->thumbsSortFlags = 0;
	setFlagsByQAction(actName, QDir::Name);
	setFlagsByQAction(actTime, QDir::Time);
	setFlagsByQAction(actSize, QDir::Size);
	setFlagsByQAction(actType, QDir::Type);
	setFlagsByQAction(actReverse, QDir::Reversed);
	refreshThumbs(false);
}

void Phototonic::reload()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		imageView->reload();
	}
	else
		refreshThumbs(false);
}

void Phototonic::setIncludeSubFolders()
{
	GData::includeSubFolders = subFoldersAction->isChecked();
	refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop)
{
	thumbView->setNeedScroll(scrollToTop);
	QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
}

void Phototonic::setClassicThumbs()
{
	GData::thumbsLayout = ThumbView::Classic;
	refreshThumbs(false);
}

void Phototonic::setCompactThumbs()
{
	GData::thumbsLayout = ThumbView::Compact;
	refreshThumbs(false);
}

void Phototonic::setSquarishThumbs()
{
	GData::thumbsLayout = ThumbView::Squares;
	refreshThumbs(false);
}

void Phototonic::showHiddenFiles()
{
	GData::showHiddenFiles = actShowHidden->isChecked();
	setfsModelFlags();
	refreshThumbs(false);
}

void Phototonic::about()
{
	QMessageBox::about(this, "About Phototonic", "<h2>Phototonic v0.99Git03</h2>"
							"<p>Image viewer and organizer</p>"
							"<p><a href=\"http://oferkv.github.io/phototonic/\">Home page</a></p>"
							"<p><a href=\"https://github.com/oferkv/phototonic/issues\">Reports Bugs</a></p>"
							"<p>Copyright &copy; 2013-2014 Ofer Kashayov</p>"
							"<p>Contact: oferkv@live.com</p>" "Built with Qt" QT_VERSION_STR);
}

void Phototonic::runExternalApp()
{
	QString execCommand;
	QString selectedFileNames("");
	execCommand = GData::externalApps[((QAction*) sender())->text()];

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		execCommand += " \"" + imageView->currentImageFullPath + "\"";
	}
	else
	{
		QModelIndexList selectedIdxList = thumbView->selectionModel()->selectedIndexes();
		if (selectedIdxList.size() < 1)
		{
			setStatus("Invalid selection");
			return;
		}

		selectedFileNames += " ";
		for (int tn = selectedIdxList.size() - 1; tn >= 0 ; --tn)
		{
			selectedFileNames += "" +
				thumbView->thumbViewModel->item(selectedIdxList[tn].row())->data(thumbView->FileNameRole).toString();
			if (tn) 
				selectedFileNames += " ";
		}
		
		execCommand += selectedFileNames;
	}

	externalProcess.start(execCommand);
}

void Phototonic::updateExternalApps()
{
	QMapIterator<QString, QString> eaIter(GData::externalApps);
	openWithSubMenu->clear(); 	// Small leak here, fix one day.
	while (eaIter.hasNext())
	{
		eaIter.next();
		QAction *extAppAct = new QAction(eaIter.key(), this);
		extAppAct->setIcon(QIcon::fromTheme(eaIter.key()));
		connect(extAppAct, SIGNAL(triggered()), this, SLOT(runExternalApp()));
		openWithSubMenu->addAction(extAppAct);
	}

	openWithSubMenu->addSeparator();
	openWithSubMenu->addAction(chooseAppAct);
}

void Phototonic::chooseExternalApp()
{
	AppMgmtDialog *dialog = new AppMgmtDialog(this);
	dialog->exec();
	updateExternalApps();
}

void Phototonic::showSettings()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		imageView->setCursorOverrides(false);

	if (GData::slideShowActive)
		slideShow();
	
	SettingsDialog *dialog = new SettingsDialog(this);
	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->setThumbColors();
		GData::imageZoomFactor = 1.0;

		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			imageView->reload();
			needThumbsRefresh = true;
		}
		else
			refreshThumbs(false);
	}

	delete dialog;

	if (stackedWidget->currentIndex() == imageViewIdx)
		imageView->setCursorOverrides(true);
}

void Phototonic::toggleFullScreen()
{
	if (fullScreenAct->isChecked())
	{
		shouldMaximize = isMaximized();
		showFullScreen();
		GData::isFullScreen = true;
		imageView->setCursorHiding(true);
	}
	else
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
		GData::isFullScreen = false;
	}
}

void Phototonic::selectAllThumbs()
{
	thumbView->selectAll();
}

void Phototonic::createCopyCutFileList()
{
	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; ++tn)
	{
		GData::copyCutFileList.append(thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].
										row())->data(thumbView->FileNameRole).toString());
	}
}

void Phototonic::cutImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = false;
	pasteAction->setEnabled(true);
}

void Phototonic::copyImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = true;
	pasteAction->setEnabled(true);
}

void Phototonic::thumbsZoomIn()
{
	if (thumbView->thumbSize < THUMB_SIZE_MAX)
	{
		thumbView->thumbSize += 50;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MAX)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::thumbsZoomOut()
{
	if (thumbView->thumbSize > THUMB_SIZE_MIN)
	{
		thumbView->thumbSize -= 50;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MIN)
			thumbsZoomOutAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::zoomOut()
{
	GData::imageZoomFactor -= (GData::imageZoomFactor <= 0.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::zoomIn()
{
	GData::imageZoomFactor += (GData::imageZoomFactor >= 3.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::resetZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::origZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = true;
	imageView->resizeImage();
}

void Phototonic::keepZoom()
{
	GData::keepZoomFactor = keepZoomAct->isChecked();
}

void Phototonic::keepTransformClicked()
{
	GData::keepTransform = keepTransformAct->isChecked();
}

void Phototonic::rotateLeft()
{
	GData::rotation -= 90;
	if (GData::rotation < 0)
		GData::rotation = 270;
	imageView->refresh();
}

void Phototonic::rotateRight()
{
	GData::rotation += 90;
	if (GData::rotation > 270)
		GData::rotation = 0;
	imageView->refresh();
}

void Phototonic::flipVert()
{
	GData::flipV = !GData::flipV;
	imageView->refresh();
}

void Phototonic::cropImage()
{
	CropDialog *dialog = new CropDialog(this, imageView);

	imageView->setCursorOverrides(false);
	dialog->exec();
	imageView->setCursorOverrides(true);
}

void Phototonic::freeRotateLeft()
{
	--GData::rotation;
	if (GData::rotation < 0)
		GData::rotation = 359;
	imageView->refresh();
}

void Phototonic::freeRotateRight()
{
	++GData::rotation;
	if (GData::rotation > 360)
		GData::rotation = 1;
	imageView->refresh();
}

void Phototonic::showColorsDialog()
{
	ColorsDialog *dialog = new ColorsDialog(this, imageView);
	imageView->setCursorOverrides(false);
	dialog->exec();
	imageView->setCursorOverrides(true);
}

void Phototonic::flipHoriz()
{
	GData::flipH = !GData::flipH;
	imageView->refresh();
}

void Phototonic::moveRight()
{
	imageView->keyMoveEvent(ImageView::MoveRight);
}

void Phototonic::moveLeft()
{
	imageView->keyMoveEvent(ImageView::MoveLeft);
}

void Phototonic::moveUp()
{
	imageView->keyMoveEvent(ImageView::MoveUp);
}

void Phototonic::moveDown()
{
	imageView->keyMoveEvent(ImageView::MoveDown);
}

void Phototonic::setMirrorDisabled()
{
	imageView->mirrorLayout = ImageView::LayNone;
	imageView->refresh();
}

void Phototonic::setMirrorDual()
{
	imageView->mirrorLayout = ImageView::LayDual;
	imageView->refresh();
}

void Phototonic::setMirrorTriple()
{
	imageView->mirrorLayout = ImageView::LayTriple;
	imageView->refresh();
}

void Phototonic::setMirrorVDual()
{
	imageView->mirrorLayout = ImageView::LayVDual;
	imageView->refresh();
}

void Phototonic::setMirrorQuad()
{
	imageView->mirrorLayout = ImageView::LayQuad;
	imageView->refresh();
}

bool Phototonic::isValidPath(QString &path)
{
	QDir checkPath(path);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		return false;
	}
	return true;
}

void Phototonic::pasteThumbs()
{
	if (!copyCutCount)
		return;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Can not paste in " + destDir);
		selectCurrentViewDir();
		return;
	}

	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);

	QFileInfo fileInfo;
	if (!GData::copyOp && pasteInCurrDir)
	{
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			fileInfo = QFileInfo(GData::copyCutFileList[tn]);
			if (fileInfo.absolutePath() == destDir)
			{
				QMessageBox msgBox;
				msgBox.critical(this, "Error", "Can not cut and paste in the same folder");
				return;
			}
		}
	}

	CpMvDialog *dialog = new CpMvDialog(this);
	dialog->exec(thumbView, destDir, pasteInCurrDir);
	if (pasteInCurrDir)
	{
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			thumbView->addThumb(GData::copyCutFileList[tn]);
		}
	}
	
	QString state = QString((GData::copyOp? "Copied " : "Moved ") + 
								QString::number(dialog->nfiles) + " images");
	setStatus(state);
	delete(dialog);
	selectCurrentViewDir();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	thumbView->loadVisibleThumbs();
}

void Phototonic::deleteSingleImage()
{
	bool ok;
	int ret;

	ret = QMessageBox::warning(this, "Delete image", "Permanently delete this image?",
									QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	int currentRow = thumbView->getCurrentRow();
	if (ret == QMessageBox::Yes)
	{
		ok = QFile::remove(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());

		if (ok)
		{
			 thumbView->thumbViewModel->removeRow(currentRow);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to delete image");
			return;
		}

		bool wrapImageListTmp = GData::wrapImageList;
		GData::wrapImageList = false;

		if (currentRow > 0)
		{
			thumbView->setCurrentRow(currentRow - 1);
		}

		if (thumbView->getNextRow() < 0 && currentRow > 0)
		{
			loadImageFile(thumbView->thumbViewModel->item(currentRow - 1)->data(thumbView->FileNameRole).toString());
		}
		else
		{
			if (thumbView->thumbViewModel->rowCount() == 0)
			{
				closeImage();
				refreshThumbs(true);
				return;
			}
			loadImageFile(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());
		}
			
		GData::wrapImageList = wrapImageListTmp;
	}
}

void Phototonic::deleteOp()
{
	if (QApplication::focusWidget() == fsTree)
	{
		deleteDir();
		return;
	}

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		deleteSingleImage();
		return;
	}

	if (thumbView->selectionModel()->selectedIndexes().size() < 1)
	{
		setStatus("No selection");
		return;
	}

	bool ok;
	int ret = QMessageBox::warning(this, "Delete images", "Permanently delete selected images?",
										QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	QModelIndexList indexesList;
	int nfiles = 0;
	if (ret == QMessageBox::Yes)
	{
		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->thumbViewModel->item(
								indexesList.first().row())->data(thumbView->FileNameRole).toString());

			++nfiles;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, "Error", "Failed to delete image");
				return;
			}
		}
		
		QString state = QString("Deleted " + QString::number(nfiles) + " images");
		setStatus(state);

		thumbView->loadVisibleThumbs();
	}
}

void Phototonic::goTo(QString path)
{
	thumbView->setNeedScroll(true);
	fsTree->setCurrentIndex(fsModel->index(path));
	thumbView->currentViewDir = path;
	refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex &idx)
{
	thumbView->setNeedScroll(true);
	thumbView->currentViewDir = getSelectedPath();
	refreshThumbs(true);
	fsTree->expand(idx);
}

void Phototonic::goPathBarDir()
{
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid Path: " + pathBar->text());
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	selectCurrentViewDir();
	refreshThumbs(true);
}

void Phototonic::setThumbsFilter()
{
	thumbView->filterStr = filterBar->text();
	refreshThumbs(true);
}

void Phototonic::clearThumbsFilter()
{
	if (filterBar->text() == "")
	{
		thumbView->filterStr = filterBar->text();
		refreshThumbs(true);
	}
}

void Phototonic::goBack()
{
	if (currentHistoryIdx > 0)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(--currentHistoryIdx));
		goFrwdAction->setEnabled(true);
		if (currentHistoryIdx == 0)
			goBackAction->setEnabled(false);
	}
}

void Phototonic::goForward()
{

	if (currentHistoryIdx < pathHistory.size() - 1)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(++currentHistoryIdx));
		if (currentHistoryIdx == (pathHistory.size() - 1))
			goFrwdAction->setEnabled(false);
	}
}

void Phototonic::goUp()
{
	QString parentDir = 
			thumbView->currentViewDir.left(thumbView->currentViewDir.lastIndexOf(QDir::separator()));

	if (parentDir.size() == 0)
		parentDir = QDir::separator();
	goTo(parentDir);
}

void Phototonic::goHome()
{
	goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled)
{
	cutAction->setEnabled(setEnabled);
	copyAction->setEnabled(setEnabled);
}

void Phototonic::changeActionsBySelection(const QItemSelection&, const QItemSelection&)
{
	setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
}

void Phototonic::updateActions(QWidget*, QWidget *selectedWidget)
{
	if (selectedWidget == fsTree)
		setCopyCutActions(false);
	else if (selectedWidget == thumbView)
		setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
}

void Phototonic::writeSettings()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		setThumbViewWidgetsVisible(true);

	if(isFullScreen())
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
	}
	QApplication::processEvents();

	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbSize);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
	GData::appSettings->setValue("thumbPagesReadahead", (int)GData::thumbPagesReadahead);
	GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
	GData::appSettings->setValue("exitInsteadOfClose", (int)GData::exitInsteadOfClose);
	GData::appSettings->setValue("enableAnimations", (bool)GData::enableAnimations);
	GData::appSettings->setValue("exifRotationEnabled", (bool)GData::exifRotationEnabled);
	GData::appSettings->setValue("showHiddenFiles", (bool)GData::showHiddenFiles);
	GData::appSettings->setValue("wrapImageList", (bool)GData::wrapImageList);
	GData::appSettings->setValue("imageZoomFactor", (float)GData::imageZoomFactor);
	GData::appSettings->setValue("shouldMaximize", (bool)isMaximized());
	GData::appSettings->setValue("defaultSaveQuality", (int)GData::defaultSaveQuality);
	GData::appSettings->setValue("noEnlargeSmallThumb", (bool)GData::noEnlargeSmallThumb);
	GData::appSettings->setValue("slideShowDelay", (int)GData::slideShowDelay);
	GData::appSettings->setValue("slideShowRandom", (bool)GData::slideShowRandom);
	GData::appSettings->setValue("editToolBarVisible", (bool)editToolBar->isVisible());
	GData::appSettings->setValue("goToolBarVisible", (bool)goToolBar->isVisible());
	GData::appSettings->setValue("viewToolBarVisible", (bool)viewToolBar->isVisible());
	GData::appSettings->setValue("fsDockVisible", (bool)fsDock->isVisible());
	GData::appSettings->setValue("iiDockVisible", (bool)iiDock->isVisible());

	/* Action shortcuts */
	GData::appSettings->beginGroup("Shortcuts");
	QMapIterator<QString, QAction *> scIter(GData::actionKeys);
	while (scIter.hasNext())
	{
		scIter.next();
		GData::appSettings->setValue(scIter.key(), scIter.value()->shortcut().toString());
	}
	GData::appSettings->endGroup();

	/* External apps */
	GData::appSettings->beginGroup("ExternalApps");
	GData::appSettings->remove("");
	QMapIterator<QString, QString> eaIter(GData::externalApps);
	while (eaIter.hasNext())
	{
		eaIter.next();
		GData::appSettings->setValue(eaIter.key(), eaIter.value());
	}
	GData::appSettings->endGroup();

	GData::appSettings->setValue("Geometry", saveGeometry());
	GData::appSettings->setValue("WindowState", saveState());
}

void Phototonic::readSettings()
{
	initComplete = false;
	needThumbsRefresh = false;

	if (!GData::appSettings->contains("thumbsZoomVal"))
	{
		resize(800, 600);
		GData::appSettings->setValue("thumbsSortFlags", (int)0);
		GData::appSettings->setValue("thumbsZoomVal", (int)200);
		GData::appSettings->setValue("isFullScreen", (bool)false);
		GData::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
		GData::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
		GData::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
		GData::appSettings->setValue("thumbSpacing", (int)5);
		GData::appSettings->setValue("thumbPagesReadahead", (int)2);
		GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
		GData::appSettings->setValue("zoomOutFlags", (int)1);
		GData::appSettings->setValue("zoomInFlags", (int)0);
		GData::appSettings->setValue("wrapImageList", (bool)false);
		GData::appSettings->setValue("exitInsteadOfClose", (int)0);
		GData::appSettings->setValue("imageZoomFactor", (float)1.0);
		GData::appSettings->setValue("defaultSaveQuality", (int)85);
		GData::appSettings->setValue("noEnlargeSmallThumb", (bool)true);
		GData::appSettings->setValue("enableAnimations", (bool)true);
		GData::appSettings->setValue("exifRotationEnabled", (bool)true);
		GData::appSettings->setValue("showHiddenFiles", (bool)false);
		GData::appSettings->setValue("slideShowDelay", (int)5);
		GData::appSettings->setValue("slideShowRandom", (bool)false);
		GData::appSettings->setValue("editToolBarVisible", (bool)true);
		GData::appSettings->setValue("goToolBarVisible", (bool)true);
		GData::appSettings->setValue("viewToolBarVisible", (bool)true);
		GData::appSettings->setValue("fsDockVisible", (bool)true);
		GData::appSettings->setValue("iiDockVisible", (bool)true);
	}

	GData::exitInsteadOfClose = GData::appSettings->value("exitInsteadOfClose").toBool();
	GData::enableAnimations = GData::appSettings->value("enableAnimations").toBool();
	GData::exifRotationEnabled = GData::appSettings->value("exifRotationEnabled").toBool();
	GData::showHiddenFiles = GData::appSettings->value("showHiddenFiles").toBool();
	GData::wrapImageList = GData::appSettings->value("wrapImageList").toBool();
	GData::imageZoomFactor = GData::appSettings->value("imageZoomFactor").toFloat();
	GData::zoomOutFlags = GData::appSettings->value("zoomOutFlags").toInt();
	GData::zoomInFlags = GData::appSettings->value("zoomInFlags").toInt();
	GData::rotation = 0;
	GData::keepTransform = false;
	shouldMaximize = GData::appSettings->value("shouldMaximize").toBool();
	GData::flipH = false;
	GData::flipV = false;
	GData::defaultSaveQuality = GData::appSettings->value("defaultSaveQuality").toInt();
	GData::noEnlargeSmallThumb = GData::appSettings->value("noEnlargeSmallThumb").toBool();
	GData::slideShowDelay = GData::appSettings->value("slideShowDelay").toInt();
	GData::slideShowRandom = GData::appSettings->value("slideShowRandom").toBool();
	GData::slideShowActive = false;
	editToolBarVisible = GData::appSettings->value("editToolBarVisible").toBool();
	goToolBarVisible = GData::appSettings->value("goToolBarVisible").toBool();
	viewToolBarVisible = GData::appSettings->value("viewToolBarVisible").toBool();
	fsDockVisible = GData::appSettings->value("fsDockVisible").toBool();
	iiDockVisible = GData::appSettings->value("iiDockVisible").toBool();

	GData::appSettings->beginGroup("ExternalApps");
	QStringList extApps = GData::appSettings->childKeys();
	for (int i = 0; i < extApps.size(); ++i)
	{
		GData::externalApps[extApps.at(i)] = 	GData::appSettings->value(extApps.at(i)).toString();
	}
	GData::appSettings->endGroup();
}

void Phototonic::loadShortcuts()
{
	// Add customizable key shortcut actions
	GData::actionKeys[thumbsGoTopAct->text()] = thumbsGoTopAct;
	GData::actionKeys[thumbsGoBottomAct->text()] = thumbsGoBottomAct;
	GData::actionKeys[closeImageAct->text()] = closeImageAct;
	GData::actionKeys[fullScreenAct->text()] = fullScreenAct;
	GData::actionKeys[settingsAction->text()] = settingsAction;
	GData::actionKeys[exitAction->text()] = exitAction;
	GData::actionKeys[thumbsZoomInAct->text()] = thumbsZoomInAct;
	GData::actionKeys[thumbsZoomOutAct->text()] = thumbsZoomOutAct;
	GData::actionKeys[cutAction->text()] = cutAction;
	GData::actionKeys[copyAction->text()] = copyAction;
	GData::actionKeys[nextImageAction->text()] = nextImageAction;
	GData::actionKeys[prevImageAction->text()] = prevImageAction;
	GData::actionKeys[deleteAction->text()] = deleteAction;
	GData::actionKeys[saveAction->text()] = saveAction;
	GData::actionKeys[saveAsAction->text()] = saveAsAction;
	GData::actionKeys[keepTransformAct->text()] = keepTransformAct;
	GData::actionKeys[keepZoomAct->text()] = keepZoomAct;
	GData::actionKeys[newImageAction->text()] = newImageAction;
	GData::actionKeys[copyImageAction->text()] = copyImageAction;
	GData::actionKeys[pasteImageAction->text()] = pasteImageAction;
	GData::actionKeys[renameAction->text()] = renameAction;
	GData::actionKeys[refreshAction->text()] = refreshAction;
	GData::actionKeys[pasteAction->text()] = pasteAction;
	GData::actionKeys[goBackAction->text()] = goBackAction;
	GData::actionKeys[goFrwdAction->text()] = goFrwdAction;
	GData::actionKeys[slideShowAction->text()] = slideShowAction;
	GData::actionKeys[firstImageAction->text()] = firstImageAction;
	GData::actionKeys[lastImageAction->text()] = lastImageAction;
	GData::actionKeys[randomImageAction->text()] = randomImageAction;
	GData::actionKeys[openAction->text()] = openAction;
	GData::actionKeys[zoomOutAct->text()] = zoomOutAct;
	GData::actionKeys[zoomInAct->text()] = zoomInAct;
	GData::actionKeys[resetZoomAct->text()] = resetZoomAct;
	GData::actionKeys[origZoomAct->text()] = origZoomAct;
	GData::actionKeys[rotateLeftAct->text()] = rotateLeftAct;
	GData::actionKeys[rotateRightAct->text()] = rotateRightAct;
	GData::actionKeys[freeRotateLeftAct->text()] = freeRotateLeftAct;
	GData::actionKeys[freeRotateRightAct->text()] = freeRotateRightAct;
	GData::actionKeys[flipHAct->text()] = flipHAct;
	GData::actionKeys[flipVAct->text()] = flipVAct;
	GData::actionKeys[cropAct->text()] = cropAct;
	GData::actionKeys[colorsAct->text()] = colorsAct;
	GData::actionKeys[mirrorDisabledAct->text()] = mirrorDisabledAct;
	GData::actionKeys[mirrorDualAct->text()] = mirrorDualAct;
	GData::actionKeys[mirrorTripleAct->text()] = mirrorTripleAct;
	GData::actionKeys[mirrorVDualAct->text()] = mirrorVDualAct;
	GData::actionKeys[mirrorQuadAct->text()] = mirrorQuadAct;
	GData::actionKeys[moveDownAct->text()] = moveDownAct;
	GData::actionKeys[moveUpAct->text()] = moveUpAct;
	GData::actionKeys[moveRightAct->text()] = moveRightAct;
	GData::actionKeys[moveLeftAct->text()] = moveLeftAct;

	GData::appSettings->beginGroup("Shortcuts");
	QStringList groupKeys = GData::appSettings->childKeys();

	if (groupKeys.size())
	{
		for (int i = 0; i < groupKeys.size(); ++i)
		{
			if (GData::actionKeys.value(groupKeys.at(i)))
				GData::actionKeys.value(groupKeys.at(i))->setShortcut
											(GData::appSettings->value(groupKeys.at(i)).toString());
		}
	}
	else
	{
		thumbsGoTopAct->setShortcut(QKeySequence("Ctrl+Home"));
		thumbsGoBottomAct->setShortcut(QKeySequence("Ctrl+End"));
		closeImageAct->setShortcut(Qt::Key_Escape);
		fullScreenAct->setShortcut(QKeySequence("F"));
		settingsAction->setShortcut(QKeySequence("P"));
		exitAction->setShortcut(QKeySequence("Ctrl+Q"));
		cutAction->setShortcut(QKeySequence("Ctrl+X"));
		copyAction->setShortcut(QKeySequence("Ctrl+C"));
		deleteAction->setShortcut(QKeySequence("Del"));
		saveAction->setShortcut(QKeySequence("Ctrl+S"));
		copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
		pasteImageAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
		renameAction->setShortcut(QKeySequence("F2"));
		refreshAction->setShortcut(QKeySequence("F5"));
		pasteAction->setShortcut(QKeySequence("Ctrl+V"));
		goBackAction->setShortcut(QKeySequence("Alt+Left"));
		goFrwdAction->setShortcut(QKeySequence("Alt+Right"));
		slideShowAction->setShortcut(QKeySequence("W"));
		nextImageAction->setShortcut(QKeySequence("PgDown"));
		prevImageAction->setShortcut(QKeySequence("PgUp"));
		firstImageAction->setShortcut(QKeySequence("Home"));
		lastImageAction->setShortcut(QKeySequence("End"));
		randomImageAction->setShortcut(QKeySequence("R"));
		openAction->setShortcut(QKeySequence("Return"));
		zoomOutAct->setShortcut(QKeySequence("Shift+Z"));
		zoomInAct->setShortcut(QKeySequence("Z"));
		resetZoomAct->setShortcut(QKeySequence("Ctrl+Z"));
		origZoomAct->setShortcut(QKeySequence("Alt+Z"));
		rotateLeftAct->setShortcut(QKeySequence("Ctrl+Left"));
		rotateRightAct->setShortcut(QKeySequence("Ctrl+Right"));
		freeRotateLeftAct->setShortcut(QKeySequence("Ctrl+Shift+Left"));
		freeRotateRightAct->setShortcut(QKeySequence("Ctrl+Shift+Right"));
		flipHAct->setShortcut(QKeySequence("Ctrl+Down"));
		flipVAct->setShortcut(QKeySequence("Ctrl+Up"));
		cropAct->setShortcut(QKeySequence("Ctrl+R"));
		colorsAct->setShortcut(QKeySequence("C"));
		mirrorDisabledAct->setShortcut(QKeySequence("Ctrl+1"));
		mirrorDualAct->setShortcut(QKeySequence("Ctrl+2"));
		mirrorTripleAct->setShortcut(QKeySequence("Ctrl+3"));
		mirrorVDualAct->setShortcut(QKeySequence("Ctrl+4"));
		mirrorQuadAct->setShortcut(QKeySequence("Ctrl+5"));
		moveDownAct->setShortcut(QKeySequence("Down"));
		moveUpAct->setShortcut(QKeySequence("Up"));
		moveLeftAct->setShortcut(QKeySequence("Left"));
		moveRightAct->setShortcut(QKeySequence("Right"));
	}
		
	GData::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event)
{
	thumbView->abort();
	writeSettings();
	event->accept();
}

void Phototonic::setStatus(QString state)
{
	stateLabel->setText("    " + state + "    ");
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			closeImage();
			event->accept();
		}
	}
}

void Phototonic::mousePressEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::MiddleButton)
		{
			fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
			toggleFullScreen();
			event->accept();
		}
		else if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(true, event->x(), event->y());
			QApplication::setOverrideCursor(Qt::ClosedHandCursor);
			event->accept();
		}
	}
}

void Phototonic::mouseReleaseEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(false, 0, 0);
			QApplication::setOverrideCursor(Qt::OpenHandCursor);
		}
	}
}

void Phototonic::newImage()
{
	loadImageFile("");
}

void Phototonic::setThumbViewWidgetsVisible(bool visible)
{
	menuBar()->setVisible(visible);
	menuBar()->setDisabled(!visible);
	statusBar()->setVisible(visible);

	editToolBar->setVisible(visible? editToolBarVisible : false);
	goToolBar->setVisible(visible? goToolBarVisible : false);
	viewToolBar->setVisible(visible? viewToolBarVisible : false);

	fsDock->setVisible(visible? fsDockVisible : false);
	iiDock->setVisible(visible? iiDockVisible : false);
}

void Phototonic::openOp()
{
	if (QApplication::focusWidget() == fsTree)
		goSelectedDir(fsTree->getCurrentIndex());
	else if (QApplication::focusWidget() == thumbView)
	{
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		if (indexesList.size() != 1)
		{
			setStatus("Invalid selection");
			return;
		}

		loadImagefromThumb(indexesList.first());
	}
	else if (QApplication::focusWidget() == filterBar)
	{
		setThumbsFilter();
	}
	else if (QApplication::focusWidget() == pathBar)
	{
		goPathBarDir();
	}
}

void Phototonic::setToolBarsVisibility()
{
	editToolBarVisible = editToolBar->isVisible();
	goToolBarVisible = goToolBar->isVisible();
	viewToolBarVisible = viewToolBar->isVisible();
	fsDockVisible = fsDock->isVisible();
	iiDockVisible = iiDock->isVisible();
}

void Phototonic::loadImageFile(QString imageFileName)
{
	imageView->loadImage(imageFileName);
	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		QApplication::setOverrideCursor(Qt::OpenHandCursor);
		stackedWidget->setCurrentIndex(imageViewIdx);
		setThumbViewWidgetsVisible(false);
		if (GData::isFullScreen == true)
		{
			shouldMaximize = isMaximized();
			showFullScreen();
			imageView->setCursorHiding(true);
		}
	}
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx)
{
	thumbView->setCurrentRow(idx.row());
	loadImageFile(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString());
}

void Phototonic::loadImagefromCli()
{
	QFile imageFile(cliFileName);
	if(!imageFile.exists()) 
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to open file \"" + cliFileName + "\", file not found");
		cliFileName = "";
		return;
	}

	loadImageFile(cliFileName);
	thumbView->setCurrentIndexByName(cliFileName);
}

void Phototonic::slideShow()
{
	if (GData::slideShowActive)
	{
		GData::slideShowActive = false;
		slideShowAction->setText("Slide Show");
		imageView->popMessage("Slide show stopped");

		SlideShowTimer->stop();
		delete SlideShowTimer;
	}
	else
	{
		if (thumbView->thumbViewModel->rowCount() <= 0)
			return;
	
		if (stackedWidget->currentIndex() == thumbViewIdx)
		{
			QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
			if (indexesList.size() != 1)
				thumbView->setCurrentRow(0);
			else
				thumbView->setCurrentRow(indexesList.first().row());
		}
	
		GData::slideShowActive = true;

		SlideShowTimer = new QTimer(this);
		connect(SlideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowHandler()));
		SlideShowTimer->start(GData::slideShowDelay * 1000);
		slideShowHandler();

		slideShowAction->setText("End Slide Show");
		imageView->popMessage("Slide show started");
	}
}

void Phototonic::slideShowHandler()
{
	if (GData::slideShowActive)
	{
		if (GData::slideShowRandom)
		{
			loadRandomImage();
		}
		else
		{
			int currentRow = thumbView->getCurrentRow();
			loadImageFile(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());

			if (thumbView->getNextRow() > 0)
				thumbView->setCurrentRow(thumbView->getNextRow());
			else 
			{
				if (GData::wrapImageList)
					thumbView->setCurrentRow(0);
				else
					slideShow();
			}
		}
	}
}

void Phototonic::loadNextImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int nextRow = thumbView->getNextRow();
	if (nextRow < 0) 
	{
		if (GData::wrapImageList)
			nextRow = 0;
		else
			return;
	}

	loadImageFile(thumbView->thumbViewModel->item(nextRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(nextRow);
}

void Phototonic::loadPrevImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int prevRow = thumbView->getPrevRow();
	if (prevRow < 0) 
	{
		if (GData::wrapImageList)
			prevRow = thumbView->getLastRow();
		else
			return;
	}
	
	loadImageFile(thumbView->thumbViewModel->item(prevRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(prevRow);
}

void Phototonic::loadFirstImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	loadImageFile(thumbView->thumbViewModel->item(0)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(0);
}

void Phototonic::loadLastImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int lastRow = thumbView->getLastRow();
	loadImageFile(thumbView->thumbViewModel->item(lastRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(lastRow);
}

void Phototonic::loadRandomImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int randomRow = thumbView->getRandomRow();
	loadImageFile(thumbView->thumbViewModel->item(randomRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(randomRow);
}

void Phototonic::scrollToLastImage()
{
	if (thumbView->thumbViewModel->rowCount() > 0) 
	{
		thumbView->setCurrentIndex(thumbView->thumbViewModel->index(0, 0));
		thumbView->setCurrentIndexByName(imageView->currentImageFullPath);
		thumbView->selectCurrentIndex();
	}
}

void Phototonic::closeImage()
{
	if (cliImageLoaded && GData::exitInsteadOfClose)
		close();

	setThumbViewWidgetsVisible(true);
	stackedWidget->setCurrentIndex(thumbViewIdx);

	if (isFullScreen())
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
	}
	while (QApplication::overrideCursor())
		QApplication::restoreOverrideCursor();

	if (GData::slideShowActive)
		slideShow();

	if (needThumbsRefresh)
	{
		needThumbsRefresh = false;
		refreshThumbs(true);
	}

	thumbView->setFocus(Qt::OtherFocusReason);
	thumbView->loadVisibleThumbs();
	setThumbviewWindowTitle();

	if (!needThumbsRefresh)
		QTimer::singleShot(100, this, SLOT(scrollToLastImage()));
}

void Phototonic::goBottom()
{
	thumbView->scrollToBottom();
}

void Phototonic::goTop()
{
	thumbView->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath)
{
	QApplication::restoreOverrideCursor();
	GData::copyOp = (keyMods == Qt::ControlModifier);
	QMessageBox msgBox;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		msgBox.critical(this, "Error", "Can not move or copy images to this folder");
		selectCurrentViewDir();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir)
	{
		msgBox.critical(this, "Error", "Destination folder is same as source");
		return;
	}

	if (dirOp)
	{
		QString dirOnly = cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);
		QFile dir(cpMvDirPath);
		bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
		if (!ok)
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "Failed to move folder");
		}
		setStatus("Folder moved");
	}
	else
	{
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false);
		QString state = QString((GData::copyOp? "Copied " : "Moved ") + QString::number(cpMvdialog->nfiles) + " images");
		setStatus(state);
		delete(cpMvdialog);
	}

	thumbView->loadVisibleThumbs();
}

void Phototonic::selectCurrentViewDir()
{
	QModelIndex idx = fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
	{
		fsTree->expand(idx);
		fsTree->setCurrentIndex(idx);
	}
}

void Phototonic::checkDirState(const QModelIndex &, int, int)
{
	if (!initComplete)
	{
		return;
	}

	if (thumbViewBusy)
	{
		thumbView->abort();
	}

	if (!QDir().exists(thumbView->currentViewDir))
	{
		thumbView->currentViewDir = "";
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::recordHistory(QString dir)
{
	if (!needHistoryRecord)
	{
		needHistoryRecord = true;
		return;
	}

	if (pathHistory.size() && dir == pathHistory.at(currentHistoryIdx))
		return;
		
	pathHistory.insert(++currentHistoryIdx, dir);

	// Need to clear irrelevant items from list
	if (currentHistoryIdx != pathHistory.size() - 1)
	{	
		goFrwdAction->setEnabled(false);
		for (int i = pathHistory.size() - 1; i > currentHistoryIdx ; --i)
		{
			pathHistory.removeAt(i);
		}
	}
}

void Phototonic::reloadThumbsSlot()
{
	if (thumbViewBusy || !initComplete)
	{	
		thumbView->abort();
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
		return;
	}

	if (thumbView->currentViewDir == "")
	{
		thumbView->currentViewDir = getSelectedPath();
		if (thumbView->currentViewDir == "")
			return;
	}

	QDir checkPath(thumbView->currentViewDir);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to open folder: " + thumbView->currentViewDir);
		return;
	}

	thumbView->infoView->clear();
	pathBar->setText(thumbView->currentViewDir);
	recordHistory(thumbView->currentViewDir);
	if (currentHistoryIdx > 0)
		goBackAction->setEnabled(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
	{
		setThumbviewWindowTitle();
	}

	thumbViewBusy = true;
	thumbView->load(cliFileName);
}

void Phototonic::setThumbviewWindowTitle()
{
	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
}

void Phototonic::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = "Rename " + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							"New name:", QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		selectCurrentViewDir();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to rename folder");
		selectCurrentViewDir();
		return;
	}

	if (thumbView->currentViewDir == dirInfo.absoluteFilePath()) 
		fsTree->setCurrentIndex(fsModel->index(newFullPathName));
	else
		selectCurrentViewDir();
}

void Phototonic::rename()
{
	if (QApplication::focusWidget() == fsTree)
	{
		renameDir();
		return;
	}
		
	QString selectedImageFileName = thumbView->getSingleSelectionFilename();
	if (selectedImageFileName.isEmpty())
	{
		setStatus("Invalid selection");
		return;
	}

	bool ok;
	QString fileName = QFileInfo(selectedImageFileName).fileName();
	QString title = "Rename " + fileName;
	QString newImageName = QInputDialog::getText(this, title, "New name:",
															QLineEdit::Normal, fileName, &ok);

	if (!ok)													
		return;

	if(newImageName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "No name entered");
		return;
	}

	QString currnetFilePath = selectedImageFileName;
	QFile currentFile(currnetFilePath);
	QString newImageFullPath = thumbView->currentViewDir + QDir::separator() + newImageName;
	ok = currentFile.rename(newImageFullPath);

	if (ok)
	{
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageFullPath, 
																			thumbView->FileNameRole);
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName,
																			Qt::DisplayRole);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to rename image");
	}
}

void Phototonic::deleteDir()
{
	bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = "Permanently delete " + dirInfo.completeBaseName() + " and all of its contents?";

	int ret = QMessageBox::warning(this, "Delete folder", question,
								QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	if (ret == QMessageBox::Yes)
		ok = removeDirOp(deletePath);
	else
	{
		selectCurrentViewDir();
		return;
	}

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to delete folder");
		selectCurrentViewDir();
	}

	QString state = QString("Removed " + deletePath);
	setStatus(state);

	if (thumbView->currentViewDir == deletePath) 
	{
		if (idxAbove.isValid())
			fsTree->setCurrentIndex(idxAbove);
	}
	else
		selectCurrentViewDir();
}

void Phototonic::createSubDirectory()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString newDirName = QInputDialog::getText(this, "New Sub folder", 
							"New folder name:", QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		selectCurrentViewDir();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to create new folder");
		selectCurrentViewDir();
		return;
	}

	setStatus("Created " + newDirName);
	fsTree->expand(selectedDirs[0]);
	selectCurrentViewDir();
}

void Phototonic::manageDir()
{
	setStatus("Executing file manager...");
	QDesktopServices::openUrl(QUrl("file:///" + getSelectedPath()));
}

QString Phototonic::getSelectedPath()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	if (selectedDirs.size() && selectedDirs[0].isValid())
	{
		QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));
		return dirInfo.absoluteFilePath();
	}
	else
		return 0;
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->delta() < 0)
			loadNextImage();
		else
			loadPrevImage();
	}
}

