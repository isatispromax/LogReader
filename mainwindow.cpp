#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include "qcustomplot.h"
#include "math.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    logfile = NULL;

   // ui->data_table->setRowCount(100);
    ui->data_table->setColumnCount(10);
    //ui->data_table->resize(10,20);

    logfile_type = 0;
    ui->data_table->setLineWidth(100);
    ui->data_table->verticalHeader()->setVisible(true);
    ui->data_table->horizontalHeader()->setStretchLastSection(true);
    ui->data_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->data_table->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->customPlot->setBackground(QColor(66,66,66));
    ui->customPlot->xAxis->setTickLabelColor(QColor(220,220,220));
    ui->customPlot->yAxis->setTickLabelColor(QColor(220,220,220));
    ui->customPlot->yAxis2->setTickLabelColor(QColor(220,220,220));
    ui->customPlot->xAxis->setLabelColor(QColor(220,220,220));
    ui->customPlot->yAxis->setLabelColor(QColor(220,220,220));
    ui->customPlot->yAxis2->setLabelColor(QColor(220,220,220));

    ui->customPlot->legend->setTextColor(QColor(220,220,220));
    ui->customPlot->legend->setBrush(QColor(66,66,66));

    QFile file(":/qss/psblack.css");
    if (file.open(QFile::ReadOnly))
    {
        QString qss = QLatin1String(file.readAll());
        QString paletteColor = qss.mid(20, 7);
        qApp->setPalette(QPalette(QColor(paletteColor)));
        qApp->setStyleSheet(qss);
        file.close();
    }

    logfile_type = 2;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{



     QByteArray readed_line;
    QString FileName = QFileDialog::getOpenFileName(this,NULL,NULL,"*.txt");
    //qDebug()<<FileName;
    if(FileName.isEmpty())
    {
        return;
    }
    time_axis.clear();
    glucose_axis.clear();
    bat_axis.clear();
    temperature_axis.clear();
    ui->text_formread->clear();
    ui->data_table->clear();
    ui->data_table->setRowCount(0);
    ui->data_table->setHorizontalHeaderLabels(QStringList()<<"Time"<<"glucose ad"<<"glucose voltage"<<"glucose I"\
                                              <<"temperature ad"<<"temperature voltage"<<"temperature R"<<"temperature Value"<<"bat ad"<<"bat voltage");

    logfile = new QFile(FileName);
    if(!logfile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<"open file error";
    }
    else
    {
        qDebug()<<"open file right";
        quint64 i=0;
        while(!logfile->atEnd())
        {
            //读取一行的内容
            readed_line = logfile->readLine();
            //提取一行内的数据
            switch (logfile_type)
            {
            case 0://nordic dongle
                get_infomationFromtext(readed_line,i);
                break;
            case 1://ti dongle
                break;
            case 2: //phone
                get_infomationFromtext_phone(readed_line,i);
                break;
            default:
                break;
            }

            //get_infomationFromtext_phone(readed_line,i);
            //提取画图信息
            //把一行的文本插入下方的文本框
            QString line_code(readed_line);
            ui->text_formread->insertPlainText(line_code);
            i++;
        }
        for(i=0; i < ui->data_table->rowCount();i++)
        {
            time_axis.append(i);
        }
        plot_data();
   // ui->customPlot->graph()->setData();
        //qDebug()<<logfile->readLine();
    }
}

