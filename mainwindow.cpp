#include "mainwindow.h"
#include <QFile>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>

QTextStream &operator>>(QTextStream &in, Comp &item)
{
    item.Name = in.readLine();
    item.Section = in.readLine();
    in >> item.Price;
    in.readLine();
    return in;
}

QTextStream &operator<<(QTextStream &out, const Comp &item)
{
    out << item.Name << '\n' << item.Section << '\n' << item.Price << '\n';
    return out;
}

QTextStream &operator>>(QTextStream &in, Bag &item)
{
    item.Name = in.readLine();
    in >> item.Num;
    in.readLine();
    item.Section = in.readLine();
    return in;
}

QTextStream &operator<<(QTextStream &out, const Bag &item)
{
    out << item.Name << '\n' << item.Num << '\n' << item.Section << '\n';
    return out;
}

template<class T>
COMP<T>::COMP(const COMP &other)
    : count(other.count)
    , arr(new T[other.count])
{
    std::copy(other.arr, other.arr + count, arr);
}

template<class T>
COMP<T> &COMP<T>::operator=(const COMP &other)
{
    if (this != &other) {
        delete[] arr;
        count = other.count;
        arr = new T[count];
        std::copy(other.arr, other.arr + count, arr);
    }
    return *this;
}

template<class T>
COMP<T>::~COMP()
{
    delete[] arr;
}

template<class T>
QString COMP<T>::printTable() const
{
    return QString();
}

template<>
QString COMP<Comp>::printTable() const
{
    QString result;
    QTextStream out(&result);

    const int wName = 30;
    const int wSection = 20;
    const int wPrice = 10;

    out << TableTitle << "\n\n";

    out << QString("Наименование").leftJustified(wName) << QString("Секция").leftJustified(wSection)
        << QString("Цена").rightJustified(wPrice) << "\n";

    out << QString(wName + wSection + wPrice, '-') << "\n";

    for (int i = 0; i < count; ++i) {
        out << arr[i].Name.leftJustified(wName) << arr[i].Section.leftJustified(wSection)
            << QString::number(arr[i].Price).rightJustified(wPrice) << "\n";
    }

    out << "\n";
    return result;
}

template<>
QString COMP<Bag>::printTable() const
{
    QString result;
    QTextStream out(&result);

    const int wName = 30;
    const int wNum = 15;
    const int wSection = 20;

    out << TableTitle << "\n\n";

    out << QString("Наименование").leftJustified(wName)
        << QString("Количество").rightJustified(wNum) << QString("Секция").rightJustified(wSection)
        << "\n";

    out << QString(wName + wSection + wNum, '-') << "\n";

    for (int i = 0; i < count; ++i) {
        out << arr[i].Name.leftJustified(wName) << QString::number(arr[i].Num).rightJustified(wNum)
            << arr[i].Section.rightJustified(wSection) << "\n";
    }

    out << "\n";
    return result;
}

template<class T>
COMP<T> COMP<T>::operator+(const COMP &other) const
{
    std::set<QString> uniqueNames;
    std::vector<T> result;

    for (int i = 0; i < count; ++i) {
        if (uniqueNames.find(arr[i].Name) == uniqueNames.end()) {
            uniqueNames.insert(arr[i].Name);
            result.push_back(arr[i]);
        }
    }

    for (int i = 0; i < other.count; ++i) {
        if (uniqueNames.find(other.arr[i].Name) == uniqueNames.end()) {
            uniqueNames.insert(other.arr[i].Name);
            result.push_back(other.arr[i]);
        }
    }

    COMP newComp;
    newComp.count = result.size();
    newComp.arr = new T[newComp.count];
    std::copy(result.begin(), result.end(), newComp.arr);

    return newComp;
}

template<class T>
COMP<T> COMP<T>::operator-(const COMP &other) const
{
    std::set<QString> otherNames;
    std::vector<T> result;

    for (int i = 0; i < other.count; ++i) {
        otherNames.insert(other.arr[i].Name);
    }

    for (int i = 0; i < count; ++i) {
        if (otherNames.find(arr[i].Name) == otherNames.end()) {
            result.push_back(arr[i]);
        }
    }

    COMP newComp;
    newComp.count = result.size();
    newComp.arr = new T[newComp.count];
    std::copy(result.begin(), result.end(), newComp.arr);

    return newComp;
}

template<class T>
void COMP<T>::sortByName()
{
    std::sort(arr, arr + count, [](const T &a, const T &b) { return a.Name < b.Name; });
}

template<class T>
void COMP<T>::sortSpecial()
{
    if (typeid(T) == typeid(Comp)) {
        std::sort(arr, arr + count, [](const T &a, const T &b) { return a.Name > b.Name; });
    } else {
        std::sort(arr, arr + count, [](const T &a, const T &b) {
            if (a.Section != b.Section)
                return a.Section < b.Section;
            return a.Name > b.Name;
        });
    }
}

