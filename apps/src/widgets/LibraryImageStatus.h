
#ifndef LIBRARYIMAGESTATUS_H
#define LIBRARYIMAGESTATUS_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QString>
#include <QStringList>
#include <QGridLayout>
#include <QFormLayout>

#include "ProjectModel.h"

class LibraryImageStatus : public QWidget {
    Q_OBJECT

public:
    LibraryImageStatus(ProjectModel* model, QWidget* parent = NULL);

public slots:
    void updateData();

private:

    QFormLayout* fillFormLayout();
    void updateFormData();
    void readParamsList();

    ProjectModel* projModel;
    
    QFormLayout* dataLayout;

    QStringList labelsList;
    QStringList paramsList;
    QLabel* imageLabel;
};


#endif /* LIBRARYIMAGESTATUS_H */

