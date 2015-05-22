                                                                /***************************************************************************
**                                                                        **
**  Part of PTPDemo, program to visualize IEEE1588 synchronization        **
**  Copyright (C) 2013 Timesys Corporation                                **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Maciej Halasz                                        **
**  Website/Contact: http://www.timesys.com/                              **
**             Date: 10.01.13                                             **
**          Version: 1.0.0                                                **
****************************************************************************/

#include "pipereader.h"
#include <QTime>


PipeReader::PipeReader(QObject *parent) :
    QThread(parent)
{

}

void PipeReader::run() {

top:
    QFile file("/tmp/ptpFifo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
//        sleep(2);
//        QStringList dummy;
 //       QTime time = QTime::currentTime();
 //       QString timeString = time.toString();
//        dummy << "1" << "" << "" << "0" << "s0" << "" << "0" << "" << "" << "0" << "";
//        emit(dataToPlot(dummy));
        goto top;
    }

    QByteArray lineB;
         do {
             QByteArray lineB = file.readLine();
             QString line = QString(lineB);
             QStringList fields = line.split(QRegExp(" "),  QString::SkipEmptyParts);
             if(fields.contains(QString("offset")) && fields.contains(QString("freq")) && fields.contains(QString("delay"))){
                 emit(dataToPlot(fields));
             }
             qDebug() << line;
    } while (lineB != "EOF");


    file.close();
    sleep(2);
//    QStringList dummy;
//    QTime time = QTime::currentTime();
//    QString timeString = time.toString();
//    dummy << "1" << "" << "" << "0" << "s0" << "" << "0" << "" << "" << "0" << "";
//    emit(dataToPlot(dummy));
    goto top;
}

