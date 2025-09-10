#include "analogclock.h"
#include <QPainter>
#include <QColor>
#include <QRadialGradient>
#include <QImage>
#include <QtMath>
#include <QCloseEvent>
#include <QDebug>
#include <QSoundEffect>
#include <QUrl>
#include <QFile>

AnalogClock::AnalogClock(QWidget *parent)
    : QWidget(parent)
    , currentBackgroundImagePath(":/images/background.jpg")
    , currentHandStyle(HandStyle::Standard)
    , isClosingOrHiding(false)
    , timer(new QTimer(this))
    , tickTockSound(new QSoundEffect(this))
    , hourBellSound(new QSoundEffect(this))
    , cuckooSound(new QSoundEffect(this))
    , lastMinute(-1)
    , lastHour(-1)
    , tickState(true)
    , isCuckooVisible(false)
    , cuckooImage(":/images/cuckoo.png")
    , useSystemTime(true)
    , currentDateTime(QDateTime::currentDateTime())
    , daliModeEnabled(false)
{
    connect(timer, &QTimer::timeout, this, &AnalogClock::updateClockAndPlaySounds);
    timer->start(1000);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(400, 400);

    QString tickPath = "qrc:/sounds/tick.wav";
    tickTockSound->setSource(QUrl(tickPath));
    if (!tickTockSound->isLoaded()) {
        qDebug() << "Failed to load tick sound from " << tickPath << ", status:" << tickTockSound->status();
    } else {
        qDebug() << "Tick sound loaded successfully from " << tickPath;
    }

    QString bellPath = "qrc:/sounds/bell.wav";
    hourBellSound->setSource(QUrl(bellPath));
    if (!hourBellSound->isLoaded()) {
        qDebug() << "Failed to load bell sound from " << bellPath << ", status:" << hourBellSound->status();
    } else {
        qDebug() << "Bell sound loaded successfully from " << bellPath;
    }

    QString cuckooPath = "qrc:/sounds/cuckoo.wav";
    cuckooSound->setSource(QUrl(cuckooPath));
    if (!cuckooSound->isLoaded()) {
        qDebug() << "Failed to load cuckoo sound from " << cuckooPath << ", status:" << cuckooSound->status();
    } else {
        qDebug() << "Cuckoo sound loaded successfully from " << cuckooPath;
    }

    tickTockSound->setVolume(0.5f);
    hourBellSound->setVolume(0.7f);
    cuckooSound->setVolume(0.8f);

    if (cuckooImage.isNull()) {
        qDebug() << "ERROR: Cuckoo image not found at :'/images/cuckoo.png'";
    } else {
        qDebug() << "Cuckoo image loaded successfully.";
    }

    cuckooTimer = new QTimer(this);
    cuckooTimer->setInterval(1000);
    connect(cuckooTimer, &QTimer::timeout, this, &AnalogClock::handleCuckooAnimation);
}

AnalogClock::~AnalogClock()
{
    if (tickTockSound->isPlaying()) tickTockSound->stop();
    if (hourBellSound->isPlaying()) hourBellSound->stop();
    if (cuckooSound->isPlaying()) cuckooSound->stop();

    delete timer;
    delete tickTockSound;
    delete hourBellSound;
    delete cuckooSound;
}

void AnalogClock::setBackgroundImage(const QString &imagePath)
{
    currentBackgroundImagePath = imagePath;
    if (imagePath != ":/images/dali_clock.jpeg") {
        setDaliMode(false);
    }
    update();
}

void AnalogClock::setHandStyle(HandStyle style)
{
    currentHandStyle = style;
    update();
}

