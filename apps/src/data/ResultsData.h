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

#ifndef RESULTSDATA_H
#define RESULTSDATA_H

#include <QFile>
#include <QFileSystemWatcher>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QStringList>
#include <QDebug>

#include "ParameterConfiguration.h"

class ResultsData : public QObject {
    
    Q_OBJECT

public:
    QMap<QString, QMap<QString, QString> > results;
    QMap<QString, QMap<QString, QString> > images;

    ResultsData(const QDir& workDir, QObject* parent=0);
    bool newImagesImported();

public slots:
    bool load(const QString &fileName);
    bool load();
    bool save();

signals:
    void loaded(bool successful);

private:
    QString mainDir;
    QString fileName;
    bool imagesImported =  false;
    QStringList imagesToBeReset;

    void printValues();
};

#endif
