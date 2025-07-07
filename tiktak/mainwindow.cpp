// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QPixmap>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setStyleSheet("QMainWindow { background-image: url(:/images/dali.jpg); background-repeat: no-repeat; background-position: center; }");
    setWindowTitle("Часы");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateClock);
    timer->start(1000);

    currentTime = QTime::currentTime();
    ui->timeEdit->setTime(currentTime);

    tickSound.setSource(QUrl("qrc:/sounds/tick.wav"));
    tickSound.setLoopCount(1);
    tickSound.setVolume(0.5);

    strikeSound.setSource(QUrl("qrc:/sounds/strike.wav"));
    strikeSound.setLoopCount(1);
    strikeSound.setVolume(0.8);

    cuckooSound.setSource(QUrl("qrc:/sounds/cuckoo.wav"));
    cuckooSound.setLoopCount(1);
    cuckooSound.setVolume(0.8);

    chimeTimer = new QTimer(this);
    chimeTimer->setInterval(1000); //  секунды между ударами
    connect(chimeTimer, &QTimer::timeout, this, &MainWindow::playNextChime);

    strikesRemaining = 0;
    isHourStrike = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPixmap background(":/images/dali.jpg");
    painter.drawPixmap(0, 0, width(), height(), background);

    int clockSize = qMin(width(), height() - 150); // Увеличили отступ снизу
    painter.translate(width() / 2, (height() - 150) / 2); // Сместили часы выше
    painter.scale(clockSize / 200.0, clockSize / 200.0);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    for (int i = 1; i <= 12; ++i) {
        double angle = (i - 3) * 30.0 * M_PI / 180.0;
        int x = static_cast<int>(80 * cos(angle));
        int y = static_cast<int>(80 * sin(angle));

        // Для цифр 4-8 уменьшаем радиус и смещаем ближе к центру
        if (i >= 3 && i <= 9) {
            x = static_cast<int>(60 * cos(angle));  // Уменьшенный радиус
            y = static_cast<int>(60 * sin(angle));
        }

        painter.drawText(x - 10, y + 5, 20, 20, Qt::AlignCenter, QString::number(i));
    }

    painter.setPen(Qt::white);
    painter.drawEllipse(-90, -90, 180, 180);

    painter.save();
    painter.rotate(30.0 * (currentTime.hour() + currentTime.minute() / 60.0));
    painter.drawLine(0, 0, 0, -40);
    painter.restore();

    painter.save();
    painter.rotate(6.0 * (currentTime.minute() + currentTime.second() / 60.0));
    painter.drawLine(0, 0, 0, -60);
    painter.restore();

    painter.setPen(Qt::red);
    painter.save();
    painter.rotate(6.0 * currentTime.second());
    painter.drawLine(0, 0, 0, -70);
    painter.restore();

    // Переместили отображение времени ниже, под часы
    painter.setPen(Qt::white);
    painter.drawText(-50, 120, 100, 30, Qt::AlignCenter, currentTime.toString("hh:mm:ss"));
}

void MainWindow::updateClock()
{
    currentTime = currentTime.addSecs(1);
    ui->timeEdit->setTime(currentTime);

    // Обычный тик
    tickSound.play();

    // В начале нового часа
    if (currentTime.minute() == 0 && currentTime.second() == 0) {
        int hour = currentTime.hour() % 24;

        if (hour == 12 || hour == 0) {
            // Кукушка для 12 и 24 часов
            strikesRemaining = (hour == 12) ? 12 : 24;
            cuckooSound.play();
            strikesRemaining--;
        } else {
            // Strike звук для остальных часов - количество ударов = номеру часа
            strikesRemaining = hour;
            strikeSound.play();
            strikesRemaining--;
        }

        // Запускаем таймер для следующих ударов
        if (strikesRemaining > 0) {
            chimeTimer->start();
        }
    }

    update();
}

void MainWindow::playNextChime()
{
    if (strikesRemaining <= 0) {
        chimeTimer->stop();
        return;
    }

    strikesRemaining--;
    int hour = currentTime.hour() % 24;

    if (hour == 12 || hour == 0) {
        cuckooSound.play();
    } else {
        strikeSound.play();
    }
}

void MainWindow::on_setTimeButton_clicked()
{
    QString text = ui->textEdit->toPlainText().trimmed();
    QTime parsedTime = QTime::fromString(text, "hh:mm:ss");
    if (parsedTime.isValid()) {
        currentTime = parsedTime;
        ui->timeEdit->setTime(currentTime);
    } else {
        ui->textEdit->setPlainText("Неверный формат (hh:mm:ss)");
    }
    update();
}
