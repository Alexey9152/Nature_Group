#include "mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QAudioOutput>

void ClockWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
    painter.setWindow(-100, -100, 200, 200);

    // Рисуем циферблат
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    painter.drawEllipse(-90, -90, 180, 180);

    // Рисуем часовые метки
    painter.setPen(QPen(Qt::black, 2));
    for (int i = 0; i < 12; ++i) {
        painter.save();
        painter.rotate(i * 30.0);
        painter.drawLine(0, -80, 0, -70);
        painter.restore();
    }

    // Рисуем цифры
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    for (int i = 1; i <= 12; ++i) {
        double angle = i * 30.0 * M_PI / 180.0;
        int x = static_cast<int>(72 * sin(angle));
        int y = static_cast<int>(-72 * cos(angle));
        painter.drawText(x - 10, y - 10, 20, 20, Qt::AlignCenter, QString::number(i));
    }

    // Часовая стрелка
    painter.save();
    painter.rotate(30.0 * (m_time.hour() + m_time.minute() / 60.0));
    painter.setPen(QPen(Qt::black, 4));
    painter.drawLine(0, 0, 0, -40);
    painter.restore();

    // Минутная стрелка
    painter.save();
    painter.rotate(6.0 * (m_time.minute() + m_time.second() / 60.0));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(0, 0, 0, -60);
    painter.restore();

    // Секундная стрелка
    painter.save();
    painter.rotate(6.0 * m_time.second());
    painter.setPen(QPen(Qt::red, 1));
    painter.drawLine(0, 0, 0, -70);
    painter.restore();

    // Центральная точка
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::red);
    painter.drawEllipse(-3, -3, 6, 6);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("Круглые Часы со Звуком");

    // Создаем виджет часов
    clockWidget = new ClockWidget;

    // Создаем элементы управления
    hourSpin = new QSpinBox;
    hourSpin->setRange(0, 23);
    hourSpin->setValue(QTime::currentTime().hour());
    hourSpin->setPrefix("Ч: ");

    minuteSpin = new QSpinBox;
    minuteSpin->setRange(0, 59);
    minuteSpin->setValue(QTime::currentTime().minute());
    minuteSpin->setPrefix("М: ");

    secondSpin = new QSpinBox;
    secondSpin->setRange(0, 59);
    secondSpin->setValue(QTime::currentTime().second());
    secondSpin->setPrefix("С: ");

    QPushButton *setTimeButton = new QPushButton("Установить время");

    // Группируем элементы управления
    QGroupBox *controlGroup = new QGroupBox("Установка времени");
    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(hourSpin);
    controlLayout->addWidget(minuteSpin);
    controlLayout->addWidget(secondSpin);
    controlLayout->addWidget(setTimeButton);
    controlGroup->setLayout(controlLayout);

    // Основной лейаут
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(clockWidget);
    mainLayout->addWidget(controlGroup);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Настраиваем таймер
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000);

    // Таймеры для боя часов и кукушки
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
            cuckooPlayer->play();
            currentCuckoo++;
        } else {
            cuckooTimer->stop();
        }
    });

    // Подключаем кнопку
    connect(setTimeButton, &QPushButton::clicked, this, &MainWindow::setCustomTime);

    // Инициализация звуковых эффектов

    tickPlayer   = new QMediaPlayer(this);
    tockPlayer   = new QMediaPlayer(this);
    gongPlayer   = new QMediaPlayer(this);
    cuckooPlayer = new QMediaPlayer(this);

    // Локальные файлы
    tickPlayer->setSource(QUrl("qrc:/sounds/tick.wav"));
    tockPlayer->setSource(QUrl("qrc:/sounds/tock.wav"));
    gongPlayer->setSource(QUrl("qrc:/sounds/gong.wav"));
    cuckooPlayer->setSource(QUrl("qrc:/sounds/cuckoo.wav"));

    QAudioOutput *audioOutput;

    // В конструкторе MainWindow после создания mediaPlayer:
    audioOutput     = new QAudioOutput(this);
    tickPlayer    ->setAudioOutput(audioOutput);
    tockPlayer    ->setAudioOutput(audioOutput);
    gongPlayer    ->setAudioOutput(audioOutput);
    cuckooPlayer  ->setAudioOutput(audioOutput);

    connect(tickPlayer, &QMediaPlayer::errorOccurred, this, [](auto err){
        qDebug() << "Ошибка QMediaPlayer:" << err;
    });
    connect(tickPlayer, &QMediaPlayer::mediaStatusChanged, this, [](auto st){
        qDebug() << "Статус media:" << st;
    });


}

