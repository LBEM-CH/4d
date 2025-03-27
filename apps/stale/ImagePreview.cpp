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

#include <QDebug>

#include "UserPreferenceData.h"
#include "ApplicationData.h"

#include "ImagePreview.h"

ImagePreview::ImagePreview(const QString& workDir, QString resultValue, bool info, QWidget *parent)
: QFrame(parent) {
    workingDir = workDir;
    result = resultValue;

    minWidth = int(QApplication::desktop()->width() / 5.00);
    if (minWidth > 235) minWidth = 235;
    showInfo = info;

    setAutoFillBackground(true);
    setFixedSize(QSize(minWidth, minWidth));
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

    if (!result.isEmpty() && QFileInfo(result).exists()) {
        image = new mrcImage(result, true);
        if (image->isEmpty()) {
            delete image;
            image = NULL;
        }
    } else image = NULL;
    navImage = NULL;

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignCenter);

    preview = new QStackedWidget(this);

    imageLabel = new QLabel(preview);
    imageLabel->setScaledContents(true);
    if (image != NULL)
        imageLabel->setPixmap(image->getPixmap());

    headerWidget = new MrcHeaderDisplay(preview);

    preview->addWidget(imageLabel);
    preview->addWidget(headerWidget);

    layout->addWidget(preview, 1, 0);

    QPalette pal(palette());
    pal.setBrush(QPalette::Background, QBrush(QColor(255, 255, 255), ApplicationData::icon("nullPreview").pixmap(minWidth, minWidth).scaledToWidth(minWidth)));
    setPalette(pal);
    setLayout(layout);

    resetImage(true);
}

void ImagePreview::mouseDoubleClickEvent(QMouseEvent *event) {
    launchNavigator();
    QFrame::mouseDoubleClickEvent(event);
}

void ImagePreview::launchNavigator() {
    if (!result.isEmpty() && navImage == NULL) {
        if (result.toLower().endsWith(".mrc") || result.toLower().endsWith(".map")) {

            navImage = new mrcImage(result);

            if (!navImage->isEmpty()) {
                nav = new ImageNavigator(workingDir, navImage, this);
                connect(nav, SIGNAL(closed()), this, SLOT(clearNavigator()));
            } else {
                delete navImage;
                navImage = NULL;
            }
        } else if (result.toLower().endsWith(".ps")) {
            QProcess::startDetached(userPreferenceData.get("psViewer") + " " + result);
        } else if (result.toLower().endsWith(".pdf")) {
            QProcess::startDetached(userPreferenceData.get("pdfViewer") + " " + result);
        } else if (result.toLower().endsWith(".txt") || result.toLower().endsWith(".hk") || result.toLower().endsWith(".hkl") ||
                result.toLower().endsWith(".dat") || result.toLower().endsWith(".aph") || result.toLower().endsWith(".spt")) {
            QProcess::startDetached(userPreferenceData.get("scriptEditor") + " " + result);
        }
    }
}

void ImagePreview::clearNavigator() {
    //if(nav !=NULL) {delete nav; nav = NULL;}
    if (navImage != NULL) {
        delete navImage;
        navImage = NULL;
    }
    emit load();
}

void ImagePreview::setImage(const QString &imageName) {
    result = imageName;
    if (!QFileInfo(result).exists()) {
        if (result.endsWith("final_map.mrc")) result = "NP";
        else if (result.endsWith("reference_map.mrc")) result = "NR";
        else if (result.endsWith("half_half.mrc")) result = "NH";
        else result = "";
    }
    resetInfo();
    resetImage(true);
}

void ImagePreview::resetInfo() {
    if (!QFileInfo(result).exists()) return;
    mrcHeader header(result);
    if (!result.isEmpty())
        headerWidget->setHeader(result, header);
}

