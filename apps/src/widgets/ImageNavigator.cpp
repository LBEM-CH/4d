/**************************************************************************
 *   Copyright (C) 2006 by UC Davis Stahlberg Laboratory                   *
 *   HStahlberg@ucdavis.edu                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QPaintEvent>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QDebug>
#include <cmath>

#include "ApplicationData.h"
#include "ProjectData.h"
#include "ImageNavigator.h"

ImageNavigator::ImageNavigator(QWidget *parent)
: QScrollArea(parent) {
    setWindowFlags(windowFlags() | Qt::Window);
    initialize();
}

ImageNavigator::ImageNavigator(const QString& workDir, mrcImage *sourceImage, QWidget *parent)
: QScrollArea(parent), workingDir(workDir) {
    QTime timer;
    timer.start();
    setWindowFlags(windowFlags() | Qt::Window);

    mainMenuBar = new QMenuBar(this);

    imageHeader = sourceImage->getHeader();
    image = new FullScreenImage(sourceImage, workingDir, this);

    if (imageHeader->mode() == 3 || imageHeader->mode() == 4) imageType = "fft";
    else imageType = "real";

    setWidget(image);
    initialize();

    update();
    center();
    qDebug() << "Load time of ImageNavigator: " << timer.elapsed();
}

ImageNavigator::~ImageNavigator() {
    delete menu;
}

void ImageNavigator::initialize() {
    setWindowFlags(Qt::Window | windowFlags());
    setAttribute(Qt::WA_DeleteOnClose);

    QRect screenRect = QApplication::desktop()->screenGeometry(QApplication::desktop()->primaryScreen());
    screenWidth = screenRect.width();
    screenHeight = screenRect.height();
    // qDebug() << "Screen Dimensions are " << screenWidth << " x " << screenHeight << endl;

    initializeTools();
    initializeActions();

    setBackgroundRole(QPalette::Dark);
    setForegroundRole(QPalette::Dark);
    navOrigin = QPoint(0, 0);
    scrollSpeed = 150;
    scrollBorder = 100;
    horizontalScrollBar()->setRange(0, 200);
    verticalScrollBar()->setRange(0, 200);
    horScroll = 0;
    verScroll = 0;
    imageScale = 1.0;
    //  maximumValueSearchRange=10;
    //  sigma = 3.0;
    //  maxSearchMethod = mrcImage::maximum_value;

    spotSelectMode = false;
    latticeRefinementMode = false;
    createPathMode = false;
    ctfView = false;
    resolutionRingView = false;
    viewDisplayParameters = false;
    fftSelectionMode = false;
    viewport()->setMouseTracking(true);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(openMenu()));

    if (imageType != "fft")
        image->setVisible("realOverlay", true);

    resize(1024, 768);
    showMaximized();
    //showFullScreen();
}

void ImageNavigator::initializeActions() {
    setAttribute(Qt::WA_DeleteOnClose);
    menu = mainMenuBar->addMenu("Focus Viewer");
    QSignalMapper *signalMap = new QSignalMapper(this);

    QAction *showFullScreenAction = new QAction(tr("Show Full Screen"), this);
    showFullScreenAction->setShortcut(tr("Ctrl+F"));
    showFullScreenAction->setCheckable(true);
    connect(showFullScreenAction, SIGNAL(toggled(bool)), this, SLOT(enableFullScreen(bool)));
    menu->addAction(showFullScreenAction);

    toggleInfoToolAction = new QAction(tr("Display Coordinate Info"), this);
    toggleInfoToolAction->setShortcut(tr("I"));
    toggleInfoToolAction->setCheckable(true);
    addAction(toggleInfoToolAction);
    connect(toggleInfoToolAction, SIGNAL(triggered()), this, SLOT(toggleInfoTool()));
    menu->addAction(toggleInfoToolAction);

    // #ifdef Q_OS_MAC
    QMenu *zoomMenu = new QMenu("Zoom", this);
    menu->addMenu(zoomMenu);
    QAction *zoomInAction = new QAction(tr("Zoom In"), this);
    zoomInAction->setShortcut(tr("."));
    addAction(zoomInAction);
    connect(zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));
    zoomMenu->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(tr("Zoom Out"), this);
    zoomOutAction->setShortcut(tr(","));
    addAction(zoomOutAction);
    connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));
    zoomMenu->addAction(zoomOutAction);
    QAction *zoomStandardAction = new QAction(tr("Zoom Standard"), this);
    zoomStandardAction->setShortcut(tr("Space"));
    addAction(zoomStandardAction);
    connect(zoomStandardAction, SIGNAL(triggered()), this, SLOT(zoomStandard()));
    zoomMenu->addAction(zoomStandardAction);
    menu->addMenu(zoomMenu);
    //#endif

    toggleColorToolAction = new QAction(tr("Adjust Contrast/Brightness"), this);
    toggleColorToolAction->setShortcut(tr("O"));
    toggleColorToolAction->setCheckable(true);
    addAction(toggleColorToolAction);
    connect(toggleColorToolAction, SIGNAL(triggered()), this, SLOT(toggleColorTool()));
    menu->addAction(toggleColorToolAction);

    // #ifdef Q_OS_MAC
    // CHEN: 4.1.2015
    // if(imageType =="fft")
    // {
    QMenu *brighterMenu = new QMenu("Quick-Adjust Brightness", this);
    menu->addMenu(brighterMenu);
    QAction *brighterAction = new QAction(tr("Brighter"), this);
    brighterAction->setShortcut(tr("b"));
    addAction(brighterAction);
    connect(brighterAction, SIGNAL(triggered()), this, SLOT(brighter()));
    brighterMenu->addAction(brighterAction);

    QAction *darkerAction = new QAction(tr("Darker"), this);
    darkerAction->setShortcut(tr("n"));
    addAction(darkerAction);
    connect(darkerAction, SIGNAL(triggered()), this, SLOT(darker()));
    brighterMenu->addAction(darkerAction);
    // }
    // #endif

    toggleMouseAssignAction = new QAction(tr("Show Mouse Button Assignment"), this);
    toggleMouseAssignAction->setShortcut(tr("M"));
    toggleMouseAssignAction->setCheckable(true);
    addAction(toggleMouseAssignAction);
    connect(toggleMouseAssignAction, SIGNAL(triggered()), this, SLOT(toggleAssignTool()));
    menu->addAction(toggleMouseAssignAction);

    QAction *screenshot = new QAction(tr("Screen Shot"), this);
    screenshot->setShortcut(tr("G"));
    addAction(screenshot);
    connect(screenshot, SIGNAL(triggered()), image, SLOT(grabScreen()));
    menu->addAction(screenshot);

    int projectMode = projectData.projectMode().toInt();

    if (imageType == "fft") {
        viewDisplayParametersAction = new QAction(tr("Display Parameters"), this);
        viewDisplayParametersAction->setShortcut(tr("D"));
        viewDisplayParametersAction->setCheckable(true);
        addAction(viewDisplayParametersAction);
        connect(viewDisplayParametersAction, SIGNAL(triggered()), this, SLOT(toggleDisplayParameters()));
        menu->addAction(viewDisplayParametersAction);

        toggleCTFViewAction = new QAction(tr("View CTF"), this);
        toggleCTFViewAction->setShortcut(tr("C"));
        // toggleCTFViewAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        toggleCTFViewAction->setCheckable(true);
        addAction(toggleCTFViewAction);
        connect(toggleCTFViewAction, SIGNAL(triggered()), this, SLOT(toggleCTFView()));
        //connect(toggleCTFViewAction,SIGNAL(triggered()),image,SLOT(toggleCTFView()));
        menu->addAction(toggleCTFViewAction);
        
        toggleResolutionRingViewAction = new QAction(tr("View Resolution Ring"), this);
        toggleResolutionRingViewAction->setShortcut(tr("R"));
        // toggleResolutionRingViewAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        toggleResolutionRingViewAction->setCheckable(true);
        addAction(toggleResolutionRingViewAction);
        connect(toggleResolutionRingViewAction, SIGNAL(triggered()), this, SLOT(toggleResolutionRingView()));
        menu->addAction(toggleResolutionRingViewAction);

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            QAction *displayMillerIndicesAction = new QAction(tr("Show Miller Indices"), this);
            displayMillerIndicesAction->setShortcut(tr("Shift+D"));
            displayMillerIndicesAction->setCheckable(true);
            addAction(displayMillerIndicesAction);
            connect(displayMillerIndicesAction, SIGNAL(triggered()), signalMap, SLOT(map()));
            signalMap->setMapping(displayMillerIndicesAction, "millerindices");
            menu->addAction(displayMillerIndicesAction);


            QAction *viewPSPeaksAction = new QAction(tr("View Peak List"), this);
            viewPSPeaksAction->setShortcut(tr("Shift+P"));
            viewPSPeaksAction->setCheckable(true);
            addAction(viewPSPeaksAction);
            connect(viewPSPeaksAction, SIGNAL(triggered()), signalMap, SLOT(map()));
            signalMap->setMapping(viewPSPeaksAction, "pspeaklist");
            connect(signalMap, SIGNAL(mapped(const QString &)), image, SLOT(toggleVisible(const QString &)));
            menu->addAction(viewPSPeaksAction);

            QAction *loadPSPeaksAction = new QAction(tr("Load Peak List"), this);
            loadPSPeaksAction->setShortcut(tr("Shift+L"));
            addAction(loadPSPeaksAction);
            connect(loadPSPeaksAction, SIGNAL(triggered()), this, SLOT(selectPSList()));
            menu->addAction(loadPSPeaksAction);
        }

        QMenu *spotSelection = new QMenu("Spot Selection");
        if (projectMode == 1 || projectMode == 2 || projectMode == 5) menu->addMenu(spotSelection);

        togglePeakListAction = new QAction(tr("Identify Spots"), spotSelection);
        togglePeakListAction->setShortcut(tr("P"));
        togglePeakListAction->setCheckable(true);

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(togglePeakListAction);
            connect(togglePeakListAction, SIGNAL(triggered()), image, SLOT(togglePeakList()));
            spotSelection->addAction(togglePeakListAction);
        }

        enterSpotSelectionModeAction = new QAction(tr("Enter Spot Selection Mode"), spotSelection);
        enterSpotSelectionModeAction->setCheckable(true);
        enterSpotSelectionModeAction->setShortcut(tr("Ctrl+P"));

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(enterSpotSelectionModeAction);
            connect(enterSpotSelectionModeAction, SIGNAL(triggered()), this, SLOT(toggleSpotSelectMode()));
            spotSelection->addAction(enterSpotSelectionModeAction);
        }

        savePeakListAction = new QAction(tr("Save Spot List"), spotSelection);
        savePeakListAction->setShortcut(tr("Ctrl+Shift+S"));

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(savePeakListAction);
            savePeakListAction->setDisabled(true);
            connect(savePeakListAction, SIGNAL(triggered()), image, SLOT(savePeakList()));
            spotSelection->addAction(savePeakListAction);
        }

        loadPeakListAction = new QAction(tr("Reload Spot List"), spotSelection);
        loadPeakListAction->setShortcut(tr("Ctrl+R"));

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(loadPeakListAction);
            loadPeakListAction->setDisabled(true);
            connect(loadPeakListAction, SIGNAL(triggered()), image, SLOT(loadPeakList()));
            connect(loadPeakListAction, SIGNAL(triggered()), image, SLOT(update()));
            spotSelection->addAction(loadPeakListAction);
        }

        clearPeakListAction = new QAction(tr("Clear Spot List"), spotSelection);
        clearPeakListAction->setShortcut(tr("Ctrl+Shift+C"));

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(clearPeakListAction);
            clearPeakListAction->setDisabled(true);
            connect(clearPeakListAction, SIGNAL(triggered()), image, SLOT(clearPeakList()));
            spotSelection->addAction(clearPeakListAction);
        }

        QMenu *latticeRefinement = new QMenu("Lattice Refinement");
        if (projectMode == 1 || projectMode == 2 || projectMode == 5) menu->addMenu(latticeRefinement);

        toggleLatticeViewAction = new QAction(tr("View Lattice"), latticeRefinement);
        toggleLatticeViewAction->setShortcut(tr("L"));
        toggleLatticeViewAction->setCheckable(true);
        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(toggleLatticeViewAction);
            connect(toggleLatticeViewAction, SIGNAL(triggered()), image, SLOT(toggleLatticeView()));
            latticeRefinement->addAction(toggleLatticeViewAction);


            QAction *toggleSecondLatticeViewAction = new QAction(tr("View Second Lattice"), latticeRefinement);
            toggleSecondLatticeViewAction->setShortcut(tr("S"));
            toggleSecondLatticeViewAction->setCheckable(true);
            addAction(toggleSecondLatticeViewAction);
            connect(toggleSecondLatticeViewAction, SIGNAL(triggered()), image, SLOT(toggleSecondLatticeView()));
            latticeRefinement->addAction(toggleSecondLatticeViewAction);
        }

        enterLatticeRefinementModeAction = new QAction(tr("Enter Lattice Refinement Mode"), latticeRefinement);
        enterLatticeRefinementModeAction->setShortcut(tr("Shift+R"));
        enterLatticeRefinementModeAction->setCheckable(true);

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(enterLatticeRefinementModeAction);
            connect(enterLatticeRefinementModeAction, SIGNAL(triggered()), this, SLOT(toggleLatticeRefinementMode()));
            latticeRefinement->addAction(enterLatticeRefinementModeAction);
        }

        addRefinementPointAction = new QAction(tr("Add Refinement Spot"), latticeRefinement);
        addRefinementPointAction->setShortcuts(QList<QKeySequence>() << tr("Enter") << tr("Return"));
        addRefinementPointAction->setEnabled(false);

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(addRefinementPointAction);
            connect(addRefinementPointAction, SIGNAL(triggered()), latticeTool, SLOT(insertPoint()));
            latticeRefinement->addAction(addRefinementPointAction);
        }

    } else {

        toggleLatticeViewAction = new QAction(tr("View Lattice"), this);
        toggleLatticeViewAction->setShortcut(tr("L"));
        toggleLatticeViewAction->setCheckable(true);

        if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
            addAction(toggleLatticeViewAction);
            menu->addAction(toggleLatticeViewAction);
            connect(toggleLatticeViewAction, SIGNAL(triggered()), signalMap, SLOT(map()));
            signalMap->setMapping(toggleLatticeViewAction, "realLattice");
            connect(signalMap, SIGNAL(mapped(const QString &)), image, SLOT(toggleVisible(const QString &)));
        }

        toggleParticlesViewAction = new QAction(tr("View Particles"), this);
        toggleParticlesViewAction->setShortcut(tr("P"));
        toggleParticlesViewAction->setCheckable(true);

        if (projectMode == 2 || projectMode == 5) {
            addAction(toggleParticlesViewAction);
            menu->addAction(toggleParticlesViewAction);
            connect(toggleParticlesViewAction, SIGNAL(triggered()), image, SLOT(toggleParticleView()));
        }

        QMenu *fftSelectionMenu = new QMenu("Selection based FFT");

        QAction *fftSelectionAction = new QAction(tr("FFT of Selection"), this);
        fftSelectionMenu->addAction(fftSelectionAction);
        fftSelectionAction->setShortcut(tr("Shift+F"));
        fftSelectionAction->setCheckable(true);
        addAction(fftSelectionAction);
        connect(fftSelectionAction, SIGNAL(triggered()), this, SLOT(toggleFFTSelection()));

        if (projectMode == 1) {
            QAction *setReferenceOriginAction = new QAction(tr("Set Reference Origin"), this);
            fftSelectionMenu->addAction(setReferenceOriginAction);
            setReferenceOriginAction->setShortcut(tr("Shift+O"));
            addAction(setReferenceOriginAction);
            connect(setReferenceOriginAction, SIGNAL(triggered()), this, SLOT(setReferenceOrigin()));
            connect(setReferenceOriginAction, SIGNAL(triggered()), spotSelect, SLOT(updateReferenceOrigin()));
        }

        menu->addMenu(fftSelectionMenu);

        selectionMenu = new QMenu("Polygonal Selection");

        if (projectMode == 1 || projectMode == 5) {
            QAction *selectionAreaAction = new QAction(tr("Polygonal Selection Masking"), this);
            selectionMenu->addAction(selectionAreaAction);
            selectionAreaAction->setShortcut(tr("Shift+S"));
            selectionAreaAction->setCheckable(true);
            addAction(selectionAreaAction);
            connect(selectionAreaAction, SIGNAL(triggered()), this, SLOT(toggleCreatePathMode()));


            QAction *saveSelectionArea = new QAction(tr("Save Selection"), this);
            selectionMenu->addAction(saveSelectionArea);
            saveSelectionArea->setShortcut(tr("Ctrl+Shift+S"));
            addAction(saveSelectionArea);
            connect(saveSelectionArea, SIGNAL(triggered()), image, SLOT(saveSelectionList()));

            QAction *clearSelectionArea = new QAction(tr("Clear Selection"), this);
            selectionMenu->addAction(clearSelectionArea);
            clearSelectionArea->setShortcut(tr("Ctrl+Shift+C"));
            addAction(clearSelectionArea);
            connect(clearSelectionArea, SIGNAL(triggered()), image, SLOT(clearSelectionVertices()));

            menu->addMenu(selectionMenu);


            QMenu *referenceMenu = new QMenu("Unbending References", menu);

            QAction *toggleBoxa1Action = new QAction(tr("View Boxa1"), this);
            toggleBoxa1Action->setCheckable(true);
            referenceMenu->addAction(toggleBoxa1Action);
            addAction(toggleBoxa1Action);
            connect(toggleBoxa1Action, SIGNAL(triggered()), this, SLOT(toggleBoxa1()));

            QAction *toggleBoxa2Action = new QAction(tr("View Boxa2"), this);
            toggleBoxa2Action->setCheckable(true);
            referenceMenu->addAction(toggleBoxa2Action);
            addAction(toggleBoxa2Action);
            connect(toggleBoxa2Action, SIGNAL(triggered()), this, SLOT(toggleBoxa2()));

            QAction *toggleBoxb1Action = new QAction(tr("View Boxb1"), this);
            toggleBoxb1Action->setCheckable(true);
            referenceMenu->addAction(toggleBoxb1Action);
            addAction(toggleBoxb1Action);
            connect(toggleBoxb1Action, SIGNAL(triggered()), this, SLOT(toggleBoxb1()));

            QAction *toggleBoxb2Action = new QAction(tr("View Boxb2"), this);
            toggleBoxb2Action->setCheckable(true);
            referenceMenu->addAction(toggleBoxb2Action);
            addAction(toggleBoxb2Action);
            connect(toggleBoxb2Action, SIGNAL(triggered()), this, SLOT(toggleBoxb2()));

            menu->addMenu(referenceMenu);

            QAction *setPhaseOriginAction = new QAction(tr("Set Phase Origin"), this);
            menu->addAction(setPhaseOriginAction);
            setPhaseOriginAction->setShortcut(tr("Shift+P"));
            addAction(setPhaseOriginAction);
            connect(setPhaseOriginAction, SIGNAL(triggered()), this, SLOT(setPhaseOrigin()));
        }

    }

    if (projectMode == 1 || projectMode == 2 || projectMode == 5) {
        QAction *showTiltAxisAction = new QAction(tr("View Tilt Axis in Raw Image (TLTAXIS)"), this);
        menu->addAction(showTiltAxisAction);
        showTiltAxisAction->setCheckable(true);
        showTiltAxisAction->setShortcut(tr("T"));
        addAction(showTiltAxisAction);
        connect(showTiltAxisAction, SIGNAL(triggered()), signalMap, SLOT(map()));
        signalMap->setMapping(showTiltAxisAction, "tiltaxis");
        connect(signalMap, SIGNAL(mapped(const QString &)), image, SLOT(toggleVisible(const QString &)));

        QAction *showTaxisAction = new QAction(tr("View Tilt Axis in Final Map (TAXA)"), this);
        menu->addAction(showTaxisAction);
        showTaxisAction->setCheckable(true);
        showTaxisAction->setShortcut(tr("Shift+T"));
        addAction(showTaxisAction);
        connect(showTaxisAction, SIGNAL(triggered()), signalMap, SLOT(map()));
        signalMap->setMapping(showTaxisAction, "tiltaxa");
        connect(signalMap, SIGNAL(mapped(const QString &)), image, SLOT(toggleVisible(const QString &)));
    }


    QAction *helpAction = new QAction(tr("Help"), this);
    helpAction->setShortcut(tr("H"));
    helpAction->setCheckable(true);
    addAction(helpAction);
    connect(helpAction, SIGNAL(triggered()), this, SLOT(toggleHelp()));
    menu->addAction(helpAction);

    closeAction = new QAction(tr("Close"), this);
    closeAction->setShortcut(tr("Esc"));
    addAction(closeAction);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(closeCurrent()));
    menu->addAction(closeAction);

    //  menuBar->addMenu(menu);
}

void ImageNavigator::initializeTools() {
    QWidget *mainWin = this; //(QMainWindow*)parent();

    if (imageType == "fft") {
        latticeTool = new LatticeRefineTool(workingDir, image, mainWin);
#ifdef Q_OS_MAC
        latticeTool->showNormal();
#endif
        latticeTool->raise();
        latticeTool->move(screenWidth - latticeTool->width() - 20, screenHeight / 2);
        // qDebug() << "latticeTool window placed at " << screenWidth - latticeTool->width() - 20 << "," << screenHeight / 2 << endl;
        latticeTool->hide();

        ctfEditor = new CtfTool(workingDir, image, mainWin);
#ifdef Q_OS_MAC
        ctfEditor->showNormal();
#endif
        ctfEditor->raise();
        QPoint imageSize = QPoint(geometry().width(), geometry().height());
        QPoint relativePos = imageSize - QPoint(20, 35);
        QPoint absolutPos = mapToGlobal(relativePos);
        ctfEditor->move(absolutPos);
        //ctfEditor->move(screenWidth-ctfEditor->width()-20,screenHeight-ctfEditor->height()-35);
        // qDebug() << "ctfEditor window placed at " << absolutPos.x() << "," << absolutPos.y() << endl;
        ctfEditor->hide();
        connect(ctfEditor, SIGNAL(defocusChanged(float, float, float)), image, SLOT(calculateCTF(float, float, float)));
        
        resolutionRingTool = new ResolutionRingTool(image, mainWin);
#ifdef Q_OS_MAC
        resolutionRingTool->showNormal();
#endif
        resolutionRingTool->raise();
        resolutionRingTool->move(absolutPos);
        resolutionRingTool->hide();

        parameterEditor = new DisplayParametersTool(mainWin);
#ifdef Q_OS_MAC
        parameterEditor->showNormal();
#endif
        parameterEditor->move(0, screenHeight - parameterEditor->height() - 35);
        parameterEditor->hide();
        connect(parameterEditor, SIGNAL(latticeSizeChanged(int)), image, SLOT(setLatticeEllipseSize(int)));
        connect(parameterEditor, SIGNAL(latticeCircleLineChanged(int)), image, SLOT(setLatticeEllipseLineThickness(int)));
        connect(parameterEditor, SIGNAL(latticeOrdersChanged(int)), image, SLOT(setLatticeOrders(int)));

        connect(parameterEditor, SIGNAL(fontSizeChanged(int)), image, SLOT(setFontSize(int)));

        connect(parameterEditor, SIGNAL(spotSizeChanged(int)), image, SLOT(setPeakEllipseSize(int)));
        connect(parameterEditor, SIGNAL(refinementSizeChanged(int)), image, SLOT(setRefinementEllipseSize(int)));
        connect(parameterEditor, SIGNAL(searchRangeChanged(int)), this, SLOT(setMaximumValueSearchRange(int)));
        connect(parameterEditor, SIGNAL(sigmaChanged(double)), this, SLOT(setSigma(double)));
        connect(parameterEditor, SIGNAL(searchRangeChanged(int)), image, SLOT(setMaxFitRange(int)));
        connect(parameterEditor, SIGNAL(sigmaChanged(double)), image, SLOT(setSigma(double)));
        connect(parameterEditor, SIGNAL(viewFitChanged(bool)), this, SLOT(setFitVisible(bool)));
        connect(parameterEditor, SIGNAL(searchMethodChanged(mrcImage::maxValueMethod)), this, SLOT(setMaxSearchMethod(mrcImage::maxValueMethod)));
        connect(parameterEditor, SIGNAL(searchMethodChanged(mrcImage::maxValueMethod)), image, SLOT(setMaxSearchMethod(mrcImage::maxValueMethod)));
        parameterEditor->setDefaults();
        parameterEditor->flush();
    } else {
        selectionFFTTool = new SelectionFFT(mainWin);
        selectionFFTTool->move(0, 0);
        selectionFFTTool->hide();

        selectionArea = new QRubberBand(QRubberBand::Rectangle, this);
        selectionArea->setFocusProxy(this);
    }

    spotSelect = new SpotSelectTool(workingDir, image, image->getSourceImage(), QPoint(image->width(), image->height()), mainWin);
#ifdef Q_OS_MAC
    spotSelect->showNormal();
#endif
    spotSelect->move((screenWidth - spotSelect->width()) / 2, screenHeight - spotSelect->height() - 35);
    // qDebug() << "spotSelect (Coordinate Info) window placed at " << (screenWidth - spotSelect->width()) / 2 << "," << screenHeight - spotSelect->height() - 35 << endl;
    spotSelect->hide();

    zoomWin = new ZoomWindow(image, mainWin);
    zoomWin->resize(256, 256);
#ifdef Q_OS_MAC
    zoomWin->showNormal();
#endif
    zoomWin->hide();
    connect(image, SIGNAL(painted()), zoomWin, SLOT(zoom()));
    connect(zoomWin, SIGNAL(zoomClick(const QPoint&)), this, SLOT(zoomClick(const QPoint&)));
    connect(zoomWin, SIGNAL(zoomDoubleClick(const QPoint&)), this, SLOT(zoomDoubleClick(const QPoint&)));
    connect(zoomWin, SIGNAL(zoomMove(const QPoint &)), this, SLOT(zoomMove(const QPoint&)));

    colorLookupTool = new ColorTool(image, imageHeader, mainWin);
#ifdef Q_OS_MAC
    colorLookupTool->showNormal();
#endif
    colorLookupTool->move(screenWidth - colorLookupTool->width() - 20, 20);
    // qDebug() << "colorLookupTool window placed at " << screenWidth - colorLookupTool->width() - 20 << "," << 20 << endl;
    colorLookupTool->hide();

    PhaseView *phaseTool = new PhaseView(mainWin);
#ifdef Q_OS_MAC
    phaseTool->showNormal();
#endif
    phaseTool->move(screenWidth - phaseTool->width() - 20, colorLookupTool->y() + colorLookupTool->height() + 20);
    // qDebug() << "phaseTool window placed at " << screenWidth - phaseTool->width() - 20 << "," << colorLookupTool->y() + colorLookupTool->height() + 20 << endl;
    phaseTool->hide();
    connect(spotSelect, SIGNAL(phaseChanged(float)), phaseTool, SLOT(setPhase(float)));
    connect(colorLookupTool, SIGNAL(togglePhase(bool)), phaseTool, SLOT(show(bool)));
    connect(colorLookupTool, SIGNAL(toggleInvert(bool)), phaseTool, SLOT(invert(bool)));

    mouseAssign = new MouseAssignTool(this);
    mouseAssign->move((screenWidth - mouseAssign->width() - 35) / 2, 20);
    // qDebug() << "mouseAssign window placed at " << (screenWidth - mouseAssign->width() - 35) / 2 << "," << 20 << endl;
    mouseAssign->hide();
    setMouseDefaults();

    helpTool = new NavigatorHelpTool(ApplicationData::configDir().absolutePath() + "/navigator_help.htm", this);
    helpTool->hide();
}

void ImageNavigator::mousePressEvent(QMouseEvent *event) {
    if (event->modifiers() != Qt::MetaModifier) {
        zoomWin->hide();
    }

    if (event->button() == Qt::RightButton) {
        if (event->modifiers() != Qt::ControlModifier) {
            if (spotSelectMode || latticeRefinementMode) {
                showZoomWindow(event->pos());
            } else if (createPathMode) {
                selectionMenu->exec(event->pos());
            } else if (fftSelectionMode) {
                selectionArea->show();
                selectionArea->move(event->pos() - QPoint(selectionArea->width(), selectionArea->height()) / 2);
                showFFTSelection(selectionArea->frameGeometry(), false);
                selectionFFTTool->move(0, 0);
            } else
                menu->exec(event->pos());
        } else
            menu->exec(event->pos());
    }

    if (event->button() == Qt::MidButton || (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::AltModifier)) {
        navOrigin = event->pos();
    }

    if (event->button() == Qt::LeftButton && event->modifiers() != Qt::AltModifier) {
        if (event->modifiers() == Qt::ControlModifier) {
            menu->exec(event->pos());
        } else if (fftSelectionMode) {
            if (event->modifiers() != Qt::ShiftModifier) {
                selectionOrigin = event->pos();
                selectionArea->setGeometry(QRect(event->pos(), QSize()));
                selectionArea->show();
            } else {
                selectionArea->show();
                selectionArea->move(event->pos() - QPoint(selectionArea->width(), selectionArea->height()) / 2);
                showFFTSelection(selectionArea->frameGeometry(), false);
                selectionFFTTool->move(0, 0);
            }
        } else if (createPathMode) {
            QPoint point = image->mapFrom(this, event->pos());
            point -= QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
            image->addSelectionVertex(point);
        } else if (spotSelectMode)
            toggleSpot(event->pos());
        else if (latticeRefinementMode)
            moveLatticePoint(event->pos());
        else
            showZoomWindow(event->pos());
    }
}

void ImageNavigator::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MidButton || (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier)) {
        //    navOrigin = QPoint(-1,-1);
    } else if (event->button() == Qt::LeftButton && fftSelectionMode && event->modifiers() != Qt::ShiftModifier) {
        selectionArea->hide();
        showFFTSelection(selectionArea->frameGeometry());
    } else if ((event->button() == Qt::RightButton || (event->button() == Qt::LeftButton && event->modifiers() == Qt::ShiftModifier)) && fftSelectionMode) {
        showFFTSelection(selectionArea->frameGeometry(), false);
        selectionFFTTool->move(0, 0);
    }
}

void ImageNavigator::mouseMoveEvent(QMouseEvent *event) {

    QPoint point = image->mapFrom(this, event->pos());
    currentMousePos = point;
    image->setCurrentMousePos(point);

#ifdef Q_OS_MAC
    point -= QPoint(image->width() / 2, image->height() / 2);
#else
    point -= QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
#endif


    if (event->buttons() == Qt::MidButton || (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::AltModifier)) {
        QPoint delta = event->pos() - navOrigin;
        float x = (float) delta.x() / (float) image->width(), y = (float) delta.y() / (float) image->height();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - int(x * (float) horizontalScrollBar()->maximum()));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - int(y * (float) verticalScrollBar()->maximum()));
        navOrigin = event->pos();
        update();
    } else if (fftSelectionMode && event->buttons() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
        QRect view = QRect(selectionOrigin, event->pos()).normalized();
        if (view.width() > view.height()) {
            view.setHeight(view.width());
            view.setSize(view.size() / 4 * 4);
        } else {
            view.setWidth(view.height());
            view.setSize(view.size() / 4 * 4);
        }

        selectionArea->setGeometry(view);
    } else if (fftSelectionMode && (event->buttons() == Qt::RightButton || (event->buttons() == Qt::LeftButton && (event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::MetaModifier)))) {
        selectionArea->show();
        selectionArea->move(event->pos() - QPoint(selectionArea->width(), selectionArea->height()) / 2);
        showFFTSelection(selectionArea->frameGeometry(), false);
        selectionFFTTool->move(0, 0);
    } else if (((event->buttons() == Qt::LeftButton && !(spotSelectMode || latticeRefinementMode)) || ((spotSelectMode || latticeRefinementMode) && event->buttons() == Qt::RightButton)) && event->modifiers() == Qt::NoModifier && !createPathMode) {
        //zoomWin->show();
        //zoomWin->move(event->pos() - QPoint(zoomWin->width()/2, zoomWin->height()/2));
        //zoomWin->zoom(point/image->imageScale());//image->mapFrom(this,event->pos()));
        showZoomWindow(event->pos());
    }

    point.setY(-point.y());

    spotSelect->updateIndices(point / image->imageScale());
}

void ImageNavigator::mouseDoubleClickEvent(QMouseEvent *event) {
    if (createPathMode) {
        if (!image->selectionVertexList().isEmpty()) {
            QPoint point = image->rawSelectionVertexList()[0];
            image->addSelectionVertex(point);
        }
        return;
    }
    QPoint pos = image->mapFrom(this, event->pos());
    float scale = image->imageScale();
    //float nx = image->getSourceImage()->getHeader()->nx();
    float ny = image->getSourceImage()->getHeader()->ny();
    bool flipped = false;
    if (imageType == "fft") {
#ifdef Q_OS_MAC
        pos -= QPoint(image->width() / 2, image->height() / 2);
#else
        pos -= QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
#endif
        pos.setY(-pos.y());
        if (pos.x() < 0) {
            pos *= -1;
            flipped = true;
        }
        pos /= scale;
    }


    pos = image->getSourceImage()->maxValue(pos + QPoint(0, ny / 2), maximumValueSearchRange, maxSearchMethod, sigma);


    if (imageType == "fft") {
        pos -= QPoint(0, ny / 2);
        if (flipped) pos *= -1;
        pos *= scale;
        pos.setY(-pos.y());
#ifdef Q_OS_MAC
        pos += QPoint(image->width() / 2, image->height() / 2);
#else
        pos += QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
#endif
    }
    //pos.setY(image->height()-pos.y());
    pos = image->mapTo(this, pos);
    QMouseEvent e(event->type(), pos, event->button(), event->buttons(), event->modifiers());
    mousePressEvent(&e);
}

void ImageNavigator::keyPressEvent(QKeyEvent *event) {
    if (fftSelectionMode) {
        if (event->text() == "=") selectionFFTTool->increaseZoom();
        if (event->text() == "-") selectionFFTTool->decreaseZoom();

        if (event->text() == "1") selectionFFTTool->setBrightness(1.0);
        if (event->text() == "2") selectionFFTTool->setBrightness(2.0);
        if (event->text() == "3") selectionFFTTool->setBrightness(3.0);
        if (event->text() == "4") selectionFFTTool->setBrightness(4.0);
        if (event->text() == "5") selectionFFTTool->setBrightness(5.0);
        if (event->text() == "6") selectionFFTTool->setBrightness(6.0);
        if (event->text() == "7") selectionFFTTool->setBrightness(7.0);
        if (event->text() == "8") selectionFFTTool->setBrightness(8.0);
        if (event->text() == "9") selectionFFTTool->setBrightness(9.0);

        if (event->text() == "0") selectionFFTTool->zoomStandard();
    } else {
        if (event->text() == "=") zoomIn();
        if (event->text() == "-") zoomOut();
    }
    QWidget::keyPressEvent(event);
}

void ImageNavigator::setType(const QString &type) {
    imageType = type;
}

void ImageNavigator::center() {
    horizontalScrollBar()->setValue((horizontalScrollBar()->maximum() - horizontalScrollBar()->minimum()) / 2);
    verticalScrollBar()->setValue((verticalScrollBar()->maximum() - verticalScrollBar()->minimum()) / 2);
    //  ensureVisible(image->width()/2,image->height()/2,screenWidth/2,screenHeight/2);
}

void ImageNavigator::closeCurrent() {
    if (image->checkSaved()) {
        releaseKeyboard();
        releaseMouse();
        setMouseTracking(false);
        showNormal();
        emit closed();
        close();
        deleteLater();
    }
}

void ImageNavigator::showZoomWindow(const QPoint &pos, const QSize &size) {
    QPoint point = image->mapFrom(this, pos);

#ifdef Q_OS_MAC
    point -= QPoint(image->width() / 2, image->height() / 2);
#else
    point -= QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
#endif
    //QPoint relativPosition = QPoint(image->geometry().x()+pos.x(),image->geometry().y()+pos.y());
    int sizex = size.width(), sizey = size.height();

    zoomWin->move(mapToGlobal(pos) - QPoint(sizex / 2, sizey / 2));
    zoomWin->zoom(image->mapFrom(this, pos) / 2 * 2);
    zoomWin->show();
}

void ImageNavigator::toggleSpot(const QPoint &pos) {
    QPoint selection = image->mapFrom(this, pos);
    selection -= QPoint(image->width() / 2 - 1, image->height() / 2 - 1);
    selection.setX(int(selection.x() / image->imageScale()));
    selection.setY(int(-selection.y() / image->imageScale()));

    image->toggleSpot(selection);
}

void ImageNavigator::moveLatticePoint(const QPoint &pos) {

    QPointF point = image->mapFrom(this, pos);
#ifdef Q_OS_MAC
    point -= QPointF((float) image->width() / 2.0, (float) image->height() / 2.0);
#else
    point -= QPointF(image->width() / 2.0 - 1.0, image->height() / 2.0 - 1.0);
#endif
    point.setX(point.x() / image->imageScale());
    point.setY(-point.y() / image->imageScale());

    latticeTool->updatePoint(point); //QPoint(point.x(),point.y()));
}

void ImageNavigator::toggleLatticeRefinementMode() {
    if (imageType == "fft") {
        latticeRefinementMode ^= 1;
        image->setRefineLatticeView(latticeRefinementMode);

        if (latticeRefinementMode) {
            latticeTool->show();
            assignMouseButtons("Select", "Move", "Zoom");
            addRefinementPointAction->setEnabled(true);
        } else {
            latticeTool->hide();
            setMouseDefaults();
            addRefinementPointAction->setEnabled(false);
        }
    }
    image->update();
}

void ImageNavigator::toggleSpotSelectMode() {
    if (imageType == "fft") {
        spotSelectMode = spotSelectMode^1;
        if (spotSelectMode) {
            assignMouseButtons("Select", "Move", "Zoom");
            image->setLatticeView(true);
            image->setPeakListView(true);
            togglePeakListAction->setChecked(true);
            toggleLatticeViewAction->setChecked(true);
            savePeakListAction->setEnabled(true);
            loadPeakListAction->setEnabled(true);
            clearPeakListAction->setEnabled(true);
        } else {
            setMouseDefaults();
            savePeakListAction->setDisabled(true);
            loadPeakListAction->setDisabled(true);
            clearPeakListAction->setDisabled(true);
            image->setLatticeView(false);
            image->setPeakListView(false);
            togglePeakListAction->setChecked(false);
            toggleLatticeViewAction->setChecked(false);
        }

        image->update();
    }
}

void ImageNavigator::toggleCTFView() {
    if (imageType == "fft") {
        ctfView ^= 1;
        image->setCTFView(ctfView);

        if (ctfView) {
            ctfEditor->showNormal();
            toggleCTFViewAction->setEnabled(true);
            toggleCTFViewAction->setChecked(true);
        } else {
            ctfEditor->hide();
            toggleCTFViewAction->setChecked(false);
        }
        image->update();
    }
}

void ImageNavigator::toggleResolutionRingView() {
    if (imageType == "fft") {
        resolutionRingView ^= 1;
        image->setResolutionRingView(resolutionRingView);

        if (resolutionRingView) {
            resolutionRingTool->showNormal();
            toggleResolutionRingViewAction->setEnabled(true);
            toggleResolutionRingViewAction->setChecked(true);
        } else {
            resolutionRingTool->hide();
            toggleResolutionRingViewAction->setChecked(false);
        }
        image->update();
    }
}


void ImageNavigator::zoomClick(const QPoint &pos) {
    QPoint p = image->mapTo(this, pos);
#ifdef Q_OS_MAC
    p += QPoint(1, 1);
#else
    p += QPoint(-1, -1);
#endif
    QMouseEvent event(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    mousePressEvent(&event);
}

void ImageNavigator::zoomDoubleClick(const QPoint &pos) {
    QPoint p = image->mapTo(this, pos);
#ifdef Q_OS_MAC
    p += QPoint(1, 1);
#else
    p += QPoint(-1, -1);
#endif
    QMouseEvent event(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    mouseDoubleClickEvent(&event);
}

void ImageNavigator::zoomMove(const QPoint &position) {
    QPoint p = image->mapTo(this, position);
#ifdef Q_OS_MAC
    p += QPoint(1, 1);
#else
    p += QPoint(-1, -1);
#endif
    QMouseEvent event(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    mouseMoveEvent(&event);
}

void ImageNavigator::toggleTool(QWidget *widget) {
    if (widget->isHidden()) widget->showNormal();
    else widget->hide();
}

void ImageNavigator::toggleAction(QWidget *widget, QAction *action) {
    if (widget == NULL) {
        qDebug() << "Widget is null and should not be!" << endl;
        return;
    }
    if (widget->isHidden()) action->setChecked(false);
    else action->setChecked(true);
}

void ImageNavigator::toggleDisplayParameters() {
    toggleTool(parameterEditor);
}

void ImageNavigator::toggleInfoTool() {
    toggleTool(spotSelect);
}

void ImageNavigator::toggleColorTool() {
    toggleTool(colorLookupTool);
}

void ImageNavigator::toggleAssignTool() {
    toggleTool(mouseAssign);
}

void ImageNavigator::toggleFFTSelection() {
    fftSelectionMode ^= true;
    if (fftSelectionMode) {
        selectionFFTTool->show();
        assignMouseButtons("Define Selection", "Move", "Select");
    } else if (!fftSelectionMode) {
        selectionFFTTool->hide();
        selectionArea->hide();
        setMouseDefaults();
    }
}

void ImageNavigator::openMenu() {
    if (imageType == "fft") {
        toggleAction(parameterEditor, viewDisplayParametersAction);
    }

    toggleAction(spotSelect, toggleInfoToolAction);
    toggleAction(colorLookupTool, toggleColorToolAction);
}

void ImageNavigator::zoomIn() {
    image->zoomIn();
    QMouseEvent event(QEvent::MouseButtonPress, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    center();
    mouseMoveEvent(&event);
}

void ImageNavigator::zoomOut() {
    image->zoomOut();
    QMouseEvent event(QEvent::MouseButtonPress, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    center();
    mouseMoveEvent(&event);
}

void ImageNavigator::zoomStandard() {
    image->zoomStandard();
    QMouseEvent event(QEvent::MouseButtonPress, QCursor::pos(), Qt::LeftButton, Qt::LeftButton, Qt::MetaModifier);
    center();
    mouseMoveEvent(&event);
}

void ImageNavigator::brighter() {
    // CHEN: 4.1.2015
    if (imageType == "fft") {
        float max = image->imageMax() / 1.4142135;
        float min = image->imageMin();
        image->rescale(min, max, false);
    } else {
        double max = image->imageMax() + (image->imageMax() - image->imageMin()) * 0.1;
        double min = image->imageMin() - (image->imageMax() - image->imageMin()) * 0.1;
        image->rescale(min, max, false);
    }
}

void ImageNavigator::darker() {
    // CHEN: 4.1.2015
    if (imageType == "fft") {
        float max = image->imageMax()*1.4142135;
        float min = image->imageMin();
        image->rescale(min, max, false);
    } else {
        double max = image->imageMax() - (image->imageMax() - image->imageMin()) * 0.1;
        double min = image->imageMin() + (image->imageMax() - image->imageMin()) * 0.1;
        image->rescale(min, max, false);
    }
}

void ImageNavigator::setMouseDefaults() {
    mouseAssign->assignButton(0, "Zoom");
    mouseAssign->assignButton(1, "Move");
    mouseAssign->assignButton(2, "Menu");
}

void ImageNavigator::assignMouseButtons(const QString &left, const QString &middle, const QString &right) {
    mouseAssign->assignButton(0, left);
    mouseAssign->assignButton(1, middle);
    mouseAssign->assignButton(2, right);
}

void ImageNavigator::showFFTSelection(const QRect & view, bool reposition) {
    if (abs(view.width()) > 0 && abs(view.height()) > 0) {
        selectionFFTTool->show();
        QPoint fftOrigin = image->mapFrom(this, view.topLeft());
        selectionFFTTool->fft(image, QRect(fftOrigin, view.size()));
        if (reposition) selectionFFTTool->move(view.topLeft());
    }
}

void ImageNavigator::setReferenceOrigin() {
    QPointF origin;

    if (selectionArea->isVisible()) {
        origin = image->mapFrom(this, selectionArea->frameGeometry().center());
        origin -= QPoint(image->width() / 2, image->height() / 2);
        origin.setX(int(origin.x() / image->imageScale()));
        origin.setY(int(-origin.y() / image->imageScale()));
        origin += QPoint(image->getImage()->width() / 2, image->getImage()->height() / 2);
    } else
        origin = spotSelect->getImageCoordinates();

    projectData.parameterData(QDir(workingDir))->set("refori", QString::number(origin.x()) + "," + QString::number(origin.y()));
    image->update();
}

void ImageNavigator::setPhaseOrigin() {
    /*  QPoint origin = image->mapFrom(this,currentMousePos);
      origin -= QPoint(image->width()/2,image->height()/2);
      origin.setX(int(origin.x()/image->imageScale()));
      origin.setY(int(-origin.y()/image->imageScale()));
      origin += QPoint(image->getImage()->width()/2,image->getImage()->height()/2);
     */
    QPointF origin = spotSelect->getImageCoordinates();

    // qDebug()<<origin.x()<<" "<<origin.y();
    origin.setX(origin.x()*360.0 / (image->getImage()->width() / 2) - 360.0);
    origin.setY(origin.y()*360.0 / (image->getImage()->height() / 2) - 360.0);
    projectData.parameterData(QDir(workingDir))->set("phaori_change", QString::number(-origin.x()) + "," + QString::number(-origin.y()));
    image->update();
    qDebug() << "image->width,height = " << image->width() << "," << image->height() << endl;
    qDebug() << "image->getImage->width,height = " << image->getImage()->width() << "," << image->getImage()->height() << endl;
}