MainWindow::~MainWindow() {
    delete tickPlayer;
    delete tockPlayer;
    delete gongPlayer;
    delete cuckooPlayer;
}

void MainWindow::updateTime() {
    QTime currentTime = QTime::currentTime();

    if (!useCustomTime) {
        clockWidget->setTime(currentTime);
    }

    // Определяем тип звука для текущей секунды
    int second = currentTime.second();
    int minute = currentTime.minute();
    int hour = currentTime.hour();

    // Тикающий звук каждую секунду
    if (!chimeTimer->isActive() && !cuckooTimer->isActive()) {
        if (second % 2 == 0) {
            // "Тик" для четных секунд
            if (tickPlayer->source().isEmpty()) {
                qDebug() << "Тик!";
            } else {
                tickPlayer->play();
            }
        } else {
            // "Так" для нечетных секунд
            if (tockPlayer->source().isEmpty()) {
                qDebug() << "Так!";
            } else {
                tockPlayer->play();
            }
        }
    }

    // Проверяем начало часа (00 минут 00 секунд)
    if (minute == 0 && second == 0) {
        if (hour == 0 || hour == 12) {
            // Кукушка в полночь и полдень
            playCuckoo(hour == 0 ? 24 : 12);
        } else {
            // Бой часов
            playClockChime(hour);
        }
    }
}

void MainWindow::setCustomTime() {
    QTime customTime(
        hourSpin->value(),
        minuteSpin->value(),
        secondSpin->value()
        );

    if (customTime.isValid()) {
        useCustomTime = true;
        clockWidget->setTime(customTime);

        // Обновляем спинбоксы
        hourSpin->setValue(customTime.hour());
        minuteSpin->setValue(customTime.minute());
        secondSpin->setValue(customTime.second());

        // Проверяем, нужно ли воспроизвести звук
        if (customTime.minute() == 0 && customTime.second() == 0) {
            if (customTime.hour() == 0 || customTime.hour() == 12) {
                playCuckoo(customTime.hour() == 0 ? 24 : 12);
            } else {
                playClockChime(customTime.hour());
            }
        }
    }
}

void MainWindow::playClockChime(int hour) {
    chimeCount = hour;
    currentChime = 0;
    chimeTimer->start(1000); // Удары с интервалом в 1 секунду

    // Для отладки, если звуки не загружены
    if (gongPlayer->source().isEmpty()) {
        qDebug() << "Бой часов:" << hour << "раз(а)";
    }
}

void MainWindow::playCuckoo(int count) {
    cuckooCount = count;
    currentCuckoo = 0;
    cuckooTimer->start(800); // Кукование с интервалом 0.8 секунды

    // Для отладки, если звуки не загружены
    if (cuckooPlayer->source().isEmpty()) {
        qDebug() << "Ку-ку!" << count << "раз(а)";
    }
}

void MainWindow::playSoundEffect(int type)
{
    switch (type) {
    case 0:
        tickPlayer->play();
        break;
    case 1:
        tockPlayer->play();
        break;
    case 2:
        gongPlayer->play();
        break;
    case 3:
        cuckooPlayer->play();
        break;
    default:
        break;
    }
}
