#include "mainwindow.h"
#include <QDebug>

// ===================================================================
// Реализация ClockWidget
// ===================================================================

ClockWidget::ClockWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 300);
    m_time = QTime::currentTime();

    if (!m_doorPixmap.load(":/images/door.png")) {
        qDebug() << "ОШИБКА: Не удалось загрузить изображение 'door.png'!";
    }
    if (!m_cuckooPixmap.load(":/images/cuckoo.png")) {
        qDebug() << "ОШИБКА: Не удалось загрузить изображение 'cuckoo.png'!";
    }
}

void ClockWidget::setTime(const QTime &time)
{
    m_time = time;
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
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
    painter.setWindow(-100, -100, 200, 200);

    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    painter.drawEllipse(-90, -90, 180, 180);

    painter.setPen(QPen(Qt::black, 2));
    for (int i = 0; i < 12; ++i) {
        painter.save();
        painter.rotate(i * 30.0);
        painter.drawLine(0, -80, 0, -70);
        painter.restore();
    }

    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    for (int i = 1; i <= 12; ++i) {
        double angle = i * 30.0 * M_PI / 180.0;
        int x = static_cast<int>(60 * sin(angle));
        int y = static_cast<int>(-60 * cos(angle));
        painter.drawText(x - 10, y - 10, 20, 20, Qt::AlignCenter, QString::number(i));
    }

    painter.save();
    painter.rotate(30.0 * (m_time.hour() + m_time.minute() / 60.0));
    painter.setPen(QPen(Qt::black, 4));
    painter.drawLine(0, 0, 0, -40);
    painter.restore();

    painter.save();
    painter.rotate(6.0 * (m_time.minute() + m_time.second() / 60.0));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(0, 0, 0, -60);
    painter.restore();

    painter.save();
    painter.rotate(6.0 * m_time.second());
    painter.setPen(QPen(Qt::red, 1));
    painter.drawLine(0, 0, 0, -70);
    painter.restore();

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::red);
    painter.drawEllipse(-3, -3, 6, 6);

    QRectF cuckooRect(-20, -85, 40, 30);
    if (m_cuckooVisible) {
        painter.drawPixmap(cuckooRect, m_cuckooPixmap, m_cuckooPixmap.rect());
    } else {
        painter.drawPixmap(cuckooRect, m_doorPixmap, m_doorPixmap.rect());
    }
}

// ===================================================================
// Реализация MainWindow
// ===================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Круглые Часы со Звуком и Кукушкой");

    m_time = QTime::currentTime();
    clockWidget = new ClockWidget;
    clockWidget->setTime(m_time);

    hourSpin = new QSpinBox;
    hourSpin->setRange(0, 23);
    hourSpin->setPrefix("Ч: ");
    hourSpin->setValue(m_time.hour());

    minuteSpin = new QSpinBox;
    minuteSpin->setRange(0, 59);
    minuteSpin->setPrefix("М: ");
    minuteSpin->setValue(m_time.minute());

    secondSpin = new QSpinBox;
    secondSpin->setRange(0, 59);
    secondSpin->setPrefix("С: ");
    secondSpin->setValue(m_time.second());

    QPushButton *setTimeButton = new QPushButton("Установить время");

    QGroupBox *controlGroup = new QGroupBox("Установка времени");
    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(hourSpin);
    controlLayout->addWidget(minuteSpin);
    controlLayout->addWidget(secondSpin);
    controlLayout->addWidget(setTimeButton);
    controlGroup->setLayout(controlLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(clockWidget);
    mainLayout->addWidget(controlGroup);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    tickPlayer = new QMediaPlayer(this);
    tockPlayer = new QMediaPlayer(this);
    gongPlayer = new QMediaPlayer(this);
    cuckooPlayer = new QMediaPlayer(this);

    tickPlayer->setAudioOutput(new QAudioOutput(this));
    tockPlayer->setAudioOutput(new QAudioOutput(this));
    gongPlayer->setAudioOutput(new QAudioOutput(this));
    cuckooPlayer->setAudioOutput(new QAudioOutput(this));

    tickPlayer->setSource(QUrl("qrc:/sounds/tick.wav"));
    tockPlayer->setSource(QUrl("qrc:/sounds/tock.wav"));
    gongPlayer->setSource(QUrl("qrc:/sounds/gong.wav"));
    cuckooPlayer->setSource(QUrl("qrc:/sounds/cuckoo.wav"));

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000);

    chimeTimer = new QTimer(this);
    connect(chimeTimer, &QTimer::timeout, this, [this]() {
        if (currentChime < chimeCount) {
            gongPlayer->play();
            currentChime++;
        } else {
            chimeTimer->stop();
        }
    });

    cuckooTimer = new QTimer(this);
    connect(cuckooTimer, &QTimer::timeout, this, [this]() {
        if (currentCuckoo < cuckooCount) {
            clockWidget->showCuckoo();
            cuckooPlayer->play();
            currentCuckoo++;

            QTimer::singleShot(400, this, [this](){
                clockWidget->hideCuckoo();
            });
        } else {
            cuckooTimer->stop();
        }
    });

    connect(setTimeButton, &QPushButton::clicked, this, &MainWindow::setCustomTime);
}

MainWindow::~MainWindow()
{
}

void MainWindow::updateTime()
{
    m_time = m_time.addSecs(1);
    clockWidget->setTime(m_time);

    int second = m_time.second();
    int minute = m_time.minute();
    int hour = m_time.hour();

    // ИСПРАВЛЕНИЕ: Принудительно останавливаем плеер перед каждым воспроизведением,
    // чтобы сбросить его состояние и избежать двойного звука.
    if (second % 2 == 0) {
        tickPlayer->stop();
        tickPlayer->play();
    } else {
        tockPlayer->stop();
        tockPlayer->play();
    }

    if (minute == 0 && second == 0) {
        if (hour == 0 || hour == 12) {
            playCuckoo(12);
        } else {
            playClockChime(hour % 12);
        }
    }
}

void MainWindow::setCustomTime()
{
    m_time.setHMS(hourSpin->value(), minuteSpin->value(), secondSpin->value());
    clockWidget->setTime(m_time);

    if (m_time.minute() == 0 && m_time.second() == 0) {
        int hour = m_time.hour();
        if (hour == 0 || hour == 12) {
            playCuckoo(12);
        } else {
            playClockChime(hour % 12);
        }
    }
}

void MainWindow::playClockChime(int hour)
{
    clockWidget->hideCuckoo();
    chimeCount = (hour == 0) ? 12 : hour;
    currentChime = 0;
    chimeTimer->start(1000);
}

void MainWindow::playCuckoo(int count)
{
    clockWidget->hideCuckoo();
    cuckooCount = count;
    currentCuckoo = 0;
    cuckooTimer->start(800);
}