void AnalogClock::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QImage backgroundImage(currentBackgroundImagePath);
    if (!backgroundImage.isNull()) {
        painter.drawImage(rect(), backgroundImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        painter.fillRect(rect(), QColor(20, 20, 20));
        qDebug() << "Ошибка: не удалось загрузить фон:" << currentBackgroundImagePath;
    }

    QTime time = useSystemTime ? QTime::currentTime() : currentDateTime.time();

    if (daliModeEnabled) {
        int minute = time.minute(); //
        QString minuteHandPath = getMinuteHandImagePath(minute);
        QImage minuteHandImage(minuteHandPath);

        if (!minuteHandImage.isNull()) {
            painter.drawImage(rect(), minuteHandImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            qDebug() << "Ошибка: не удалось загрузить минутную стрелку Дали:" << minuteHandPath; //
        }

        int hour = time.hour();
        if (hour >= 12) hour -= 12;
        QString hourHandPath = QString(":/images/hour_%1.png").arg(hour, 2, 10, QChar('0'));
        QImage hourHandImage(hourHandPath);
        if (!hourHandImage.isNull()) {
            painter.drawImage(rect(), hourHandImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            qDebug() << "Ошибка: не удалось загрузить часовую стрелку Дали:" << hourHandPath;
        }


    } else {
        painter.translate(width() / 2, height() / 2);

        int clockRadius = qMin(width(), height()) / 2 * 0.85;
        double scale_factor = clockRadius / 100.0;
        painter.scale(scale_factor, scale_factor);
        painter.setPen(QPen(Qt::white, 2));
        QFont font = painter.font();
        font.setPointSizeF(20.0 / scale_factor);
        painter.setFont(font);

        const int numDrawRadius = 80;

        for (int i = 0; i < 12; ++i) {
            painter.save();
            double angle = (i * 30.0) - 90.0;
            painter.rotate(angle);
            painter.translate(numDrawRadius, 0);
            painter.rotate(-angle);
            QString numStr = QString::number(i == 0 ? 12 : i);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(numStr);
            int textHeight = fm.height();
            painter.drawText(-textWidth / 2, textHeight / 3, numStr);
            painter.restore();
        }
        QVector<QPoint> hourHandPoints = getHourHandPoints(currentHandStyle);
        QVector<QPoint> minuteHandPoints = getMinuteHandPoints(currentHandStyle);
        QVector<QPoint> secondHandPoints = getSecondHandPoints(currentHandStyle);

        painter.save();
        painter.rotate((time.second() + time.msec() / 1000.0) * 6.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 0));
        painter.drawConvexPolygon(secondHandPoints.constData(), secondHandPoints.size());
        painter.restore();

        painter.save();
        painter.rotate((time.minute() + time.second() / 60.0) * 6.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 165, 0));
        painter.drawConvexPolygon(minuteHandPoints.constData(), minuteHandPoints.size());
        painter.restore();

        painter.save();
        double currentHour = time.hour();
        if (currentHour >= 12) currentHour -= 12;
        painter.rotate((currentHour + time.minute() / 60.0) * 30.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 0, 0));
        painter.drawConvexPolygon(hourHandPoints.constData(), hourHandPoints.size());
        painter.restore();

        if (isCuckooVisible && !cuckooImage.isNull()) {
            double scale = 0.03;
            int imgWidth = cuckooImage.width() * scale;
            int imgHeight = cuckooImage.height() * scale;
            QPoint center(-imgWidth / 2, -imgHeight / 2 - 80);
            painter.drawImage(QRect(center, QSize(imgWidth, imgHeight)), cuckooImage);
        }
    }
}

QVector<QPoint> AnalogClock::getHourHandPoints(HandStyle style) const {
    switch (style) {
    case HandStyle::Standard: return {QPoint(0, 5), QPoint(5, 0), QPoint(0, -45)};
    case HandStyle::Thin: return {QPoint(0, 3), QPoint(3, 0), QPoint(0, -50)};
    case HandStyle::Massive: return {QPoint(0, 8), QPoint(8, 0), QPoint(0, -40)};
    default: return {QPoint(0, 5), QPoint(5, 0), QPoint(0, -45)};
    }
}

QVector<QPoint> AnalogClock::getMinuteHandPoints(HandStyle style) const {
    switch (style) {
    case HandStyle::Standard: return {QPoint(0, 5), QPoint(5, 0), QPoint(0, -75)};
    case HandStyle::Thin: return {QPoint(0, 3), QPoint(3, 0), QPoint(0, -80)};
    case HandStyle::Massive: return {QPoint(0, 8), QPoint(8, 0), QPoint(0, -70)};
    default: return {QPoint(0, 5), QPoint(5, 0), QPoint(0, -75)};
    }
}

QVector<QPoint> AnalogClock::getSecondHandPoints(HandStyle style) const {
    switch (style) {
    case HandStyle::Standard: return {QPoint(0, 2), QPoint(2, 0), QPoint(0, -85)};
    case HandStyle::Thin: return {QPoint(0, 1), QPoint(1, 0), QPoint(0, -90)};
    case HandStyle::Massive: return {QPoint(0, 3), QPoint(3, 0), QPoint(0, -80)};
    default: return {QPoint(0, 2), QPoint(2, 0), QPoint(0, -85)};
    }
}