cgm_dat MainWindow::get_infomationFromtext(QByteArray &line,quint64 line_num)
{
    cgm_dat data;
    QString lin(line);
    QRegExp info(tr("\\bHandleValueNotification\\b"));
    int pos(0);

    if(pos = info.indexIn(lin,pos)!=-1)
    {
        info.setPattern("0600\\w*");
        if(pos = info.indexIn(lin,pos)!=-1)
        {
            quint8 hi;
            quint8 lo;
            QString dat = info.capturedTexts().at(0);
            //电池信息
            hi = twochartohex(dat.at(10).toLatin1(),dat.at(11).toLatin1());
            lo = twochartohex(dat.at(8).toLatin1(),dat.at(9).toLatin1());
            data.bat = (hi<<8)|lo;
            if(data.bat <0)
            {
                data.bat = 0-data.bat;
            }
            data.bat_voltage = saadc_getvoltage(data.bat);
            bat_axis.append((double)data.bat_voltage);
            //温度信息
            hi = twochartohex(dat.at(6).toLatin1(),dat.at(7).toLatin1());
            lo = twochartohex(dat.at(4).toLatin1(),dat.at(5).toLatin1());
            data.temperature = (hi<<8)|lo;

         //   qDebug()<<data.temperature;

            if(data.temperature <0)
            {
                data.temperature = 0-data.temperature;
            }
 //           data.temperature_voltage = saadc_getvoltage(data.temperature);
            //更新温度电压算法
            data.temperature_voltage = saadc_getvoTemVoltage(data.temperature,data.bat_voltage);
            data.temperature_R = saadc_tempertureRData(data.temperature);
            data.temperatureValue = saadc_tempertureValue(data.temperature_R);

            temperature_axis.append((double)data.temperature_voltage);

            //血糖信息
            hi = twochartohex(dat.at(14).toLatin1(),dat.at(15).toLatin1());
            lo = twochartohex(dat.at(12).toLatin1(),dat.at(13).toLatin1());
            data.glucose = (hi<<8)|lo;
            if(data.glucose <0)
            {
                data.glucose = 0-data.glucose;
            }
            data.glucose_voltage = ads1114_getvoltage(data.glucose);
            data.glucose_I = ads114_turntoI(data.glucose_voltage);

            glucose_axis.append((double)data.glucose_I);
            //时间信息
            quint8 t1,t2,t3,t4;
            t4 = twochartohex(dat.at(16).toLatin1(),dat.at(17).toLatin1());
            t3 = twochartohex(dat.at(18).toLatin1(),dat.at(19).toLatin1());
            t2 = twochartohex(dat.at(20).toLatin1(),dat.at(21).toLatin1());
            t1 = twochartohex(dat.at(22).toLatin1(),dat.at(23).toLatin1());
            time_t tim;
            tim = (t1<<24)|(t2<<16)|(t3<<8)|t4;
            QDateTime dati = QDateTime::fromTime_t(tim);
            qDebug()<<"n:"<<data.temperature<<data.bat<<data.glucose<<"lin"<<line_num;
            //插入表格
            ui->data_table->setRowCount(ui->data_table->rowCount()+1);

            ui->data_table->setItem(ui->data_table->rowCount()-1,0,new QTableWidgetItem(dati.toString("yyyy:MM:dd:hh:mm:ss")));
            ui->data_table->setItem(ui->data_table->rowCount()-1,1,new QTableWidgetItem(QString::number(data.glucose)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,2,new QTableWidgetItem(QString::number(data.glucose_voltage)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,3,new QTableWidgetItem(QString::number(data.glucose_I)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,4,new QTableWidgetItem(QString::number(data.temperature)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,5,new QTableWidgetItem(QString::number(data.temperature_voltage)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,6,new QTableWidgetItem(QString::number(data.temperature_R)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,7,new QTableWidgetItem(QString::number(data.temperatureValue)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,8,new QTableWidgetItem(QString::number(data.bat)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,9,new QTableWidgetItem(QString::number(data.bat_voltage)));
            /*
            ui->data_table->item(ui->data_table->rowCount()-1,0)->setBackgroundColor(QColor(245,245,245));
            ui->data_table->item(ui->data_table->rowCount()-1,1)->setBackgroundColor(QColor(200,200,200));
            ui->data_table->item(ui->data_table->rowCount()-1,2)->setBackgroundColor(QColor(200,200,200));
            ui->data_table->item(ui->data_table->rowCount()-1,3)->setBackgroundColor(QColor(200,200,200));
            ui->data_table->item(ui->data_table->rowCount()-1,4)->setBackgroundColor(QColor(186,210,252));
            ui->data_table->item(ui->data_table->rowCount()-1,5)->setBackgroundColor(QColor(186,210,252));
            ui->data_table->item(ui->data_table->rowCount()-1,6)->setBackgroundColor(QColor(244,244,150));
            ui->data_table->item(ui->data_table->rowCount()-1,7)->setBackgroundColor(QColor(244,244,150));
            */
            //ui->data_table->setItem();


        }
        info.setPattern("\\w{2}:\\w{2}:\\w{2}");
        if(pos = info.indexIn(lin,pos) != -1)
        {
            QString dat;
            dat = info.capturedTexts().at(0);
        }
        return data;
    }
    else
    {
        return data;
}
}

quint8 MainWindow::twochartohex(quint8 hi, quint8 lo)
{
  // quint8 num;

   // hi.toLatin1();

    if(hi >='0' && hi<='9') hi = hi - '0';
    else if(hi >='a' && hi<='f') hi = hi - 'a' + 10;
    else if(hi >='A' && hi<='F') hi = hi - 'A' + 10;

    if(lo >='0' && lo<='9') lo = lo - '0';
    else if(lo >='a' && lo<='f') lo = lo - 'a' + 10;
    else if(lo >='A' && lo<='F') lo = lo - 'A' + 10;

   // num = (hi<<4)|lo;

   // qDebug()<<"hi:"<<hi<<"lo:"<<lo<<"result:"<<num;

    return (hi<<4)|lo;
}

void MainWindow::plot_data()
{
    ui->customPlot->clearGraphs();
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes|
                                    QCP::iSelectLegend | QCP::iSelectPlottables);

    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->xAxis->setLabel("序号/时间");
    ui->customPlot->yAxis->setLabel("电流nA");
    ui->customPlot->yAxis2->setLabel("电压V");
    ui->customPlot->xAxis->setRange(0,1);
    ui->customPlot->yAxis->setRange(0,1);
    ui->customPlot->yAxis2->setRange(1,2);
    ui->customPlot->addGraph();
    ui->customPlot->addGraph(ui->customPlot->xAxis,ui->customPlot->yAxis2);
    ui->customPlot->addGraph(ui->customPlot->xAxis,ui->customPlot->yAxis2);

    ui->customPlot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->customPlot->legend->setFont(legendFont);
    ui->customPlot->legend->setSelectedFont(legendFont);
    ui->customPlot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items

    ui->customPlot->graph(0)->setLineStyle((QCPGraph::LineStyle)1);
    ui->customPlot->graph(0)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(rand()%14+1)));
    QPen graphPen;
    graphPen.setColor(QColor(rand()%245+10, rand()%245+10, rand()%245+10));
    graphPen.setWidthF(rand()/(double)RAND_MAX*2+1);
    ui->customPlot->graph(0)->setPen(graphPen);
    ui->customPlot->graph(0)->setName("glucose");

    ui->customPlot->graph(1)->setLineStyle((QCPGraph::LineStyle)1);
    ui->customPlot->graph(1)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(rand()%14+1)));
    graphPen.setColor(QColor(rand()%245+10, rand()%245+10, rand()%245+10));
    graphPen.setWidthF(rand()/(double)RAND_MAX*2+1);
    ui->customPlot->graph(1)->setPen(graphPen);
    ui->customPlot->graph(1)->setName("bat");

    ui->customPlot->graph(2)->setLineStyle((QCPGraph::LineStyle)1);
    ui->customPlot->graph(2)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(rand()%14+1)));
    graphPen.setColor(QColor(rand()%245+10, rand()%245+10, rand()%245+10));
    graphPen.setWidthF(rand()/(double)RAND_MAX*2+1);
    ui->customPlot->graph(2)->setPen(graphPen);
    ui->customPlot->graph(2)->setName("temperature");

    ui->customPlot->graph(0)->setData(time_axis,glucose_axis);
    ui->customPlot->graph(1)->setData(time_axis,bat_axis);
    ui->customPlot->graph(2)->setData(time_axis,temperature_axis);
    ui->customPlot->graph(0)->rescaleAxes(true);
    ui->customPlot->graph(1)->rescaleAxes(true);
    ui->customPlot->graph(2)->rescaleAxes(true);
    ui->customPlot->replot();

    connect(ui->customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
    connect(ui->customPlot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel(QWheelEvent*)));
    connect(ui->customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));

}

