#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>
#include <QUrl>
#include <QDebug>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "manualinputwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , analogClockWindow(new AnalogClock())
    , audioOutput(new QAudioOutput(this))
    , numericFormatWindow(nullptr)
    , textFormatWindow(nullptr)
    , setTimeWindow(nullptr)
    , manualInputWindow(nullptr)
    , mediaPlayer(new QMediaPlayer(this))
{
    qDebug() << "MainWindow constructor called!";
    ui->setupUi(this);

    mediaPlayer->setAudioOutput(audioOutput);

    ui->menubar->addAction("Установить время", this, &MainWindow::onSetTimeTriggered);


    connect(ui->action, &QAction::triggered, this, &MainWindow::on_action_triggered);
    connect(ui->action_2, &QAction::triggered, this, &MainWindow::close);

    connect(ui->action3, &QAction::triggered, this, &MainWindow::handleMusicSelectionAction);
    connect(ui->action_13, &QAction::triggered, this, &MainWindow::helpAction);

    connect(ui->action_7, &QAction::triggered, this, &MainWindow::on_action_7_triggered);
    connect(ui->action1_4, &QAction::triggered, this, &MainWindow::on_action1_4_triggered);
    connect(ui->action2, &QAction::triggered, this, &MainWindow::on_action2_triggered);
    connect(ui->action_8, &QAction::triggered, this, &MainWindow::on_action_8_triggered);
    connect(ui->action4, &QAction::triggered, this, &MainWindow::on_action4_triggered);
    connect(ui->action5, &QAction::triggered, this, &MainWindow::on_action5_triggered);
    connect(ui->action6, &QAction::triggered, this, &MainWindow::on_action6_triggered);
    connect(ui->action8, &QAction::triggered, this, &MainWindow::on_action8_triggered);

    connect(ui->action_14, &QAction::triggered, this, &MainWindow::on_action_14_triggered);
    connect(ui->action_15, &QAction::triggered, this, &MainWindow::on_action_15_triggered);
    connect(ui->action_16, &QAction::triggered, this, &MainWindow::on_action_16_triggered);

    connect(ui->action_10, &QAction::triggered, this, &MainWindow::on_action_10_triggered);
    connect(ui->action_11, &QAction::triggered, this, &MainWindow::on_action_11_triggered);
    connect(ui->action_12, &QAction::triggered, this, &MainWindow::on_action_12_triggered);

    connect(analogClockWindow, &AnalogClock::clockWindowClosed, this, &MainWindow::stopMusic);
    connect(analogClockWindow, &AnalogClock::windowShown, this, &MainWindow::playSelectedMusic);
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow destructor called!";
    delete ui;
    delete analogClockWindow;
    delete mediaPlayer;
    delete audioOutput;
    if (numericFormatWindow) delete numericFormatWindow;
    if (textFormatWindow) delete textFormatWindow;
    if (setTimeWindow) delete setTimeWindow;
    if (manualInputWindow) delete manualInputWindow;
}

void MainWindow::onSetTimeTriggered()
{
    if (!setTimeWindow) {
        setTimeWindow = new SetTimeWindow(this);
    }

    if (setTimeWindow->exec() == QDialog::Accepted) {
        QTime newTime = setTimeWindow->getTime();
        QDateTime currentDateTime = QDateTime::currentDateTime();
        currentDateTime.setTime(newTime);

        analogClockWindow->setCustomDateTime(currentDateTime);
        analogClockWindow->setUseSystemTime(false);

        if (numericFormatWindow) {
            numericFormatWindow->setCustomDateTime(currentDateTime);
        }
        if (textFormatWindow) {
            textFormatWindow->setCustomDateTime(currentDateTime);
        }
    }
}