void ImageNavigator::toggleCreatePathMode() {
    createPathMode ^= true;
    image->setPathView(createPathMode);
    image->update();
    if (createPathMode)
        assignMouseButtons("Select", "Move", "Selection Menu");
    else
        setMouseDefaults();
}

void ImageNavigator::toggleHelp() {
    toggleTool(helpTool);
}

void ImageNavigator::toggleTiltAxis() {
    image->setVisible("tiltAxis", image->isVisible("tiltAxis")^true);
}

void ImageNavigator::toggleBoxa1() {
    image->setVisible("boxa1", image->isVisible("boxa1")^true);
}

void ImageNavigator::toggleBoxa2() {
    image->setVisible("boxa2", image->isVisible("boxa2")^true);
}

void ImageNavigator::toggleBoxb1() {
    image->setVisible("boxb1", image->isVisible("boxb1")^true);
}

void ImageNavigator::toggleBoxb2() {
    image->setVisible("boxb2", image->isVisible("boxb2")^true);
}

void ImageNavigator::setFitVisible(bool visible) {
    image->setVisible("maximumvaluefit", visible);
}

void ImageNavigator::selectPSList() {
    QString selection = QFileDialog::getOpenFileName(this, "Select a comma separated peak file", workingDir);
    if (QFileInfo(selection).exists())
        image->setPSPeakListFile(selection);
}

void ImageNavigator::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
    center();
}

void ImageNavigator::setMaximumValueSearchRange(int range) {
    maximumValueSearchRange = range;
}

void ImageNavigator::setSigma(double s) {
    sigma = s;
}

void ImageNavigator::setMaxSearchMethod(mrcImage::maxValueMethod method) {
    maxSearchMethod = method;
}

void ImageNavigator::closeEvent(QCloseEvent *event) {
    closeCurrent();
}

void ImageNavigator::enableFullScreen(bool enable) {
    if (enable)
        showFullScreen();
    else {
        //    setParent(savedParent);
        setWindowFlags(windowFlags() | Qt::Window);
        showNormal();
    }
}