/*************************************************
Function: ads1114_getvoltage
Description:转换ads1114的值为电压值
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:ads1114的ad值
Output: \
Return: 返回转换后的电压值
Others:
*************************************************/
float MainWindow::ads1114_getvoltage(qint16 advalue)
{
    return ((float)advalue/32767)*2.048;
}
/*************************************************
Function: ads114_turntoI
Description:把ads1114采集来的电压值转换为电流值
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:ads1114的电压值
Output: \
Return: 返回转换后电压值，单位na
Others:
*************************************************/
float MainWindow::ads114_turntoI(float voltage)
{
    return voltage*100;
}
/*************************************************
Function: saadc_getvoTemVoltage
Description:转换温度采集到的AD值为电压值
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:ads1114的电压值
Output: \
Return: 返回转换后电压值
Others:
*************************************************/
float MainWindow::saadc_getvoTemVoltage(quint16 advalue, float bat)
{
    return (float)advalue/4096*bat;
}
/*************************************************
Function: saadc_getvoltage
Description:转换saadc的值为电压值
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:saadc的ad值
Output: \
Return: 返回转换后的电压值
Others:
*************************************************/
float MainWindow::saadc_getvoltage(quint16 advalue)
{
    return ((float)advalue*3.6)/4096;

}
/*************************************************
Function: saadc_tempertureRData
Description:返回温度电阻的电阻值
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:saadc的ad值
Output: \
Return: 温度电阻的电阻值
Others:
*************************************************/
float MainWindow::saadc_tempertureRData(quint16 adValue)
{
    float sp;
    sp = (float)adValue/4096;
    return 100*sp/(1-sp);
}
/*************************************************
Function: saadc_tempertureValue
Description:返回温度电阻的温度值，采用matlab拟合的公式
Calls: \
Called By:
Table Accessed: \
Table Updated: \
Input:温度电阻的电阻值
Output: \
Return: 温度电阻的温度值
Others:
*************************************************/
float MainWindow::saadc_tempertureValue(float Rdata)
{
    return 341.8*pow(Rdata,-0.1298)-163.0;
}

