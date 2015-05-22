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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    rangeMax = 10;
    rangeMin = -10;
    idle = 0;

    pixGreen= new QPixmap(":/resources/greenLED.png");
    pixRed= new QPixmap(":/resources/redLED.png");

    ui->label_LED->setPixmap(pixRed->scaledToHeight(50));

    ui->plotWidget->setBackground(QBrush("black"));
    ui->plotWidget->xAxis->setBasePen(QPen("white"));
    ui->plotWidget->xAxis->setLabelColor(QColor("white"));
    ui->plotWidget->xAxis->setTickLabelColor(QColor("white"));
    ui->plotWidget->xAxis->setTickPen(QPen("white"));
    ui->plotWidget->xAxis->setSubTickPen(QPen("white"));

    ui->plotWidget->yAxis->setBasePen(QPen("white"));
    ui->plotWidget->yAxis->setLabelColor(QColor("white"));
    ui->plotWidget->yAxis->setTickLabelColor(QColor("white"));
    ui->plotWidget->yAxis->setTickPen(QPen("white"));
    ui->plotWidget->yAxis->setSubTickPen(QPen("white"));

    ui->plotWidget->xAxis->setLabel(QString("time (s)"));
    ui->plotWidget->yAxis->setLabel(QString("Jitter (ppm)"));

    ui->plotWidget->yAxis->setRange(rangeMin, rangeMax);
    ui->plotWidget->xAxis->setRange(0,1);

    ui->plotWidget->addGraph();
    ui->plotWidget->graph(0)->setPen(QPen(Qt::yellow));
    ui->plotWidget->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->plotWidget->setInteraction(QCP::iRangeDrag, true);

    //get current date
    QDate date = QDate::currentDate();
    QString dateString = date.toString();
    ui->label_Day->setText(dateString);

    //get current time
    QTime time = QTime::currentTime();
    QString timeString = time.toString();
    ui->label_Time->setText(timeString);


    clock_timer = new QTimer(this);
    connect(clock_timer, SIGNAL(timeout()),this, SLOT(UpdateTime()));
    clock_timer->start(1000);//updating time on screen every 1 sec.

    thread = new PipeReader();
    thread->start();

    connect(thread, SIGNAL(dataToPlot(QStringList)), this, SLOT(processNewData(QStringList)));

    this->showFullScreen();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    qApp->exit();
}

void MainWindow::UpdateTime(void)

{
    //get current date
    QDate date = QDate::currentDate();
    QString dateString = date.toString();
    ui->label_Day->setText(dateString);

    //get current time
    QTime time = QTime::currentTime();
    QString timeString = time.toString();
    ui->label_Time->setText(timeString);

    idle++;

    if(idle > 3)
    {
        QStringList dummy;
        dummy << "1" << "" << "" << "0" << "s0" << "" << "0" << "" << "" << "0" << "";
        processNewData(dummy);
    }
}

void MainWindow::processNewData(QStringList list)
{
//    QString timeStamp;
    int isDummy;

    if(list.at(0).contains("ptp4l")){
        isDummy = 0;
        idle = 0;
    }else
        isDummy = 1;

    if(prevDummy != isDummy)
        resetGraph();

    QString masterOffset = list.at(3);
    QString state = list.at(4);
    QString freq = list.at(6);
    QString pathDelay = list.at(9);

    //setup lock state
    if(state.contains(QString("s0")))
        switchLED(0);
    else if(state.contains(QString("s1")))
        switchLED(0);
    else if(state.contains(QString("s2")))
        switchLED(1);

    //display the numbers
    if(!freq.isEmpty())
        ui->label_Frequency->setText(freq);
    if(!pathDelay.isEmpty())
        ui->label_Delay->setText(pathDelay);

    //add to graph
    int valueToPlot = masterOffset.toInt();
    if(valueToPlot > rangeMax+5){
        rangeMax = valueToPlot + 5;
        ui->plotWidget->yAxis->setRangeUpper(rangeMax);
    }
    if(valueToPlot < rangeMin-5){
        rangeMin = valueToPlot - 5;
        ui->plotWidget->yAxis->setRangeLower(rangeMin);
    }

    QTime time = QTime::currentTime();
//    QStringList timeStampList = timeStamp.split(":");

    int seconds = time.second() + 60* (time.minute() + 60* time.hour());

    //we don't want to have timestamps that spread over days between 2 consecutive samples - plot not readable

    if(isDummy)
        ui->plotWidget->graph(0)->setPen(QPen(Qt::red));
    else
        ui->plotWidget->graph(0)->setPen(QPen(Qt::yellow));

    if(seconds > ui->plotWidget->xAxis->range().upper+100 || seconds < ui->plotWidget->xAxis->range().lower-20)
    {
        resetGraph();
    }

    if(ui->plotWidget->xAxis->range().size() < 3)
        ui->plotWidget->xAxis->setRange(seconds, seconds+5);

    if(ui->plotWidget->xAxis->range().upper < seconds+2)
        ui->plotWidget->xAxis->setRangeUpper(seconds+2);
    if(ui->plotWidget->xAxis->range().lower < ui->plotWidget->xAxis->range().upper - 20)
        ui->plotWidget->xAxis->setRangeLower( ui->plotWidget->xAxis->range().upper - 20);

    ui->plotWidget->graph(0)->addData(seconds,valueToPlot);
    ui->plotWidget->replot();

    prevDummy = isDummy;

}

void MainWindow::switchLED(int flag)
{
    if(flag)
        ui->label_LED->setPixmap(pixGreen->scaledToHeight(50));
    else
        ui->label_LED->setPixmap(pixRed->scaledToHeight(50));
}

void MainWindow::resetGraph()
{
        //reset the graph
        ui->plotWidget->graph(0)->removeDataAfter(0);
        rangeMin=-10;
        rangeMax=10;
        ui->plotWidget->yAxis->setRange(rangeMin, rangeMax);
        ui->plotWidget->xAxis->setRange(0,1);
}
