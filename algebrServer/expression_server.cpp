#include "expression_server.h"
#include <QDataStream>
#include <QFile>
#include <QDateTime>
#include <cmath>
#include <QStack>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

ExpressionServer::ExpressionServer(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &ExpressionServer::onNewConnection);
}

void ExpressionServer::start(quint16 port)
{
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        emit logMessage("Ошибка запуска сервера: " + tcpServer->errorString(), "red");
    } else {
        emit logMessage(QString("Сервер запущен на порту %1").arg(port), "green");
    }
}

void ExpressionServer::onNewConnection()
{
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &ExpressionServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ExpressionServer::onClientDisconnected);
    clients.insert(socket, ClientInfo());
    emit logMessage("Новый клиент: " + socket->peerAddress().toString(), "blue");
}

void ExpressionServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients.contains(socket))
        return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_0);

    quint16 blockSize;
    if (socket->bytesAvailable() < sizeof(quint16))
        return;
    in >> blockSize;

    if (socket->bytesAvailable() < blockSize)
        return;

    quint16 messageTypeInt;
    in >> messageTypeInt;
    MessageType type = static_cast<MessageType>(messageTypeInt);

    switch (type) {
    case MessageType::C2S_EXPRESSION_SUBMISSION:
        handleExpressionSubmission(socket, in);
        break;
    case MessageType::C2S_COEFFICIENTS_SUBMISSION:
        handleCoefficientsSubmission(socket, in);
        break;
    default:
        emit logMessage("Неизвестный тип сообщения: " + QString::number(messageTypeInt), "red");
        sendResponse(socket, MessageType::S2C_PROTOCOL_ERROR, "Неизвестный тип сообщения");
        break;
    }
}

void ExpressionServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        clients.remove(socket);
        emit logMessage("Клиент отключен: " + socket->peerAddress().toString(), "blue");
        socket->deleteLater();
    }
}

void ExpressionServer::saveToBinaryLog(const QString &expr, const QString &rpn)
{
    QFile file("log.bin");
    if (!file.open(QIODevice::Append)) {
        emit logMessage("Не удалось открыть log.bin для записи", "red");
        return;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    BinaryLogEntry entry;
    qstrncpy(entry.expression, expr.toUtf8().constData(), sizeof(entry.expression) - 1);
    qstrncpy(entry.rpn, rpn.toUtf8().constData(), sizeof(entry.rpn) - 1);

    QDateTime now = QDateTime::currentDateTime();
    entry.hour = static_cast<unsigned char>(now.time().hour());
    entry.minute = static_cast<unsigned char>(now.time().minute());
    entry.second = static_cast<unsigned char>(now.time().second());

    out.writeRawData(reinterpret_cast<const char*>(&entry), sizeof(BinaryLogEntry));
    file.close();
}

void ExpressionServer::saveCoefficientsToBinary(const QMap<QString, double> &coeffs)
{
    QFile file("coeffs.bin");
    if (!file.open(QIODevice::Append)) {
        emit logMessage("Не удалось открыть coeffs.bin для записи", "red");
        return;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
        CoefficientEntry entry;
        qstrncpy(entry.name, it.key().toUtf8().constData(), sizeof(entry.name) - 1);
        entry.value = it.value();
        out.writeRawData(reinterpret_cast<const char*>(&entry), sizeof(CoefficientEntry));
    }
    file.close();
}

void ExpressionServer::handleExpressionSubmission(QTcpSocket *socket, QDataStream &in)
{
    QString expression;
    in >> expression;

    clients[socket].expression = expression;
    clients[socket].timer.restart();

    emit logMessage("Получено выражение от " + socket->peerAddress().toString() + ": " + expression, "black");

    QString clientRpn;
    in >> clientRpn;
    emit logMessage("ОПЗ от клиента: " + clientRpn, "blue");

    // Далее идет ваша существующая логика
    QString serverRpn;

    QString rpnCalculated;
    QString rpnError;
    if (!convertToRPN(expression, rpnCalculated, rpnError)) {
        emit logMessage("Ошибка преобразования в ОПЗ: " + rpnError, "red");
        sendResponse(socket, MessageType::S2C_EXPRESSION_ERROR, rpnError);
        return;
    }

    clients[socket].rpn = rpnCalculated;
    clients[socket].state = ClientInfo::WaitingForCoefficients;
    sendResponse(socket, MessageType::S2C_RPN_MATCH_REQUEST_COEFFS, rpnCalculated);
}

void ExpressionServer::handleCoefficientsSubmission(QTcpSocket *socket, QDataStream &in)
{

    QMap<QString, double> coefficients;
    in >> coefficients;

    QString logMsg = "Получены коэффициенты: ";
    for (auto it = coefficients.begin(); it != coefficients.end(); ++it) {
        logMsg += QString("%1=%2; ").arg(it.key()).arg(it.value());
    }
    emit logMessage(logMsg, "blue");
    quint32 coeffCount;
    in >> coeffCount;

    emit logMessage("Получены коэффициенты от " + socket->peerAddress().toString() + ": " +
                        QString::number(coefficients.size()) + " шт.", "black");

    QString serverRpn = clients[socket].rpn;
    double result;
    QString calcError;
    if (!calculateRPN(serverRpn, coefficients, result, calcError)) {
        emit logMessage("Ошибка вычисления ОПЗ: " + calcError, "red");
        sendResponse(socket, MessageType::S2C_CALCULATION_ERROR, calcError);
    } else {
        qint64 processingTime = clients[socket].timer.elapsed();
        emit logMessage(QString("Результат вычисления: %1").arg(result), "green");
        sendResponse(socket, MessageType::S2C_FINAL_RESULT, QString::number(result));
        addHistoryRecord("Calculation", clients[socket].expression, serverRpn, QString::number(result), processingTime);
    }
}

void ExpressionServer::sendResponse(QTcpSocket *socket, MessageType type, const QString &payload)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint16(0);
    out << static_cast<quint16>(type);
    out << payload;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    socket->write(block);
}