void MainWindow::mousePress(QMouseEvent* event)
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
      ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->xAxis->orientation());
    else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
      ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->yAxis->orientation());
    else
      ui->customPlot->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}


void MainWindow::mouseWheel(QWheelEvent* event)
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
      ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->xAxis->orientation());
//      qDebug("file:%s fun:%s line:%d mouse wheel111",__FILE__,__FUNCTION__,__LINE__);
    }
    //
    else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    {
       // ui->customPlot->axisRect()->setRangeZoomAxes(ui->customPlot->yAxis2,ui->customPlot->yAxis2);
        ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->yAxis->orientation());
     //  ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->yAxis2->orientation());
     // ui->customPlot->axisRect()->setRangeDragAxes(ui->customPlot->yAxis2);
     // double dCenter = ui->customPlot->yAxis2->range().center();
    //  ui->customPlot->yAxis2->scaleRange(2,dCenter);
 //     qDebug("file:%s fun:%s line:%d mouse wheel222",__FILE__,__FUNCTION__,__LINE__);
    }
    else
    {
      ui->customPlot->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
 //     qDebug("file:%s fun:%s line:%d mouse wheel333",__FILE__,__FUNCTION__,__LINE__);
    }
}

void MainWindow::selectionChanged()
{

    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
        ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
      ui->customPlot->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
      ui->customPlot->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
      ui->customPlot->axisRect()->setRangeZoomAxes(ui->customPlot->xAxis,ui->customPlot->xAxis);
      ui->customPlot->axisRect()->setRangeDragAxes(ui->customPlot->xAxis,ui->customPlot->xAxis);
    }
    else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
    //  ui->customPlot->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
      ui->customPlot->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
      ui->customPlot->axisRect()->setRangeZoomAxes(ui->customPlot->yAxis,ui->customPlot->yAxis);
      ui->customPlot->axisRect()->setRangeDragAxes(ui->customPlot->yAxis,ui->customPlot->yAxis);
    }
    else if(ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        ui->customPlot->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
        ui->customPlot->axisRect()->setRangeZoomAxes(ui->customPlot->yAxis2,ui->customPlot->yAxis2);
        ui->customPlot->axisRect()->setRangeDragAxes(ui->customPlot->yAxis2,ui->customPlot->yAxis2);
    }
    else
    {
        ui->customPlot->axisRect()->setRangeZoomAxes(ui->customPlot->xAxis,ui->customPlot->yAxis);
        ui->customPlot->axisRect()->setRangeDragAxes(ui->customPlot->xAxis,ui->customPlot->yAxis);
    }
    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i=0; i<ui->customPlot->graphCount(); ++i)
    {
      QCPGraph *graph = ui->customPlot->graph(i);
      QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
      if (item->selected() || graph->selected())
      {
        item->setSelected(true);
        graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
      }
    }

}

