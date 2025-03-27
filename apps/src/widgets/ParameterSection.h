/***************************************************************************
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

#ifndef CONFSECTIONHEADER_H

#include <QObject>
#include <QWidget>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QString>

#include "ParameterInput.h"
#include "GroupContainer.h"

class ParameterSection : public GroupContainer {
    Q_OBJECT

public:
    ParameterSection(QString sectionTitle, QWidget *parent=NULL);
    void finishAddingParameters();
    void changeDisplayedParameters(int userLevel, QStringList parametersDisplayed);

public slots:
    void loadFromConfig();
    void resetParameters(const QMap<QString, QString>& toBeReset);
    void addParameter(ParameterElementData* element);
    QString getWhatsThis(ParameterElementData* element);

private:

    QMap<QString, ParameterInput*> parameterInputLookup_;
    QMap<QString, int> parameterRowLevelLookup_;
    QMap<QString, int> parameterUserLevelLookup_;

    QFormLayout* formLayout_;
};

#endif