void ExpressionServer::addHistoryRecord(const QString &type, const QString &expr,
                                        const QString &rpn, const QString &res, qint64 time)
{
    HistoryRecord record;
    QDateTime now = QDateTime::currentDateTime();
    record.dateTime = Date(now.date().year(), now.date().month(), now.date().day(),
                           now.time().hour(), now.time().minute(), now.time().second());
    record.requestType = type;
    record.expression = expr;
    record.rpn = rpn;
    record.result = res;
    record.processingTime = time;
    historyRecords.append(record);
    emit historyUpdated(historyRecords);
}

bool ExpressionServer::isOpeningBracket(QChar c) const {
    return c == '(' || c == '[' || c == '{';
}

bool ExpressionServer::isClosingBracket(QChar c) const {
    return c == ')' || c == ']' || c == '}';
}

QChar ExpressionServer::getMatchingBracket(QChar c) const {
    switch(c.unicode()) {
    case '(': return ')';
    case ')': return '(';
    case '[': return ']';
    case ']': return '[';
    case '{': return '}';
    case '}': return '{';
    default: return QChar();
    }
}

bool ExpressionServer::isOperator(QChar c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '~' || c == '#';
}

bool ExpressionServer::isAllowedSymbol(QChar c) const {
    return c.isLetterOrNumber() || c == '.' || isOperator(c) ||
           isOpeningBracket(c) || isClosingBracket(c);
}

int ExpressionServer::getPrecedence(QChar op) const {
    if (op == '~' || op == '#') return 4;
    if (op == '*' || op == '/') return 3;
    if (op == '+' || op == '-') return 2;
    return 0;
}


