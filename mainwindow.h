#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSpinBox>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <QPixmap>

class ClockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClockWidget(QWidget *parent = nullptr);
    void setTime(const QTime &time);

public slots:
    void showCuckoo();
    void hideCuckoo();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QTime m_time;
    QPixmap m_doorPixmap;
    QPixmap m_cuckooPixmap;
    bool m_cuckooVisible = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void setCustomTime();

private:
    void playClockChime(int hour);
    void playCuckoo(int count);

    // Визуальные компоненты
    ClockWidget *clockWidget;
    QSpinBox *hourSpin;
    QSpinBox *minuteSpin;
    QSpinBox *secondSpin;

    // Таймеры
    QTimer *timer;
    QTimer *chimeTimer;
    QTimer *cuckooTimer;

    // Внутреннее время часов
    QTime m_time;

    // Звуковые эффекты
    QMediaPlayer *tickPlayer;
    QMediaPlayer *tockPlayer;
    QMediaPlayer *gongPlayer;
    QMediaPlayer *cuckooPlayer;

    // Для последовательного воспроизведения боя/кукушки
    int chimeCount = 0;
    int cuckooCount = 0;
    int currentChime = 0;
    int currentCuckoo = 0;
};
#endif // MAINWINDOW_H