cgm_dat MainWindow::get_infomationFromtext_phone(QByteArray &line, quint64 line_num)
{
    cgm_dat data;
    QString lin(line);
    QRegExp info(tr("\\bNotification\\b"));
    int pos(0);

    if(pos = info.indexIn(lin,pos)!=-1)
    {
        info.setPattern("\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}-\\w{2}");
        if(pos = info.indexIn(lin,pos)!=-1)
        {
            quint8 hi,lo;

            QString dat = info.capturedTexts().at(0);
            //血糖信息
            hi = twochartohex(dat.at(21).toLatin1(),dat.at(22).toLatin1());
            lo = twochartohex(dat.at(18).toLatin1(),dat.at(19).toLatin1());
            data.glucose = hi<<8|lo;

            if(data.glucose<0)
            {
                data.glucose = 0-data.glucose;
            }
            data.glucose_voltage = ads1114_getvoltage(data.glucose);
            data.glucose_I = ads114_turntoI(data.glucose_voltage);
            glucose_axis.append((double)data.glucose_I);

            //电量信息
            hi = twochartohex(dat.at(15).toLatin1(),dat.at(16).toLatin1());
            lo = twochartohex(dat.at(12).toLatin1(),dat.at(13).toLatin1());
            data.bat = hi<<8|lo;

            if(data.bat<0)
            {
                data.bat = 0-data.bat;
            }
            data.bat_voltage = saadc_getvoltage(data.bat);
            bat_axis.append((double)data.bat_voltage);
            //温度信息
            hi = twochartohex(dat.at(9).toLatin1(),dat.at(10).toLatin1());
            lo = twochartohex(dat.at(6).toLatin1(),dat.at(7).toLatin1());
            data.temperature = hi<<8|lo;

            if(data.temperature < 0)
            {
                data.temperature = 0-data.temperature;
            }
            data.temperature_voltage = saadc_getvoltage(data.temperature);
            data.temperature_R = saadc_tempertureRData(data.temperature);
            data.temperatureValue = saadc_tempertureValue(data.temperature_R);
            temperature_axis.append((double)data.temperature_voltage);
            //时间信息
            quint8 t1,t2,t3,t4;
            t4 = twochartohex(dat.at(24).toLatin1(),dat.at(25).toLatin1());
            t3 = twochartohex(dat.at(27).toLatin1(),dat.at(28).toLatin1());
            t2 = twochartohex(dat.at(30).toLatin1(),dat.at(31).toLatin1());
            t1 = twochartohex(dat.at(33).toLatin1(),dat.at(34).toLatin1());
            time_t tim;
            tim = (t1<<24)|(t2<<16)|(t3<<8)|t4;
            QDateTime dati = QDateTime::fromTime_t(tim);
            ui->data_table->setRowCount(ui->data_table->rowCount()+1);

            ui->data_table->setItem(ui->data_table->rowCount()-1,0,new QTableWidgetItem(dati.toString("yyyy:MM:dd:hh:mm:ss")));
            ui->data_table->setItem(ui->data_table->rowCount()-1,1,new QTableWidgetItem(QString::number(data.glucose)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,2,new QTableWidgetItem(QString::number(data.glucose_voltage)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,3,new QTableWidgetItem(QString::number(data.glucose_I)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,4,new QTableWidgetItem(QString::number(data.temperature)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,5,new QTableWidgetItem(QString::number(data.temperature_voltage)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,6,new QTableWidgetItem(QString::number(data.temperature_R)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,7,new QTableWidgetItem(QString::number(data.temperatureValue)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,8,new QTableWidgetItem(QString::number(data.bat)));
            ui->data_table->setItem(ui->data_table->rowCount()-1,9,new QTableWidgetItem(QString::number(data.bat_voltage)));
           // qDebug()<<dat<<line_num<< data.glucose_I;
        }
       // qDebug()<<info.capturedTexts().at(0)<<line_num;
    }
    else
    {
        return data;
    }

}

void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
    double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
    time_t tm = (time_t)(time_axis.at(dataIndex));

    QDateTime time_ax  = QDateTime::fromTime_t(tm);

    QString message = QString("图像: '%1' x值: %2 y值:  %3.").arg(plottable->name()).arg(time_ax.toString("yyyy:MM:dd-hh:mm:ss")).arg(dataValue);
    ui->statusBar->showMessage(message,3000);
}

void MainWindow::on_save_data_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save as..."),QString(),tr("EXCEL files (*.xls *.xlsx);;text-Files (*.txt);;"));

  //  qDebug()<<filePath;

    if(filePath.length() == 0)
    {
        return;
    }
    int row = ui->data_table->rowCount();
    int col = ui->data_table->columnCount();
    QList<QString> list;
    //添加列标题
    QString HeaderRow;
    for(int i=0;i<col;i++)
    {
       HeaderRow.append(ui->data_table->horizontalHeaderItem(i)->text()+"\t");
    }
    list.push_back(HeaderRow);
    for(int i=0;i<row;i++)
    {
        QString rowStr = "";
        for(int j=0;j<col;j++)
        {
            rowStr += ui->data_table->item(i,j)->text() + "\t";
        }
     list.push_back(rowStr);
     }
     QTextEdit textEdit;
     for(int i=0;i<list.size();i++)
     {
         textEdit.append(list.at(i));
     }

     QFile file(filePath);
     if(file.open(QFile::WriteOnly | QIODevice::Text))
     {
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        ts<<textEdit.document()->toPlainText();
        file.close();
     }
}


void MainWindow::on_actionAbout_triggered()
{
    QString message = tr("LogReader")+tr("\n")+tr("Copyright Sisensing")+tr("\n")+tr("v1.1.1");
   QMessageBox::about(this,tr("About"),message);
}

void MainWindow::on_actionSet_triggered()
{
    QStringList typeitems;
    typeitems<<tr("Nordic Dongle Log")<<tr("Ti Dongle Log")<<tr("Phone Log");

    bool ok;
    QString typeitem = QInputDialog::getItem(this,tr("文件类型选择"),tr("请选择文件类型："),typeitems,0,false,&ok);
    if(ok&!typeitem.isEmpty())
    {
        if(typeitem == tr("Nordic Dongle Log"))
        {
            logfile_type = 0;
        }
        else if(typeitem == tr("Ti Dongle Log"))
        {
           logfile_type = 1;
        }
        else if(typeitem == tr("Phone Log"))
        {
            logfile_type = 2;
        }
    }
    qDebug()<<typeitem;
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_checkBat_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(1)->setVisible(true);
            ui->customPlot->replot();
        }
    }
    else
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(1)->setVisible(false);
            ui->customPlot->replot();
        }
    }
}

void MainWindow::on_checkTemperature_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(2)->setVisible(true);
            ui->customPlot->replot();
        }

    }
    else
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(2)->setVisible(false);
            ui->customPlot->replot();
        }
    }
}