void ImagePreview::resetImage(bool ignore_size) {
    int max_size = 500;
    if ((!ignore_size) && result.toLower().endsWith(".mrc")) {
        mrcImage *tempImage = new mrcImage(result, true, this);

        int sizex = tempImage->getHeader()->nx();
        if (sizex < max_size) {
            showInfo = false;
        } else {
            showInfo = true;
        }
    }

    QString suffix = QFileInfo(result).suffix().toLower();
    imageLabel->clear();
    if (!showInfo) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        //parentContainer->setText("Image Preview");
        preview->setCurrentWidget(imageLabel);
        if (!result.isEmpty() && (result.contains(".mrc"))) {
            mrcImage *tempImage = new mrcImage(result, true, this);

            if (tempImage->isEmpty()) {
                delete tempImage;
                result = "";
                return;
            }
            if (image != NULL) delete image;
            image = tempImage;
            imageLabel->setPixmap(image->getPixmap()); //.scaledToWidth(minWidth));

            update();
        } else if (result.contains(".png")) {
            imageLabel->setPixmap(QPixmap(result));
        }
        QApplication::restoreOverrideCursor();
    } else if (!result.isEmpty()) {
        //parentContainer->setText("Image Header");
        if (result.toLower().endsWith(".mrc") || result.toLower().endsWith(".map")) preview->setCurrentWidget(headerWidget);
        else preview->setCurrentWidget(imageLabel);
    }

    if (result.isEmpty()) preview->setCurrentWidget(imageLabel);

    if (suffix != "mrc" && suffix != "map" && !suffix.isEmpty()) {
        clearImage();
        QFont labelFont("Apple Chancery", 23, QFont::Normal);
        QString ext = QFileInfo(result).suffix().toLower();
        if (ext == "txt") ext = "text";
        if (ext == "ps") ext = "postscript";
        ext[0] = ext[0].toUpper();
        QPalette pal(palette());
        int color = 255 - (int) (0.80 * 255);
        pal.setColor(QPalette::WindowText, QColor(color, color, color));
        pal.setColor(QPalette::Text, QColor(color, color, color));
        pal.setBrush(QPalette::Background, Qt::white);
        imageLabel->setText("." + ext + " File.<br>");
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setFont(labelFont);
        imageLabel->setAutoFillBackground(true);
        imageLabel->setPalette(pal);
    }

    QString msg;
    if (result == "NP") msg = "Image not processed<br>(Double click to start processing)";
    else if (result == "NR") msg = "No reference available";
    else if (result == "NH") msg = "Half-Half map<br>not available";

    if (result == "NP" || result == "NR" || result == "NH") {
        clearImage();
        QFont labelFont("Apple Chancery", 15, QFont::Normal);
        QPalette pal(palette());
        int color = 255 - (int) (0.80 * 255);
        pal.setColor(QPalette::WindowText, QColor(color, color, color));
        pal.setColor(QPalette::Text, QColor(color, color, color));
        pal.setBrush(QPalette::Background, Qt::white);
        imageLabel->setText(msg);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setFont(labelFont);
        imageLabel->setAutoFillBackground(true);
        imageLabel->setPalette(pal);
    }

    update();
}

void ImagePreview::shade() {
    if (isHidden()) {
        resetImage(true);
        show();
    } else
        close();
}

void ImagePreview::toggleInfo() {
    showInfo = showInfo ^ true;
    resetImage(true);
}

void ImagePreview::showImageHeader(bool show) {
    showInfo = show;
    resetImage(true);
}

void ImagePreview::progressDialog() {
    QDialog *dialog = new QDialog(this);
    dialog->setModal(false);
    dialog->setFixedSize(QSize(700, 400));
    QGridLayout *dialogLayout = new QGridLayout(dialog);
    dialogLayout->setAlignment(Qt::AlignCenter);
    dialogLayout->addWidget(new QLabel("Generating Thumbnail", dialog));
    QProgressBar *progress = new QProgressBar(dialog);
    progress->setMaximum(100);
    progress->setValue(0);
    connect(this, SIGNAL(setProgress(int)), progress, SLOT(setValue(int)));
    dialogLayout->addWidget(progress);
    dialog->setLayout(dialogLayout);
    dialog->show();
}

void ImagePreview::clearImage() {
    if (image != NULL) delete image;
    image = NULL;
}

