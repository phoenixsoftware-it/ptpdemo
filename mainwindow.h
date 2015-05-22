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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "pipereader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButton_clicked();
    void UpdateTime();
    void processNewData(QStringList);

private:
    Ui::MainWindow *ui;
    QPixmap *pixGreen;
    QPixmap *pixRed;
    QTimer *clock_timer;
    PipeReader *thread;
    int rangeMax;
    int rangeMin;
    int idle;
    int prevDummy;

    void switchLED(int flag);
    void resetGraph();
};

#endif // MAINWINDOW_H
