// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QSoundEffect>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateClock();
    void playNextChime();
    void on_setTimeButton_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QTimer *chimeTimer;
    QTime currentTime;

    QSoundEffect tickSound;
    QSoundEffect strikeSound;
    QSoundEffect cuckooSound;

    int strikesRemaining;
    bool isHourStrike;
};

#endif // MAINWINDOW_H
