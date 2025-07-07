#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QTimer>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QMediaPlayer>
#include <cmath>
#include <QAudioOutput>

class ClockWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClockWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumSize(300, 300);
    }

    void setTime(const QTime &time) {
        m_time = time;
        update();
    }


protected:
    void paintEvent(QPaintEvent *) override;

private:
    QTime m_time = QTime::currentTime();
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void setCustomTime();
    void playSoundEffect(int type);
    void playClockChime(int hour);
    void playCuckoo(int count);

private:
    ClockWidget *clockWidget;
    QSpinBox *hourSpin;
    QSpinBox *minuteSpin;
    QSpinBox *secondSpin;
    QTimer *timer;
    QTimer *chimeTimer;
    QTimer *cuckooTimer;
    bool useCustomTime = false;

    // Звуковые эффекты
    QMediaPlayer *tickPlayer;
    QMediaPlayer *tockPlayer;
    QMediaPlayer *gongPlayer;
    QMediaPlayer *cuckooPlayer;



    // Для последовательного воспроизведения
    int chimeCount = 0;
    int cuckooCount = 0;
    int currentChime = 0;
    int currentCuckoo = 0;


};
#endif // MAINWINDOW_H
