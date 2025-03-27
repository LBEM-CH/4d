#ifndef CONFINTERFACE_H
#define CONFINTERFACE_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QToolButton>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QLineEdit>

#include "GraphicalButton.h"
#include "ParameterConfiguration.h"
#include "ParameterSection.h"

class ParametersWidget : public QScrollArea {
    Q_OBJECT

public:
    ParametersWidget(ParametersConfiguration* data, QWidget *parent = NULL);
    ParametersWidget(ParametersConfiguration* data, int userLevel, QWidget *parent = NULL);
    ParametersWidget(ParametersConfiguration* data, QStringList parametersDisplayed, int userLevel = 0, QWidget *parent = NULL);
    
    void resetConf(ParametersConfiguration* conf);
    
public slots:
    void setSelectionUserLevel(int);
    void changeParametersDisplayed(const QStringList& toBeDisplayed);
    void load();
    void resetParameters(const QMap<QString, QString>& toBeReset);
    void searchParams(const QString&);

private:

    void initialize(int userLevel, const QStringList& toBeDisplayed);
    void changeFormWidget();

    QWidget* formWidget();

    int userLevel_;
    QStringList parametersDisplayed_;
    QStringList parametersActuallyShown_;
    
    ParametersConfiguration* data;
    
    QList<ParameterSection*> sections_;
};

#endif