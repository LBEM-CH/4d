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
#include "ApplicationData.h"
#include "ResultsModule.h"

ResultsModule::ResultsModule(const QString& workDir, ResultsData *resultsInfo, ResultsModule::Type moduleType, QWidget *parent)
: QWidget(parent) {
    workingDir = workDir;
    data = resultsInfo;
    viewType = moduleType;

    showImportant = false;
    showFileNames = false;

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    view = new QTreeWidget;
    view->setAttribute(Qt::WA_MacShowFocusRect, 0);
    view->setIndentation(8);
    view->setTextElideMode(Qt::ElideLeft);
    connect(view, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(itemSelected(QTreeWidgetItem*)));
    connect(view, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(itemActivated(QTreeWidgetItem*)));

    load();

    editor = new Translator(workingDir);

    connect(data, SIGNAL(loaded(bool)), this, SLOT(load()));

    layout->addWidget(view);
}

void ResultsModule::load() {
    QTreeWidgetItem *item, *variable;
    QString title;
    view->clear();
    fullPath.clear();
    QMap<QString, QMap<QString, QString> > *map = NULL;
    if (viewType == ResultsModule::results) {
        view->setHeaderLabels(QStringList() << "Parameter" << "Value");
        view->setColumnCount(2);
        map = &data->results;
    } else if (viewType == ResultsModule::images) {
        view->setColumnCount(1);
        view->header()->hide();
        map = &data->images;
    }
    QMapIterator<QString, QMap<QString, QString> > i(*map);
    while (i.hasNext()) {
        i.next();
        if (!i.value().isEmpty() && i.value().size() >= 1) {
            title = i.key();
            title = QDir(workingDir).relativeFilePath(title);
            if(title.startsWith('/')) title = title.remove(0, 1);
            if(title.endsWith('/')) title = title.remove(title.length()-1, 1);
            if (!title.contains('/')) item = NULL;
            else item = new QTreeWidgetItem(view, QStringList() << title);
            QMapIterator<QString, QString> j(i.value());
            while (j.hasNext()) {
                j.next();
                if (!j.key().contains(QRegExp("^##\\w*##$"))) {
                    if (viewType == ResultsModule::results) {
                        bool important = false;
                        QString param = j.key();
                        if(j.key().contains("##FORCE##")) {
                            param.remove("##FORCE##");
                            important = true;
                        }
                        if (item != NULL) variable = new QTreeWidgetItem(item, QStringList() << param << j.value());
                        else variable = new QTreeWidgetItem(view, QStringList() << param << j.value());
                        
                        if(important) {
                            variable->setForeground(0, Qt::blue);
                            variable->setForeground(1, Qt::blue);
                        }                      
                    } else if (viewType == ResultsModule::images) {
                        QString fileBaseName = j.key();
                        fileBaseName.remove(QRegExp("<<@\\d+>>"));
                        QStringList cells = fileBaseName.split("##");
                        fileBaseName = cells.first();
                        QString ext;
                        if(cells.size() == 1) ext = QFileInfo(cells.first()).suffix();
                        else ext = cells[1];
                        
                        bool important = false;
                        QString itemName;
                        itemName = j.value();
                        if (itemName.startsWith("<<@important>>")) {
                            important = true;
                            itemName.remove("<<@important>>");
                        }
                        
                        if (itemName.trimmed().isEmpty() || showFileNames) itemName = fileBaseName;
                        QString path;

                        if(fileBaseName.startsWith('/')) path = QDir(fileBaseName).canonicalPath();
                        else path = QDir(i.key() + "/" + fileBaseName).canonicalPath();

                        if (!path.isEmpty()) {
                            if (!showImportant || (showImportant && important)) {
                                if (item != NULL)
                                    variable = new QTreeWidgetItem(item, QStringList() << itemName);
                                else
                                    variable = new QTreeWidgetItem(view, QStringList() << itemName);
                                if (important) variable->setForeground(0, QColor(198, 25, 10));
                                
                                
                                fullPath.insert(variable, QStringList() << i.key() + "/" + fileBaseName << ext);
                                variable->setToolTip(0, "<B>" + ext.toUpper() + "</B> Type Image:<br>" + fileBaseName);
                                
                            }
                        }
                        
                        if (item != NULL && item->childCount() == 0) item->setHidden(true);
                    }
                }
            }
        }
    }
    view->expandAll();
    resetColumnSize();
}

QString ResultsModule::selectedImagePath() {
    if(viewType == ResultsModule::Type::results) return QString();
    if(view->selectedItems().isEmpty()) return QString();

    return fullPath[view->selectedItems().first()].first();
}

QString ResultsModule::selectedImageExtenstion() {
    if(viewType == ResultsModule::Type::results) return QString();
    if(view->selectedItems().isEmpty()) return QString();
    
    QString ext;
    
    if(fullPath[view->selectedItems().first()].size() < 2) ext = QFileInfo(fullPath[view->selectedItems().first()].first()).suffix();
    else ext = fullPath[view->selectedItems().first()].at(1);
    
    return ext;
}

void ResultsModule::itemSelected(QTreeWidgetItem *item) {
    QString ext, file;
    if(fullPath.keys().contains(item)) {
        if(fullPath[item].size() >= 1) file = fullPath[item].first();
        if(fullPath[item].size() >= 2) ext = fullPath[item].at(1);
        emit resultChanged(file, ext);
    }
    else {
        emit resultChanged(QString(), QString());
    }
}

void ResultsModule::itemActivated(QTreeWidgetItem *item) {
    if (viewType == ResultsModule::images) {
        QString result = fullPath[item].first();
        QString ext;
        if(fullPath[item].size() >= 2) ext = fullPath[item].at(1);
        editor->open(result, ext);
    }
}

void ResultsModule::setImportant(int value) {
    showImportant = (bool)value;
    load();
}

void ResultsModule::setImportant(bool value) {
    showImportant = value;
    load();
}

void ResultsModule::setShowFilenames(int value) {
    showFileNames = (bool)value;
    load();
}

void ResultsModule::resetColumnSize() {
    for (int col = 0; col < view->columnCount(); col++) {
        view->resizeColumnToContents(col);
    }
}