void MainWindow::helpAction()
{
    QMessageBox::about(this, "О программе", "Программа предназначена для создания часов\n\n"
"Часы\n\n"
"1. предусмотрен ввод даты и времени в формате словесного описания,\nнапример, Пятнадцатого июля две тысячи двадцать пятого года Двадцать два часа пятнадцать минут сорок пять секунд. с последующим выводом стандартного формата даты и времени (15.07.2025 22:15:45).\n"
"2. Возможность взятия даты и времени из таймера для автоматического ввода с последующим выводом в формате словесного описания.\n"
"З. Возможностью ввода даты и времени в стандартном формате с последующим выводом в формате словесного описания.\n"
"4. Установка времени с последующим отображением выбранного времени на часах.\n"
"5. Можно изменять фон часов, стиль стрелок и устанавливать фоновую музыку.\n"
"Программа предусматривает проверку правильности ввода даты и времени. В случае неверного формата сообщается об ошибке.");
}


void MainWindow::on_action_triggered()
{
    analogClockWindow->show();
    analogClockWindow->raise();
    analogClockWindow->activateWindow();
}

void MainWindow::stopMusic()
{
    if (mediaPlayer && mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer->stop();
        qDebug() << "Music stopped.";
    }
}

void MainWindow::handleMusicSelectionAction()
{
    qDebug() << "handleMusicSelectionAction() called!";
    QString filePath = QFileDialog::getOpenFileName(this, "Выбрать музыкальный файл", "", "Аудио файлы (*.mp3 *.wav *.ogg);;Все файлы (*)");
    if (!filePath.isEmpty()) {
        currentMusicFilePath = filePath;
        qDebug() << "Music file selected: " << currentMusicFilePath;
    } else {
        qDebug() << "File dialog cancelled or no file selected.";
    }
}

void MainWindow::on_action_7_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/dali_clock.jpeg");
        analogClockWindow->setDaliMode(true);
    }
}

void MainWindow::on_action1_4_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background1.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action2_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background2.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action_8_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background3.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action4_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background4.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action5_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background5.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action6_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background6.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action8_triggered()
{
    if (analogClockWindow) {
        analogClockWindow->setBackgroundImage(":/images/background7.jpg");
        analogClockWindow->setDaliMode(false);
    }
}

void MainWindow::on_action_14_triggered()
{
    if (analogClockWindow) analogClockWindow->setHandStyle(AnalogClock::HandStyle::Standard);
}

void MainWindow::on_action_15_triggered()
{
    if (analogClockWindow) analogClockWindow->setHandStyle(AnalogClock::HandStyle::Thin);
}

void MainWindow::on_action_16_triggered()
{
    if (analogClockWindow) analogClockWindow->setHandStyle(AnalogClock::HandStyle::Massive);
}

void MainWindow::on_action_10_triggered()
{
    if (!textFormatWindow) {
        textFormatWindow = new TextFormatWindow(this);
    }
    textFormatWindow->show();
    textFormatWindow->raise();
    textFormatWindow->activateWindow();
}

void MainWindow::on_action_11_triggered()
{
    if (!numericFormatWindow) {
        numericFormatWindow = new NumericFormatWindow(this, NumericFormatWindow::ClockMode);
    } else {
        numericFormatWindow->setMode(NumericFormatWindow::ClockMode);
    }
    numericFormatWindow->show();
    numericFormatWindow->raise();
    numericFormatWindow->activateWindow();
}

void MainWindow::on_action_12_triggered()
{
    if (!manualInputWindow) {
        manualInputWindow = new ManualInputWindow(this);
    }
    manualInputWindow->show();
    manualInputWindow->raise();
    manualInputWindow->activateWindow();
}

void MainWindow::playSelectedMusic()
{
    if (!currentMusicFilePath.isEmpty() && mediaPlayer) {
        if (mediaPlayer->source().isEmpty() || mediaPlayer->source().toLocalFile() != currentMusicFilePath) {
            mediaPlayer->setSource(QUrl::fromLocalFile(currentMusicFilePath));
        }
        if (mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
            mediaPlayer->play();
            qDebug() << "Music started playing because clock window was shown.";
        }
    }
}
