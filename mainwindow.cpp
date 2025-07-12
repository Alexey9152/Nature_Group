#include "mainwindow.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>


ClockWidget::ClockWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 400);
    m_time = QTime::currentTime();

    if (!m_doorPixmap.load(":/images/door.png")) {
        qDebug() << "ОШИБКА: Не удалось загрузить 'door.png'!";
    }
    if (!m_cuckooPixmap.load(":/images/cuckoo.png")) {
        qDebug() << "ОШИБКА: Не удалось загрузить 'cuckoo.png'!";
    }
}

void ClockWidget::setTime(const QTime &time)
{
    m_time = time;
    update();
}

void ClockWidget::setBackground(const QPixmap &pixmap)
{
    m_backgroundPixmap = pixmap;
    update();
}

void ClockWidget::showCuckoo()
{
    m_cuckooVisible = true;
    update();
}

void ClockWidget::hideCuckoo()
{
    m_cuckooVisible = false;
    update();
}

void ClockWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!m_backgroundPixmap.isNull()) {
        painter.drawPixmap(this->rect(), m_backgroundPixmap);
    }

    int side = qMin(width(), height()) - 20;
    painter.save();
    painter.translate(width() / 2.0, height() / 2.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(QPointF(0, 0), side / 2.0, side / 2.0);

    painter.setPen(QPen(Qt::black, 3));
    painter.drawEllipse(QPointF(0, 0), side / 2.0, side / 2.0);

    for (int i = 0; i < 60; ++i) {
        if (i % 5 == 0) {
            painter.setPen(QPen(Qt::black, 4));
            painter.drawLine(0, -side / 2 + 5, 0, -side / 2 + 20);
        } else {
            painter.setPen(QPen(Qt::gray, 1));
            painter.drawLine(0, -side / 2 + 5, 0, -side / 2 + 10);
        }
        painter.rotate(6.0);
    }
    painter.restore();

    painter.save();
    painter.translate(width() / 2.0, height() / 2.0);

    QPen hourPen(QColor(60, 60, 60), 10, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(hourPen);
    painter.save();
    painter.rotate(30.0 * (m_time.hour() + m_time.minute() / 60.0));
    painter.drawLine(0, 0, 0, -side / 3.5);
    painter.restore();

    QPen minutePen(QColor(40, 40, 40), 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(minutePen);
    painter.save();
    painter.rotate(6.0 * (m_time.minute() + m_time.second() / 60.0));
    painter.drawLine(0, 0, 0, -side / 2.5);
    painter.restore();

    QRectF cuckooRect(-side/10, -side/2 + 10, side/5, side/6);
    if (m_cuckooVisible) {
        painter.drawPixmap(cuckooRect, m_cuckooPixmap, m_cuckooPixmap.rect());
    } else {
        painter.drawPixmap(cuckooRect, m_doorPixmap, m_doorPixmap.rect());
    }

    QPen secondPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(secondPen);
    painter.save();
    painter.rotate(6.0 * m_time.second());
    painter.drawLine(0, 15, 0, -side / 2.2);
    painter.restore();

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawEllipse(QPointF(0,0), 8, 8);

    painter.restore();
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Часы Lipo ");

    m_time = QTime::currentTime();
    clockWidget = new ClockWidget;
    clockWidget->setTime(m_time);


    clockWidget->setBackground(QPixmap(":/images/dali.jpg"));

    hourSpin = new QSpinBox;
    hourSpin->setRange(0, 23);
    minuteSpin = new QSpinBox;
    minuteSpin->setRange(0, 59);
    secondSpin = new QSpinBox;
    secondSpin->setRange(0, 59);

    hourSpin->setValue(m_time.hour());
    minuteSpin->setValue(m_time.minute());
    secondSpin->setValue(m_time.second());

    QPushButton *setTimeButton = new QPushButton("Установить");
    QPushButton *resetTimeButton = new QPushButton("Сбросить");

    QGroupBox *timeGroup = new QGroupBox("Установка времени");
    QHBoxLayout *timeLayout = new QHBoxLayout;
    timeLayout->addWidget(new QLabel("Ч:"));
    timeLayout->addWidget(hourSpin);
    timeLayout->addWidget(new QLabel("М:"));
    timeLayout->addWidget(minuteSpin);
    timeLayout->addWidget(new QLabel("С:"));
    timeLayout->addWidget(secondSpin);
    timeLayout->addWidget(setTimeButton);
    timeLayout->addWidget(resetTimeButton);
    timeGroup->setLayout(timeLayout);

    soundsCheckBox = new QCheckBox("Включить звук");
    soundsCheckBox->setChecked(true);
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    QGroupBox *soundGroup = new QGroupBox("Управление звуком");
    QHBoxLayout *soundLayout = new QHBoxLayout;
    soundLayout->addWidget(soundsCheckBox);
    soundLayout->addWidget(new QLabel("Громкость:"));
    soundLayout->addWidget(volumeSlider);
    soundGroup->setLayout(soundLayout);

    QVBoxLayout *controlPanelLayout = new QVBoxLayout;
    controlPanelLayout->addWidget(timeGroup);
    controlPanelLayout->addWidget(soundGroup);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(clockWidget, 1);
    mainLayout->addLayout(controlPanelLayout);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    resize(800, 600);

    tickPlayer = new QMediaPlayer(this);
    tockPlayer = new QMediaPlayer(this);
    gongPlayer = new QMediaPlayer(this);
    cuckooPlayer = new QMediaPlayer(this);
    m_allPlayers << tickPlayer << tockPlayer << gongPlayer << cuckooPlayer;

    for (auto player : m_allPlayers) {
        player->setAudioOutput(new QAudioOutput(this));
    }
    setVolume(volumeSlider->value());

    tickPlayer->setSource(QUrl("qrc:/sounds/tick.wav"));
    tockPlayer->setSource(QUrl("qrc:/sounds/tock.wav"));
    gongPlayer->setSource(QUrl("qrc:/sounds/gong.wav"));
    cuckooPlayer->setSource(QUrl("qrc:/sounds/cuckoo.wav"));

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000);

    chimeTimer = new QTimer(this);
    chimeTimer->setSingleShot(true);
    connect(chimeTimer, &QTimer::timeout, this, &MainWindow::playNextChime);

    cuckooTimer = new QTimer(this);
    cuckooTimer->setSingleShot(true);
    connect(cuckooTimer, &QTimer::timeout, this, &MainWindow::playNextCuckoo);

    connect(setTimeButton, &QPushButton::clicked, this, &MainWindow::setCustomTime);
    connect(resetTimeButton, &QPushButton::clicked, this, &MainWindow::resetToSystemTime);
    connect(soundsCheckBox, &QCheckBox::toggled, this, &MainWindow::toggleSounds);
    connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
}

MainWindow::~MainWindow() {}

void MainWindow::updateTime()
{
    m_time = m_time.addSecs(1);
    clockWidget->setTime(m_time);

    if (soundsCheckBox->isChecked()) {

        if (m_time.second() % 2 == 0) {
            tickPlayer->stop();
            tickPlayer->play();
        } else {
            tockPlayer->stop();
            tockPlayer->play();
        }
    }

    if (m_time.minute() == 0 && m_time.second() == 0) {
        int hour = m_time.hour();
        if (hour == 0) {
            startCuckoo(24);
        } else if (hour == 12) {
            startCuckoo(12);
        } else {
            startClockChime(hour);
        }
    }
}

void MainWindow::setCustomTime()
{
    m_time.setHMS(hourSpin->value(), minuteSpin->value(), secondSpin->value());
    clockWidget->setTime(m_time);
}

void MainWindow::resetToSystemTime()
{
    m_time = QTime::currentTime();
    hourSpin->setValue(m_time.hour());
    minuteSpin->setValue(m_time.minute());
    secondSpin->setValue(m_time.second());
    clockWidget->setTime(m_time);
}

void MainWindow::toggleSounds(bool enabled)
{
    if (!enabled) {
        for(auto player : m_allPlayers) {
            player->stop();
        }
        chimeTimer->stop();
        cuckooTimer->stop();
        clockWidget->hideCuckoo();
    }
}

void MainWindow::setVolume(int volume)
{
    float floatVolume = static_cast<float>(volume) / 100.0f;
    for(auto player : m_allPlayers) {
        player->audioOutput()->setVolume(floatVolume);
    }
}

void MainWindow::startClockChime(int hour)
{
    if (!soundsCheckBox->isChecked()) return;
    chimeCount = hour;
    currentChime = 0;
    playNextChime();
}

void MainWindow::playNextChime()
{
    if (currentChime >= chimeCount || !soundsCheckBox->isChecked()) return;


    gongPlayer->stop();
    gongPlayer->play();

    currentChime++;
    if (currentChime < chimeCount) {
        chimeTimer->start(1500);
    }
}

void MainWindow::startCuckoo(int count)
{
    if (!soundsCheckBox->isChecked()) return;
    cuckooCount = count;
    currentCuckoo = 0;
    playNextCuckoo();
}

void MainWindow::playNextCuckoo()
{
    if (currentCuckoo >= cuckooCount || !soundsCheckBox->isChecked()) return;

    clockWidget->showCuckoo();


    cuckooPlayer->stop();
    cuckooPlayer->play();

    currentCuckoo++;

    QTimer::singleShot(400, this, [this](){ clockWidget->hideCuckoo(); });

    if (currentCuckoo < cuckooCount) {
        cuckooTimer->start(800);
    }
}