template<class T>
T *COMP<T>::findByName(const QString &name)
{
    for (int i = 0; i < count; ++i) {
        if (arr[i].Name == name) {
            return &arr[i];
        }
    }
    return nullptr;
}

// Явная инстанциация шаблонов
template class COMP<Comp>;
template class COMP<Bag>;

// Реализация методов MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    loadData();
    showData();
}

void MainWindow::setupUI()
{
    setWindowTitle("Управление товарами");
    setMinimumSize(800, 600);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Courier New"));
    mainLayout->addWidget(textEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    QPushButton *btnMerge = new QPushButton("1. Объединить массивы");
    QPushButton *btnSortReverse = new QPushButton("2. Сортировать COMP (обратный алфавит)");
    QPushButton *btnSortSection = new QPushButton("3. Сортировать BAG (по секциям)");
    QPushButton *btnSearch = new QPushButton("4. Найти товар");

    buttonLayout->addWidget(btnMerge);
    buttonLayout->addWidget(btnSortReverse);
    buttonLayout->addWidget(btnSortSection);
    buttonLayout->addWidget(btnSearch);

    mainLayout->addLayout(buttonLayout);

    setCentralWidget(centralWidget);
    statusBar()->showMessage("Готово");

    connect(btnMerge, &QPushButton::clicked, this, &MainWindow::mergeArrays);
    connect(btnSortReverse, &QPushButton::clicked, this, &MainWindow::sortReverseAlphabetical);
    connect(btnSortSection, &QPushButton::clicked, this, &MainWindow::sortBySection);
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::searchItem);
}

void MainWindow::loadData()
{
    QFile file1("Ob1.txt");
    if (file1.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file1);
        stream >> M1;
        file1.close();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл Ob1.txt");
    }

    QFile file2("Ob2.txt");
    if (file2.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file2);
        stream >> M2;
        file2.close();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл Ob2.txt");
    }
    QFile file3("TOb1.txt");
    if (file3.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file3);
        stream >> K1;
        file3.close();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл TOb1.txt");
    }

    QFile file4("TOb2.txt");
    if (file4.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file4);
        stream >> K2;
        file4.close();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл TOb2.txt");
    }

    M3 = M1 + M2;
    K3 = K1 + K2;
}

void MainWindow::showData()
{
    QString output;
    QTextStream stream(&output);

    stream << "===== ДАННЫЕ COMP ДО ОБЪЕДИНЕНИЯ =====\n";
    stream << "--- Массив M1 ---\n" << M1.printTable();
    stream << "--- Массив M2 ---\n" << M2.printTable();

    stream << "\n===== ДАННЫЕ BAG ДО ОБЪЕДИНЕНИЯ =====\n";
    stream << "--- Массив K1 ---\n" << K1.printTable();
    stream << "--- Массив K2 ---\n" << K2.printTable();

    stream << "\n===== ДАННЫЕ COMP ПОСЛЕ ОБЪЕДИНЕНИЯ =====\n";
    stream << M3.printTable();

    stream << "\n===== ДАННЫЕ BAG ПОСЛЕ ОБЪЕДИНЕНИЯ =====\n";
    stream << K3.printTable();

    textEdit->setText(output);
}

void MainWindow::mergeArrays()
{
    M3 = M1 + M2;
    K3 = K1 + K2;
    showData();
    statusBar()->showMessage("Массивы объединены");
}

void MainWindow::sortReverseAlphabetical()
{
    M3.sortSpecial();
    showData();
    statusBar()->showMessage("Сортировка COMP в обратном алфавитном порядке");
}

void MainWindow::sortBySection()
{
    K3.sortSpecial();
    showData();
    statusBar()->showMessage("Сортировка BAG по секциям и обратному алфавиту");
}

void MainWindow::searchItem()
{
    bool ok;
    QString name = QInputDialog::getText(this,
                                         "Поиск товара",
                                         "Введите наименование товара:",
                                         QLineEdit::Normal,
                                         "",
                                         &ok);
    if (!ok || name.isEmpty())
        return;

    Comp *compItem = M3.findByName(name);
    Bag *bagItem = K3.findByName(name);

    QString result;
    if (compItem) {
        result = QString("Найден в COMP:\nНаименование: %1\nСекция: %2\nЦена: %3")
                     .arg(compItem->Name)
                     .arg(compItem->Section)
                     .arg(compItem->Price);
    } else if (bagItem) {
        result = QString("Найден в BAG:\nНаименование: %1\nКоличество: %2\nСекция: %3")
                     .arg(bagItem->Name)
                     .arg(bagItem->Num)
                     .arg(bagItem->Section);
    } else {
        result = "Товар не найден ни в одном массиве";
    }

    QMessageBox::information(this, "Результат поиска", result);
}
