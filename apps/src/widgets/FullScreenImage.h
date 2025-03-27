#ifndef FULLSCREENIMAGE_H
#define FULLSCREENIMAGE_H

#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QTextStream>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QPixmap>
#include <QVector>
#include <QSet>
#include <QPoint>
#include <QMessageBox>
#include <QDebug>
#include <QGridLayout>
#include <math.h>

#include "PointHash.h"
#include "mrcImage.h"

class FullScreenImage : public QWidget {
    
    Q_OBJECT

public:
    FullScreenImage(mrcImage *source_image, QString imPath, QWidget *parent = NULL);
    void setImage(QImage *image);
    void setLatticeRefinementList(QMap<QPoint, QPoint> &list);
    void setRefinementCandidate(QPointF &candidate);
    QImage *getImage();
    QPixmap *getPixmap();
    mrcImage *getSourceImage();
    void viewOverlay();


    void addSelectionVertex(const QPoint &point);
    void addNativeSelectionVertex(const QPoint &point);

    const QList<QPoint> &selectionVertexList();
    const QList<QPoint> &rawSelectionVertexList();

    void toggleSpot(const QPoint &pos);

    float imageMax();
    float imageMin();

    float imageScale();

    bool isVisible(const QString &overlay);
    bool isSaved(const QString &name);
    void setPSPeakListFile(const QString &file);

    bool checkSaved();

protected:
    void paintEvent(QPaintEvent *event);

public slots:
    void update();
    void togglePeakList();
    void toggleLatticeView();
    void toggleSecondLatticeView();
    void toggleCTFView();
    void toggleResolutionRing();
    void setResolutionRingValue(double value);
    void toggleParticleView();
    void setCTFView(bool enable);
    void setResolutionRingView(bool enable);
    void setParticlesView(bool enable);
    void setPeakListView(bool enable);
    void setLatticeView(bool enable);
    void setRefineLatticeView(bool enable);
    void setVisible(const QString &overlay, bool enable);
    void toggleVisible(const QString &overlay);
    void setPathView(bool enable);
    void updateLattice();
    void calculateCTF(float defocusX, float defocusY, float astigmatism);
    void setCurrentMousePos(const QPoint &pos);

    bool loadPSPeaks();
    bool loadParticles();

    int loadPeakList();
    int savePeakList();
    void clearPeakList();

    int saveSelectionList();
    void clearSelectionVertices();

    int gaussian(int i, int j);

    void setLatticeEllipseSize(int size);
    void setFontSize(int size);
    void setLatticeEllipseLineThickness(int thickness);
    void setLatticeOrders(int orders);
    void setPeakEllipseSize(int size);
    void setRefinementEllipseSize(int size);
    void setPhaseOrigin(const QPoint &pos);

    void setMaxFitRange(int range);
    void setSigma(double sigma);
    void setMaxSearchMethod(mrcImage::maxValueMethod method);
    void createProfile();

    void grabScreen();

    void rescale(float min, float max, bool invert);

    void rescaleWidget();

    void zoomIn();
    void zoomOut();
    void zoomStandard();


signals:
    void painted();


private:
    typedef int (FullScreenImage::*saveFunction)();
    
    QString workingDir;
    mrcHeader *imageHeader;

    int screenWidth, screenHeight;

    QString peakListFileName, selectionListFileName, particlesFileName;

    bool overlayVisible, peakListVisible, latticeVisible, secondLatticeVisible, refineLatticeVisible, ctfVisible, resolutionRingVisible, selectionVisible, particlesVisible;
    QHash<QString, bool> visible;
    float scale;

    QPainter *image_base;
    QImage *image;
    mrcImage *sourceImage;
    QPixmap pixmap;

    QSet<QPoint> peakList;
    QSet<QPoint> psPeaks;
    QMap<QPoint, float> particlePositionsToFom;
    QSet<QPointF> rawPeaks;
    QMap<QPoint, QPoint> *latticeRefineList;
    QPointF *refinementCandidate;
    int peakNum;
    int latticeEllipseSize, latticeEllipseLineThickness, latticeOrders, peakEllipseSize, refinementEllipseSize, fontSize;

    int maximumValueFitRange;
    double maximumValueSigma;
    mrcImage::maxValueMethod maxSearchMethod;
    QImage profile;

    QPainterPath ctfCurves;
    QPainterPath selectionPath;
    QList<QPoint> selectionList;
    QList<QPoint> rawSelectionList;

    QStringList refBoxes;

    float lattice[2][2];
    float secondLattice[2][2];
    
    double resolutionRingValue;

    QString psPeakListFile;

    QHash<QString, bool> saved;
    QHash<QString, QString> saveStrings;
    QHash<QString, saveFunction> saveFunctions;

    void drawImage();
    void drawPeakList();
    void drawParticles();
    void drawSpotList();
    void drawLattice(float lattice[2][2], bool primary);
    void drawRealLattice(float lattice[2][2]);
    void drawOverlay();
    void drawRealOverlay();
    void drawRefinementList();
    void drawCTF();
    void drawResolutionRing();
    void drawTiltAxis(const QString &axis, const QString &coAxis, bool realSpace, bool invertAngle = false);
    void drawSelectionPath();
    void drawReferenceLocation(int i);
    void drawMaximumValueFit();
    void drawUnbendProfile();

    QPointF coordinates(const QPoint &i);

};

#endif