void AnalogClock::closeEvent(QCloseEvent *event)
{
    qDebug() << "AnalogClock::closeEvent() вызван.";
    if (isClosingOrHiding) {
        event->ignore();
        qDebug() << "AnalogClock::closeEvent() вызван повторно во время обработки. Игнорируем.";
        return;
    }

    if (this->isVisible()) {
        isClosingOrHiding = true;
        event->ignore();
        hide();
        emit clockWindowClosed();
        qDebug() << "Сигнал clockWindowClosed() излучен.";
    } else {
        event->ignore();
        qDebug() << "AnalogClock::closeEvent() вызван, но окно уже скрыто. Игнорируем повторный вызов.";
    }
}

void AnalogClock::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    isClosingOrHiding = false;
    qDebug() << "AnalogClock::hideEvent() вызван. isClosingOrHiding сброшен.";
}

void AnalogClock::setCustomDateTime(const QDateTime &dt)
{
    useSystemTime = false;
    currentDateTime = dt;
    update();
}

void AnalogClock::setUseSystemTime(bool use)
{
    useSystemTime = use;
    if (use) {
        currentDateTime = QDateTime::currentDateTime();
    }
    update();
}


void AnalogClock::updateClockAndPlaySounds()
{
    if (useSystemTime) {
        currentDateTime = QDateTime::currentDateTime();
    } else {
        currentDateTime = currentDateTime.addSecs(1);
    }

    update();

    QTime currentTime = currentDateTime.time();

    if (isClosingOrHiding || !this->isVisible()) {
        if (isCuckooVisible) {
            isCuckooVisible = false;
            update();
        }
        return;
    }

    if (tickTockSound->isLoaded() && !tickTockSound->isMuted()) {
        tickTockSound->play();
    }

    if (currentTime.minute() == 0 && currentTime.second() == 0) {
        if (currentTime.hour() == 12 && lastHour != 12) {
            startCuckooSequence(12);
            lastHour = 12;
        }
        else if (currentTime.hour() == 0 && lastHour != 0) {
            startCuckooSequence(24);
        }
        else if (currentTime.minute() == 0 && currentTime.second() == 0 &&
            lastHour != currentTime.hour() &&
            currentTime.hour() != 0 && currentTime.hour() != 12)
        {
            if (hourBellSound->isLoaded() && !hourBellSound->isMuted()) {
                int bellCount = currentTime.hour();
                qDebug() << "Playing hour bell " << bellCount << " times.";
                for (int i = 0; i < bellCount; ++i) {
                    QTimer::singleShot(1200 * i, this, [this]() {
                        if (this->hourBellSound->isLoaded()) {
                            this->hourBellSound->play();
                        }
                    });
                }
            }
            lastHour = currentTime.hour();
        }
        else if (lastHour != currentTime.hour() && currentTime.minute() != 0) {
            lastHour = currentTime.hour();
        }
    }
}

void AnalogClock::startCuckooSequence(int count)
{
    if (cuckooTimer->isActive()) return;

    cuckooRemaining = count * 2;
    isCuckooVisible = true;
    update();

    if (cuckooSound->isLoaded()) {
        cuckooSound->play();
    }

    cuckooTimer->start();
}

void AnalogClock::handleCuckooAnimation()
{
    cuckooRemaining--;

    if (cuckooRemaining <= 0) {
        cuckooTimer->stop();
        isCuckooVisible = false;
        update();
        return;
    }

    isCuckooVisible = !isCuckooVisible;
    update();

    if (isCuckooVisible && cuckooSound->isLoaded()) {
        cuckooSound->play();
    }
}

void AnalogClock::setDaliMode(bool enabled)
{
    daliModeEnabled = enabled;
    update();
}

QString AnalogClock::getMinuteHandImagePath(int minute) const
{
    return QString(":/images/min_%1.png").arg(minute, 2, 10, QChar('0'));
}

void AnalogClock::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    emit windowShown();
    qDebug() << "AnalogClock::showEvent() вызван. Сигнал windowShown() излучен.";
    isClosingOrHiding = false;
}