bool ExpressionServer::convertToRPN(const QString &expression, QString &rpn, QString &error) {
    QStack<QChar> opStack;
    QStringList rpnTokens;
    bool expectOperand = true;

    for (int i = 0; i < expression.length(); ++i) {
        QChar c = expression[i];
        if (c.isSpace()) continue;

        if (!isAllowedSymbol(c)) {
            error = QString("Недопустимый символ '%1'").arg(c);
            return false;
        }

        if (c.isLetterOrNumber() || c == '.') {
            QString token;
            while (i < expression.length() && (expression[i].isLetterOrNumber() || expression[i] == '.')) {
                token += expression[i++];
            }
            i--;
            rpnTokens.append(token);
            expectOperand = false;
        }
        else if (isOpeningBracket(c)) {
            opStack.push(c);
            expectOperand = true;
        }
        else if (isClosingBracket(c)) {
            QChar matchingOpen = getMatchingBracket(c);
            while (!opStack.empty() && opStack.top() != matchingOpen) {
                rpnTokens.append(opStack.pop());
            }
            if (opStack.empty()) {
                error = QString("Несоответствующая скобка '%1'").arg(c);
                return false;
            }
            opStack.pop();
            expectOperand = false;
        }
        else if (isOperator(c)) {
            if (expectOperand && (c == '+' || c == '-')) {
                opStack.push(c == '-' ? '~' : '#');
            }
            else {
                while (!opStack.empty() && !isOpeningBracket(opStack.top()) &&
                       getPrecedence(opStack.top()) >= getPrecedence(c)) {
                    rpnTokens.append(opStack.pop());
                }
                opStack.push(c);
                expectOperand = true;
            }
        }
    }

    while (!opStack.empty()) {
        if (isOpeningBracket(opStack.top())) {
            error = QString("Незакрытая скобка '%1'").arg(opStack.top());
            return false;
        }
        rpnTokens.append(opStack.pop());
    }

    rpn = rpnTokens.join(" ");
    return true;
}

bool ExpressionServer::calculateRPN(const QString &rpn, const QMap<QString, double> &operands,
                                    double &result, QString &error) {
    QStack<double> stack;
    QStringList tokens = rpn.split(' ', Qt::SkipEmptyParts);

    for (const QString &token : tokens) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stack.size() < 2) {
                error = QString("Недостаточно операндов для оператора '%1'").arg(token);
                return false;
            }
            double b = stack.pop();
            double a = stack.pop();
            double res = 0;

            if (token == "+") res = a + b;
            else if (token == "-") res = a - b;
            else if (token == "*") res = a * b;
            else if (token == "/") {
                if (qFuzzyIsNull(b)) {
                    error = "Деление на ноль";
                    return false;
                }
                res = a / b;
            }

            stack.push(res);
        }
        else if (token == "~" || token == "#") {
            if (stack.empty()) {
                error = QString("Недостаточно операндов для унарного оператора '%1'").arg(token);
                return false;
            }
            double val = stack.pop();
            stack.push(token == "~" ? -val : val);
        }
        else {
            bool ok;
            double val = token.toDouble(&ok);
            if (!ok) {
                if (!operands.contains(token)) {
                    error = QString("Неизвестная переменная '%1'").arg(token);
                    return false;
                }
                val = operands.value(token);
            }
            stack.push(val);
        }
    }

    if (stack.size() != 1) {
        error = QString("Некорректное выражение (осталось %1 значений в стеке)").arg(stack.size());
        return false;
    }

    result = stack.top();
    return true;
}

void ExpressionServer::saveHistoryToFile()
{
    QFile file("server_history.bin");
    if (!file.open(QIODevice::WriteOnly)) {
        emit logMessage("Не удалось открыть server_history.bin для записи", "red");
        return;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    for (const auto& record : historyRecords) {
        out << static_cast<quint16>(record.dateTime.getYear());
        out << static_cast<quint8>(record.dateTime.getMonth());
        out << static_cast<quint8>(record.dateTime.getDay());
        out << static_cast<quint8>(record.dateTime.getHour());
        out << static_cast<quint8>(record.dateTime.getMinute());
        out << static_cast<quint8>(record.dateTime.getSecond());

        out << record.requestType;
        out << record.expression;
        out << record.rpn;
        out << record.result;
        out << record.processingTime;
    }
    file.close();
    emit logMessage("История сохранена в server_history.bin", "green");
}
