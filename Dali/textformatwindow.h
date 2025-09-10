#ifndef TEXTFORMATWINDOW_H
#define TEXTFORMATWINDOW_H

#include <QMainWindow>
#include <QDateTime>

namespace Ui {
class TextFormatWindow;
}

class TextFormatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextFormatWindow(QWidget *parent = nullptr);
    ~TextFormatWindow();

    void setCustomDateTime(const QDateTime& dt);

private slots:
    void onConvertClicked();

private:
    void updateDisplay();
    int parseNumberFromVerbal(const QStringList& words) const;
    QDateTime parseVerbalDateTime(const QString& verbalText);
    int wordToNumber(const QString& word) const;
    int monthWordToNumber(const QString& monthWord) const;

    Ui::TextFormatWindow *ui;
    QDateTime currentDateTime;
};
#endif
