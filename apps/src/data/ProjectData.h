
#ifndef PROJECT_DATA_H
#define PROJECT_DATA_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QProgressDialog>
#include <QMap>
#include <QDateTime>
#include <QTimer>
#include <QProcess>

#include "ProjectPreferences.h"
#include "ProjectImage.h"
#include "ParameterConfiguration.h"
#include "ProjectMode.h"

#define projectData (ProjectData::Instance())

class ProjectData : public QObject {
    
    Q_OBJECT
    
public:

    static ProjectData& Instance();
    
    void initiailze(const QDir& projectDir);
    
    ParametersConfiguration* projectParameterData();
    void reloadProjectParameters();
    ParametersConfiguration* parameterData(const QDir& workDir);
    void reloadParameterData(const QDir& workDir);
    
    QList<ProjectImage*> projectImageList();
    ProjectImage* projectImage(const QString& group, const QString& directory);
    ProjectImage* projectImage(const QDir& workingDir);
    
    void indexImages(bool init = false);
    ProjectImage* addImage(const QString& group, const QString& directory);
    void moveImage(const QMap<ProjectImage*, QString>& movedImagesToNewPaths);
    
    QList<ProjectImage*> imagesOpen();
    bool imageOpen(ProjectImage* image);
    void setImagesOpen(const QList<ProjectImage*>& paths);
    
    QList<ProjectImage*> imagesSelected();
    QList<ProjectImage*> loadSelection(const QString& dirFileName);
    void saveSelection(const QString& newDirFileName);
    void setImagesSelected(const QList<ProjectImage*>& paths);
    
    QDir projectDir() const;
    QDir projectWorkingDir() const;

    QString projectName();
    void setProjectName(const QString& projectName);
    
    ProjectMode projectMode();
    void changeProjectMode();
    
    void emitStartupFinished();
    void emitLibraryLoaded();
     
    void toggleAutoSave();
    void setAutoSave(bool save);
    bool isAutoSave();
    
    static void writeStatisticsToStatusFolder(const QString& fileName, long currentTime = QDateTime::currentMSecsSinceEpoch());
    
    static QDir logsDir(const QDir& workingDir);
    static QDir procDir(const QDir& workingDir);
    
    static QString commitIntToStringLength(int num, int length);
    
    static QStringList readParamList(const QString& file, bool convertToLower);
    static QStringList uniqueParamList();
    static QStringList fileNameParamList();
    
public slots:
    
    void saveAsProjectDefault(ProjectImage* image);
    void changeProjectName();
    
    //Tasks
    void renumberImages();
    void assignEvenOdd();
    void repairLinks();
    void resetImageConfigs();
    
    void addSelectedToQueue(bool prioritize=false);
    void addImageToQueue(ProjectImage* image, QStringList scripts, bool prioritize);
    
    void openImage(ProjectImage* image);
    
    void uploadStatusData();
    
signals:

    //Emitted whenever the the count of images in project changes
    void imageCountChanged(int count);

    //Emitted when the user requests to re-index images
    void imagesReindexed();
    
    //Emitted when a new image is added
    void imageAdded(ProjectImage* image);
    
    //Emitted when a image is moved
    void imageMoved(const QList<ProjectImage*>& movedImages);
    
    //Emitted when images are to be added to the queue
    void toBeAddedToProcessingQueue(QMap<ProjectImage*, QStringList> imageAndScripts, bool prioritize);
    
    //Emitted when the focus should be on the processing window
    void focusProcessingWindow();
    
    //Emitted when an image needs to be opened
    void imageToBeOpened(ProjectImage* image);
    
    void selectionChanged(const QList<ProjectImage*>& images);
    void projectNameChanged(const QString& name);
    
    //initialization signals
    void parametersRegistered();
    void groupsInitializationStatus(const QString& status);
    void groupsInitialized(int imageCount);
    void imageInitializationStatus(const QString& status);
    void imagesInitialized();
    void startupFinished();
    void libraryLoaded();

private:

    ProjectData() {
    }
    
    bool sureDialog(const QString& title, const QString& text);
    
    void linkProjectConfig(const QString& sourceName, const QString& targetLinkName);
    
    QString selectionDirfile();
    QString evenSelectionDirfile();
    QString oddSelectionDirfile();
    
    QDir projectDir_;
    ParametersConfiguration* projectParameters_;
    QMap<QString, QMap<QString, ProjectImage*>> projectImages_;
    bool autoSave_=true;
    
    QTimer statusUploadTimer_;
    QProcess statusUploadProcess_;
    
};


#endif /* PROJECT_DATA_H */

