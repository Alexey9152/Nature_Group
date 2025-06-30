#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <set>
#include <vector>
#include <typeinfo>

struct Comp {
    QString Name;
    QString Section;
    int Price;

    bool operator==(const Comp& other) const {
        return Name == other.Name;
    }
};

struct Bag {
    QString Name;
    int Num;
    QString Section;

    bool operator==(const Bag& other) const {
        return Name == other.Name;
    }
};

QTextStream& operator>>(QTextStream& in, Comp& item);
QTextStream& operator<<(QTextStream& out, const Comp& item);
QTextStream& operator>>(QTextStream& in, Bag& item);
QTextStream& operator<<(QTextStream& out, const Bag& item);

template <class T>
class COMP {
private:
    inline static QString TableTitle;
    int count;
    T* arr;

public:
    COMP() : count(0), arr(nullptr) {}
    COMP(const COMP& other);
    COMP& operator=(const COMP& other);
    ~COMP();

    friend QTextStream& operator>>(QTextStream& in, COMP& comp) {
        std::vector<T> temp;
        T item;
        while (!in.atEnd()) {
            in >> item;
            if (in.status() != QTextStream::Ok) {
                in.resetStatus();
                break;
            }
            temp.push_back(item);
        }

        comp.count = temp.size();
        delete[] comp.arr;
        comp.arr = new T[comp.count];
        std::copy(temp.begin(), temp.end(), comp.arr);

        return in;
    }

    friend QTextStream& operator<<(QTextStream& out, const COMP& comp) {
        for (int i = 0; i < comp.count; ++i) {
            out << comp.arr[i];
        }
        return out;
    }

    QString printTable() const;
    COMP operator+(const COMP& other) const;
    COMP operator-(const COMP& other) const;
    int GetCount() const { return count; }
    void sortByName();
    void sortSpecial();
    T* findByName(const QString& name);
};

// Объявляем явные специализации для printTable
template<> QString COMP<Comp>::printTable() const;
template<> QString COMP<Bag>::printTable() const;

// Инициализация статических членов
template<> QString COMP<Comp>::TableTitle = "ТОВАРЫ В МАГАЗИНЕ (COMP)";
template<> QString COMP<Bag>::TableTitle = "ТОВАРЫ В МАГАЗИНЕ (BAG)";

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void mergeArrays();
    void sortReverseAlphabetical();
    void sortBySection();
    void searchItem();

private:
    QTextEdit *textEdit;
    COMP<Comp> M1, M2, M3;
    COMP<Bag> K1, K2, K3;

    void setupUI();
    void loadData();
    void showData();
};

#endif // MAINWINDOW_H