void MainWindow::on_checkGlucose_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(0)->setVisible(true);
            ui->customPlot->replot();
        }
    }
    else
    {
        if(ui->customPlot->graphCount() > 0)
        {
            ui->customPlot->graph(0)->setVisible(false);
            ui->customPlot->replot();
        }
    }
}

void MainWindow::on_pushButton_resetTimeAxis_clicked()
{
   // timeticker->setDateTimeSpec(Qt::LocalTime);
    if(time_axis.count() > 0)
    {
        QSharedPointer <QCPAxisTickerDateTime> timeticker(new QCPAxisTickerDateTime);
        timeticker->setDateTimeFormat("MM:dd:hh:mm:ss");
        ui->customPlot->xAxis->setTicker(timeticker);

        QDateTime temp;
        qint32 timeNum = time_axis.count();

        temp = ui->dateTimeEdit_startTime->dateTime();


        time_axis.clear();
        for(int i =0;i < timeNum ;i++)
        {
            time_axis.append(temp.toMSecsSinceEpoch()/1000);
            temp = temp.addSecs(ui->spinBox_space->value());
        }

        ui->customPlot->graph(0)->setData(time_axis,glucose_axis);
        ui->customPlot->graph(1)->setData(time_axis,bat_axis);
        ui->customPlot->graph(2)->setData(time_axis,temperature_axis);
        ui->customPlot->graph(0)->rescaleAxes(true);
        ui->customPlot->graph(1)->rescaleAxes(true);
        ui->customPlot->graph(2)->rescaleAxes(true);
        ui->customPlot->xAxis->setRange(time_axis.at(0),time_axis.at(timeNum-1));
        //ui->customPlot->xAxis->setNumberFormat("");
        ui->customPlot->xAxis->setTickLabelRotation(30);
        ui->customPlot->replot();
    }

   // ui->customPlot->xAxis->setNumberFormat();
}
