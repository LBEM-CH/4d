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
#include <QDebug>
#include <QProgressDialog>
#include <QtConcurrent>
#include <QMutex>

#include "ApplicationData.h"
#include "ProjectData.h"
#include "ParameterElementData.h"

#include "ResultsData.h"

ResultsData::ResultsData(const QDir& workDir, QObject *parent)
: QObject(parent) {
    mainDir = workDir.canonicalPath();
}

bool ResultsData::load(const QString &name) {
    fileName = name;
    QFile file(fileName);
    results.clear();
    images.clear();
    imagesToBeReset.clear();
    imagesImported = false;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit loaded(false);
        return false;
    }
    QString line, image, nickName, ext;
    QString currentDirectory = mainDir;
    if(QFileInfo(mainDir + "/2dx_merge.cfg").exists()) results[currentDirectory]["##CONFFILE##"] = "2dx_merge.cfg";
    else if(QFileInfo(mainDir + "/2dx_image.cfg").exists()) results[currentDirectory]["##CONFFILE##"] = "2dx_image.cfg";
    else {
        results[currentDirectory]["##CONFFILE##"] = "2dx_image.cfg";
        qDebug() << "CRITICAL: While parsing results file, could not locate a cfg file in: " << currentDirectory;
    }
    int k = 0;
    while (!file.atEnd()) {
        line = file.readLine().trimmed();
        if (line.contains(QRegExp("^\\s*#")) && !line.contains(QRegExp("^\\s*#\\s*lock", Qt::CaseInsensitive))) {
            QRegExp pattern(".*#\\s*image(-important)?\\s*([^\"\\s]*)?\\s*:\\s*\"?([^\"\\s]*)\"?\\s*(<.*>)?", Qt::CaseInsensitive);
            if (pattern.indexIn(line) != -1) {
                QString important;
                if (pattern.cap(1).toLower() == "-important") important = "<<@important>>";
                ext = pattern.cap(2).toLower();
                image = pattern.cap(3);
                if(ext.isEmpty()) ext = QFileInfo(image).suffix().toLower();
                nickName = pattern.cap(4);
                nickName.remove(QRegExp("^.*<|>.*$"));

                QFileInfo inf(currentDirectory + "/" + image);
                QString order = QString("%1").arg(k, 6, 10, QChar('0'));

                order.prepend("<<@");
                order.append(">>");
                if (inf.isDir()) {
                    QDir dir(inf.canonicalFilePath());
                    foreach(QString file, dir.entryList(QDir::Files)) images[inf.canonicalFilePath()][order + file] = important + file;
                } else{
                    images[currentDirectory][order + image + "##" + ext] = important + nickName;
                }
                k++;
            }
        } else {
            //line.replace(QRegExp("#.*"),"");
            if (line.contains(QRegExp("<\\s*IMAGEDIR\\s*=", Qt::CaseInsensitive))) {
                currentDirectory = line.replace(QRegExp(".*<\\s*IMAGEDIR\\s*=\\s*\\\"?\\s*([^\"\\s]*)\\s*\\\"?\\s*>.*", Qt::CaseInsensitive), "\\1");
                results[currentDirectory]["##CONFFILE##"] = "2dx_image.cfg";
            } else if (line.contains(QRegExp("<\\s*RESETDIR\\s*=", Qt::CaseInsensitive))) {
                imagesToBeReset.append(line.replace(QRegExp(".*<\\s*RESETDIR\\s*=\\s*\\\"?\\s*([^\"\\s]*)\\s*\\\"?\\s*>.*", Qt::CaseInsensitive), "\\1"));
            } else if (line.contains(QRegExp("<IMPORTDIR>", Qt::CaseInsensitive))) {
                imagesImported = true;
                //qDebug() << "Ok. Got that new images were added";
            } else if (line.contains(QRegExp("<IMAGEDIR>", Qt::CaseInsensitive))) {
                //qDebug() << "Ok. Now will be saving in results in:" << mainDir;
                currentDirectory = mainDir;
            } else if (line.startsWith("set -force", Qt::CaseInsensitive)) {
                QStringList resultKeyValue = line.remove(0, QString("set -force").length()).split('=');
                if (resultKeyValue.size() == 2) {
                    resultKeyValue[0] = "##FORCE##" + resultKeyValue[0].simplified();
                    results[currentDirectory][resultKeyValue[0]] = resultKeyValue[1].remove('"').simplified();
                }
            } else if (line.startsWith("set", Qt::CaseInsensitive)) {
                QStringList resultKeyValue = line.remove(0, QString("set").length()).split('=');
                if (resultKeyValue.size() == 2) {
                    results[currentDirectory][resultKeyValue[0].simplified()] = resultKeyValue[1].remove('"').simplified();
                }
            } else if (line.contains(QRegExp("^\\s*#\\s*lock", Qt::CaseInsensitive))) {
                qDebug() << "Results contains [un]lock. This is not yet implemented.";
            }
        }
    }
    
    file.close();
    
    emit loaded(true);
    return true;
}

bool ResultsData::save() {
    for(QString image : imagesToBeReset) {
        qDebug() << "Reseting: " << image;
        projectData.reloadParameterData(QDir(image));
    }

    //Copy the results to a local copy
    QMap<QString, QMap<QString, QString> > localResults = results;
    QList<ParametersConfiguration*> toBeSaved;
    
    QProgressDialog dialog;
    dialog.setRange(0, localResults.keys().size());
    dialog.setWindowTitle("Updating results");
    dialog.setValue(0);
    
    //Change parameters in serial
    int progress = 0;
    for (QString image : localResults.keys()) {
        if(dialog.wasCanceled()) break;
        dialog.setLabelText(QString("Updating parameters from %1 of %2 images...").arg(progress++).arg(localResults.keys().size()));
        dialog.setValue(progress);
        
        ParametersConfiguration* local = projectData.parameterData(QDir(image));
        QMap<QString, QString> toBeChanged = localResults[image];

        if (local) {
            for (QString param : toBeChanged.keys()) {
                if (param.contains("##FORCE##")) {
                    //qDebug() << "setForce " << j.key().trimmed().replace("##FORCE##","") << " to " << j.value().trimmed(); 
                    local->setForce(param.replace("##FORCE##", "").trimmed(), toBeChanged[param].trimmed(), false);
                } else {
                    //qDebug() << "set " << j.key().trimmed() << " to " << j.value().trimmed(); 
                    local->set(param.trimmed(), toBeChanged[param].trimmed(), false);
                }
            }
            toBeSaved.append(local);
        }
    }
    
    if(!toBeSaved.isEmpty()) {
        //Save the files in parallel
        QFuture<void> future = QtConcurrent::map(toBeSaved, [=](ParametersConfiguration* local) {
            local->setModified(true);
        });

        future.waitForFinished();
    }
    
    return true;
}

void ResultsData::printValues() {
    QMapIterator<QString, QMap<QString, QString> > it(results);
    while (it.hasNext()) {
        it.next();
        qDebug() << it.key();
        QMapIterator<QString, QString> j(it.value());
        while (j.hasNext()) {
            j.next();
            qDebug() << " " << j.key() << " = " << j.value();
        }
    }

    qDebug() << "\nImages: \n";

    QMapIterator<QString, QMap<QString, QString> > k(images);
    while (k.hasNext()) {
        k.next();
        QMapIterator<QString, QString> j(it.value());
        while (j.hasNext()) {
            j.next();
            qDebug() << k.key();
            qDebug() << " " << j.value();
        }
    }
}

bool ResultsData::load() {
    return load(fileName);
}

bool ResultsData::newImagesImported() {
    return imagesImported;
}

