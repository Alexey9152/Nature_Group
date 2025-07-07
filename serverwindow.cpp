#include "serverwindow.h"
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QHeaderView>
#include <QStatusBar>
#include <QLabel>
#include <QCloseEvent>
#include <QTime>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // --- ИСПРАВЛЕННЫЙ ПОРЯДОК ---

    // 1. Сначала создаем экземпляр логики. Теперь serverLogic - валидный указатель.
    serverLogic = new ExpressionServer(this);

    // 2. Теперь настраиваем UI, который может ссылаться на serverLogic.
    setupUi();
    setupMenu(); // Теперь этот вызов безопасен

    // 3. Соединяем сигналы от логики со слотами в GUI
    connect(serverLogic, &ExpressionServer::historyUpdated, this, &ServerWindow::updateHistoryView);
    connect(serverLogic, &ExpressionServer::logMessage, this, &ServerWindow::appendToLog);

    // 4. Запускаем сервер
    serverLogic->start(12345);
}

ServerWindow::~ServerWindow()
{
    // serverLogic будет удален автоматически, так как QMainWindow (this) является его родителем
}

void ServerWindow::setupUi()
{
    setWindowTitle("Expression Server Control Panel");
    resize(900, 700);

    logView = new QPlainTextEdit(this);
    logView->setReadOnly(true);
    logView->setMaximumHeight(200);
    logView->setFont(QFont("Courier New", 9));

    historyTable = new QTableWidget(this);
    historyTable->setColumnCount(6);
    historyTable->setHorizontalHeaderLabels({"DateTime", "Type", "Expression", "RPN", "Result", "Time (ms)"});
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setAlternatingRowColors(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("<b>Server Log:</b>", this));
    layout->addWidget(logView);
    layout->addWidget(new QLabel("<b>Calculation History:</b>", this));
    layout->addWidget(historyTable);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    statusLabel = new QLabel("Server is initializing...");
    statusBar()->addWidget(statusLabel);
}

void ServerWindow::setupMenu()
{
    QMenu *historyMenu = menuBar()->addMenu("&History");
    // Теперь эта строка абсолютно безопасна, т.к. serverLogic уже существует
    historyMenu->addAction("&Save History Now", serverLogic, &ExpressionServer::saveHistoryToFile);
    historyMenu->addSeparator();
    historyMenu->addAction("E&xit", this, &ServerWindow::close);

    QMenu *sortMenu = menuBar()->addMenu("&Sort");
    sortMenu->addAction("Sort by &Date (Newest First)", this, &ServerWindow::sortHistoryByDate);
    sortMenu->addAction("Sort by Request &Type", this, &ServerWindow::sortHistoryByType);
    sortMenu->addAction("Sort by Expression &Length", this, &ServerWindow::sortHistoryByLength);
    sortMenu->addAction("Sort by Processing &Time", this, &ServerWindow::sortHistoryByTime);
}

void ServerWindow::closeEvent(QCloseEvent *event)
{
    appendToLog("Shutting down. Saving history...", "purple");
    serverLogic->saveHistoryToFile();
    event->accept();
}

void ServerWindow::appendToLog(const QString& message, const QString& color)
{
    QString html = QString("<span style='color:%1;'>[%2] %3</span>")
    .arg(color)
        .arg(QTime::currentTime().toString("hh:mm:ss.zzz"))
        .arg(message);
    logView->appendHtml(html);
    statusLabel->setText(message);
}

void ServerWindow::updateHistoryView(const QVector<ExpressionServer::HistoryRecord>& records)
{
    currentHistory = records;
    populateTable();
}

void ServerWindow::populateTable()
{
    historyTable->setSortingEnabled(false);
    historyTable->setRowCount(0); // Очищаем таблицу
    historyTable->setRowCount(currentHistory.size());

    for (int i = 0; i < currentHistory.size(); ++i) {
        const auto& record = currentHistory[i];
        historyTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(record.dateTime.toString())));
        historyTable->setItem(i, 1, new QTableWidgetItem(record.requestType));
        historyTable->setItem(i, 2, new QTableWidgetItem(record.expression));
        historyTable->setItem(i, 3, new QTableWidgetItem(record.rpn));
        historyTable->setItem(i, 4, new QTableWidgetItem(record.result));
        historyTable->setItem(i, 5, new QTableWidgetItem(QString::number(record.processingTime)));
    }
    historyTable->setSortingEnabled(true);
}

void ServerWindow::sortHistoryByDate() {
    std::sort(currentHistory.begin(), currentHistory.end(),
              [](const auto& a, const auto& b) { return a.dateTime > b.dateTime; });
    populateTable();
}

void ServerWindow::sortHistoryByType() {
    std::sort(currentHistory.begin(), currentHistory.end(),
              [](const auto& a, const auto& b) { return a.requestType < b.requestType; });
    populateTable();
}

void ServerWindow::sortHistoryByLength() {
    std::sort(currentHistory.begin(), currentHistory.end(),
              [](const auto& a, const auto& b) { return a.expression.length() > b.expression.length(); });
    populateTable();
}

void ServerWindow::sortHistoryByTime() {
    std::sort(currentHistory.begin(), currentHistory.end(),
              [](const auto& a, const auto& b) { return a.processingTime > b.processingTime; });
    populateTable();
}
