#include "serverwindow.h"
#include <QAction>
#include <QCloseEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent),
    logView(nullptr),
    historyTable(nullptr),
    statusLabel(nullptr),
    serverLogic(new ExpressionServer(this))
{
    setupUi();
    setupMenu();

    connect(serverLogic, &ExpressionServer::historyUpdated,
            this, &ServerWindow::updateHistoryView);
    connect(serverLogic, &ExpressionServer::logMessage,
            this, &ServerWindow::appendToLog);

    serverLogic->start(12345);
}

ServerWindow::~ServerWindow()
{
    // serverLogic удаляется автоматически как дочерний объект
}

void ServerWindow::setupUi()
{
    setWindowTitle("Сервер вычисления выражений");
    resize(900, 700);

    logView = new QPlainTextEdit(this);
    logView->setReadOnly(true);
    logView->setMaximumHeight(200);
    logView->setFont(QFont("Courier New", 9));

    historyTable = new QTableWidget(this);
    historyTable->setColumnCount(6);
    historyTable->setHorizontalHeaderLabels(
        {"Дата", "Тип", "Выражение", "ОПЗ", "Результат", "Время (мс)"});
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("<b>Журнал сервера:</b>", this));
    layout->addWidget(logView);
    layout->addWidget(new QLabel("<b>История вычислений:</b>", this));
    layout->addWidget(historyTable);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    statusLabel = new QLabel("Сервер запущен");
    statusBar()->addWidget(statusLabel);
}

void ServerWindow::setupMenu()
{
    QMenu *historyMenu = menuBar()->addMenu("&История");
    historyMenu->addAction("&Сохранить историю", serverLogic, &ExpressionServer::saveHistoryToFile);
    historyMenu->addSeparator();
    historyMenu->addAction("&Выход", this, &ServerWindow::close);

    QMenu *sortMenu = menuBar()->addMenu("&Сортировка");
    sortMenu->addAction("По &дате (новые сначала)", this, &ServerWindow::sortHistoryByDate);
    sortMenu->addAction("По &типу запроса", this, &ServerWindow::sortHistoryByType);
    sortMenu->addAction("По &длине выражения", this, &ServerWindow::sortHistoryByLength);
    sortMenu->addAction("По времени &обработки", this, &ServerWindow::sortHistoryByTime);
}

void ServerWindow::closeEvent(QCloseEvent *event)
{
    appendToLog("Завершение работы. Сохранение истории...", "purple");
    serverLogic->saveHistoryToFile();
    event->accept();
}

void ServerWindow::appendToLog(const QString &message, const QString &color)
{
    QString html = QString("<span style='color:%1;'>[%2] %3</span>")
    .arg(color)
        .arg(QTime::currentTime().toString("hh:mm:ss.zzz"))
        .arg(message);
    logView->appendHtml(html);
    statusLabel->setText(message);
}

void ServerWindow::updateHistoryView(const QVector<ExpressionServer::HistoryRecord> &records)
{
    currentHistory = records;
    populateTable();
}

void ServerWindow::populateTable()
{
    historyTable->setSortingEnabled(false);
    historyTable->setRowCount(0);
    historyTable->setRowCount(currentHistory.size());

    for (int i = 0; i < currentHistory.size(); ++i) {
        const auto &record = currentHistory[i];
        historyTable->setItem(i, 0, new QTableWidgetItem(
                                        QString::fromStdString(record.dateTime.toString())));
        historyTable->setItem(i, 1, new QTableWidgetItem(record.requestType));
        historyTable->setItem(i, 2, new QTableWidgetItem(record.expression));
        historyTable->setItem(i, 3, new QTableWidgetItem(record.rpn));
        historyTable->setItem(i, 4, new QTableWidgetItem(record.result));
        historyTable->setItem(i, 5, new QTableWidgetItem(
                                        QString::number(record.processingTime)));
    }

    historyTable->setSortingEnabled(true);
}

void ServerWindow::sortHistoryByDate()
{
    std::sort(currentHistory.begin(), currentHistory.end(), [](const auto &a, const auto &b) {
        return a.dateTime > b.dateTime;
    });
    populateTable();
}

void ServerWindow::sortHistoryByType()
{
    std::sort(currentHistory.begin(), currentHistory.end(), [](const auto &a, const auto &b) {
        return a.requestType < b.requestType;
    });
    populateTable();
}

void ServerWindow::sortHistoryByLength()
{
    std::sort(currentHistory.begin(), currentHistory.end(), [](const auto &a, const auto &b) {
        return a.expression.length() > b.expression.length();
    });
    populateTable();
}

void ServerWindow::sortHistoryByTime()
{
    std::sort(currentHistory.begin(), currentHistory.end(), [](const auto &a, const auto &b) {
        return a.processingTime > b.processingTime;
    });
    populateTable();
}
