#ifndef ANALOGCLOCK_H
#define ANALOGCLOCK_H

#include <QWidget>
#include <QTimer>
#include <QSoundEffect>
#include <QImage>
#include <QTime>
#include <QDateTime>
class QCloseEvent;
class QHideEvent;

class AnalogClock : public QWidget
{
    Q_OBJECT

public:
    enum class HandStyle {
        Standard,
        Thin,
        Massive
    };
    Q_ENUM(HandStyle)

    explicit AnalogClock(QWidget *parent = nullptr);
    ~AnalogClock();

    void setBackgroundImage(const QString &imagePath);
    void setHandStyle(HandStyle style);

    void setCustomDateTime(const QDateTime &dt);
    void setUseSystemTime(bool use);

    void setDaliMode(bool enabled);

    QString getMinuteHandImagePath(int minute) const;

signals:
    void clockWindowClosed();
    void windowShown();

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateClockAndPlaySounds();

private:
    QVector<QPoint> getHourHandPoints(HandStyle style) const;
    QVector<QPoint> getMinuteHandPoints(HandStyle style) const;
    QVector<QPoint> getSecondHandPoints(HandStyle style) const;

    QString currentBackgroundImagePath;
    HandStyle currentHandStyle;
    bool isClosingOrHiding;

    QTimer *timer;
    QSoundEffect *tickTockSound;
    QSoundEffect *hourBellSound;
    QSoundEffect *cuckooSound;

    int lastMinute;
    int lastHour;
    bool tickState;

    QImage cuckooImage;
    bool isCuckooVisible;

    QDateTime currentDateTime;
    bool useSystemTime;

    int cuckooRemaining = 0;
    QTimer* cuckooTimer;
    void startCuckooSequence(int count);
    void handleCuckooAnimation();

    bool daliModeEnabled;
};
#endif
