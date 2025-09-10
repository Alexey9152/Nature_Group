#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "analogclock.h"
#include "numericformatwindow.h"
#include "settimewindow.h"
#include "textformatwindow.h"
#include <QMediaPlayer>
#include <QDateTime>
#include <QTimer>
#include <QAudioOutput>
#include "manualinputwindow.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void helpAction();
    void on_action_triggered();
    void stopMusic();
    void handleMusicSelectionAction();

    //фон
    void on_action_7_triggered();
    void on_action1_4_triggered();
    void on_action2_triggered();
    void on_action_8_triggered();
    void on_action4_triggered();
    void on_action5_triggered();
    void on_action6_triggered();
    void on_action8_triggered();

    //стрелки
    void on_action_14_triggered();
    void on_action_15_triggered();
    void on_action_16_triggered();

    //формат
    void on_action_10_triggered();
    void on_action_11_triggered();
    void on_action_12_triggered();

    void onSetTimeTriggered();
    void playSelectedMusic();

private:
    Ui::MainWindow *ui;
    AnalogClock *analogClockWindow;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QString currentMusicFilePath;
    NumericFormatWindow *numericFormatWindow;
    TextFormatWindow *textFormatWindow;
    SetTimeWindow *setTimeWindow;
    ManualInputWindow *manualInputWindow;
};
#endif
