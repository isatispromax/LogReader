#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFIle>
#include "qcustomplot.h"

struct cgm_dat
{
    qint16 bat;
    qint16 temperature;
    qint16 glucose;

    float bat_voltage;
    float temperature_voltage;
    float glucose_voltage;

    float glucose_I;
    float temperature_R;
    float temperatureValue;
    cgm_dat()
    {
        bat = 0;
        temperature = 0;
        glucose = 0;
        bat_voltage = 0;
        temperature_voltage = 0;
        glucose_voltage = 0;
        glucose_I = 0;
    }
};
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
    void on_actionOpen_triggered();

//    void on_pushButton_clicked();

    void mousePress(QMouseEvent* event);//鼠标左键单击

    void mouseWheel(QWheelEvent* event);//鼠标滚轮

    void selectionChanged();//选择改变

    void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);//单击图像

    void on_save_data_clicked();

    void on_actionAbout_triggered();

    void on_actionSet_triggered();

    void on_actionExit_triggered();

    void on_checkBat_stateChanged(int arg1);

    void on_checkTemperature_stateChanged(int arg1);

    void on_checkGlucose_stateChanged(int arg1);

    void on_pushButton_resetTimeAxis_clicked();

private:
    Ui::MainWindow *ui;

    QVector <double> time_axis;
    QVector <double> glucose_axis;
    QVector <double> bat_axis;
    QVector <double> temperature_axis;

    quint8 logfile_type;

    QFile *logfile;
    cgm_dat get_infomationFromtext(QByteArray &line,quint64 line_num);
    quint8 twochartohex(quint8 hi,quint8 lo);
    void plot_data(void);
    float ads1114_getvoltage(qint16 advalue);
    float ads114_turntoI(float voltage);
    float saadc_getvoltage(quint16 advalue);
    float saadc_getvoTemVoltage(quint16 advalue,float bat);
    float saadc_tempertureRData(quint16 adValue);
    float saadc_tempertureValue(float Rdata);
    cgm_dat get_infomationFromtext_phone(QByteArray &line,quint64 line_num);
};

#endif // MAINWINDOW_H
