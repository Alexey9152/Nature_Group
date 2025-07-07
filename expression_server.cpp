#include "expression_server.h"
#include <QDataStream>
#include <QFile>
#include <QTextStream>
#include <stack>
#include <cmath>

ExpressionServer::ExpressionServer(QObject *parent) : QObject(parent) {
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &ExpressionServer::onNewConnection);
}

void ExpressionServer::start(quint16 port) {
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        emit logMessage("Server could not start: " + tcpServer->errorString(), "red");
    } else {
        emit logMessage(QString("Server started and listening on port %1").arg(port), "green");
    }
}

void ExpressionServer::onNewConnection() {
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &ExpressionServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ExpressionServer::onClientDisconnected);
    clients.insert(socket, ClientInfo());
    emit logMessage("New client connected: " + socket->peerAddress().toString(), "blue");
}

void ExpressionServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients.contains(socket)) return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_0);

    while(socket->bytesAvailable() > 0) {
        quint16 blockSize;
        if (socket->bytesAvailable() < sizeof(quint16)) return;
        in >> blockSize;
        if (socket->bytesAvailable() < blockSize) return;

        quint16 msgTypeRaw;
        in >> msgTypeRaw;
        MessageType msgType = static_cast<MessageType>(msgTypeRaw);
        ClientInfo &info = clients[socket];

        if (msgType == MessageType::C2S_EXPRESSION_SUBMISSION && info.state == ClientInfo::WaitingForExpression) {
            handleExpressionSubmission(socket, in);
        } else if (msgType == MessageType::C2S_COEFFICIENTS_SUBMISSION && info.state == ClientInfo::WaitingForCoefficients) {
            handleCoefficientsSubmission(socket, in);
        } else {
            emit logMessage("Protocol error or wrong state from client " + socket->peerAddress().toString(), "red");
            sendResponse(socket, MessageType::S2C_PROTOCOL_ERROR, "Invalid message for current state.");
            socket->disconnectFromHost();
        }
    }
}

void ExpressionServer::handleExpressionSubmission(QTcpSocket* socket, QDataStream& in) {
    ClientInfo& info = clients[socket];
    info.timer.start();

    QString expression, clientRpn;
    in >> expression >> clientRpn;
    info.expression = expression;

    QString serverRpn, errorMsg;
    if (!convertToRPN(expression, serverRpn, errorMsg)) {
        sendResponse(socket, MessageType::S2C_EXPRESSION_ERROR, errorMsg);
        addHistoryRecord("Expression Error", expression, clientRpn, errorMsg, info.timer.elapsed());
        info.state = ClientInfo::WaitingForExpression;
        return;
    }

    info.rpn = serverRpn;
    if (serverRpn == clientRpn) {
        emit logMessage("RPN match for '" + expression + "'. Requesting coefficients.", "darkgreen");
        sendResponse(socket, MessageType::S2C_RPN_MATCH_REQUEST_COEFFS);
        info.state = ClientInfo::WaitingForCoefficients;
    } else {
        emit logMessage("RPN mismatch for '" + expression + "'. Sending correct RPN.", "orange");
        sendResponse(socket, MessageType::S2C_RPN_MISMATCH_SEND_CORRECT, serverRpn);
        QString logMsg = "RPN Mismatch. Client: " + clientRpn + ", Server: " + serverRpn;
        addHistoryRecord("RPN Mismatch", expression, clientRpn, logMsg, info.timer.elapsed());
        info.state = ClientInfo::WaitingForExpression;
    }
}

void ExpressionServer::handleCoefficientsSubmission(QTcpSocket* socket, QDataStream& in) {
    ClientInfo& info = clients[socket];
    QMap<QString, double> operands;
    in >> operands;

    double result;
    QString errorMsg;
    if (!calculateRPN(info.rpn, operands, result, errorMsg)) {
        emit logMessage("Calculation error for '" + info.expression + "': " + errorMsg, "red");
        sendResponse(socket, MessageType::S2C_CALCULATION_ERROR, errorMsg);
        addHistoryRecord("Calculation Error", info.expression, info.rpn, errorMsg, info.timer.elapsed());
    } else {
        emit logMessage("Calculation success for '" + info.expression + "'. Result: " + QString::number(result), "magenta");
        sendResponse(socket, MessageType::S2C_FINAL_RESULT, QString::number(result, 'g', 15));
        addHistoryRecord("Calculation Success", info.expression, info.rpn, QString::number(result, 'g', 15), info.timer.elapsed());
    }
    info.state = ClientInfo::WaitingForExpression;
}

void ExpressionServer::onClientDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        emit logMessage("Client disconnected: " + socket->peerAddress().toString(), "orange");
        clients.remove(socket);
        socket->deleteLater();
    }
}

void ExpressionServer::sendResponse(QTcpSocket* socket, MessageType type, const QString &payload) {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << static_cast<quint16>(type);
    if (!payload.isEmpty()) out << payload;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));
    socket->write(block);
}

void ExpressionServer::addHistoryRecord(const QString& type, const QString& expr, const QString& rpn, const QString& res, qint64 time) {
    HistoryRecord record;
    record.dateTime = Date::now();
    record.requestType = type;
    record.expression = expr;
    record.rpn = rpn;
    record.result = res;
    record.processingTime = time;
    history.append(record);
    emit logMessage("History record added: " + type, "gray");
    emit historyUpdated(history);
}

void ExpressionServer::saveHistoryToFile() {
    QFile file("server_history.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "DateTime\tType\tExpression\tRPN\tResult\tProcessingTime(ms)\n";
        for (const auto& record : history) {
            out << QString::fromStdString(record.dateTime.toString()) << "\t" << record.requestType << "\t"
                << record.expression << "\t" << record.rpn << "\t"
                << record.result << "\t" << record.processingTime << "\n";
        }
        file.close();
        emit logMessage("History saved to server_history.txt", "purple");
    } else {
        emit logMessage("Failed to save history: " + file.errorString(), "red");
    }
}

int ExpressionServer::getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

bool ExpressionServer::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

bool ExpressionServer::convertToRPN(const QString& expression, QString& rpn, QString& error) {
    std::stack<char> opStack;
    std::string output;
    std::string expr = expression.toStdString();
    for (size_t i = 0; i < expr.length(); ++i) {
        char c = expr[i];
        if (isspace(c)) continue;
        if (isdigit(c) || (c == '.')) {
            while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) output += expr[i++];
            output += ' '; i--;
        } else if (isalpha(c)) {
            while (i < expr.length() && isalnum(expr[i])) output += expr[i++];
            output += ' '; i--;
        } else if (c == '(') {
            opStack.push(c);
        } else if (c == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                output += opStack.top(); output += ' '; opStack.pop();
            }
            if (opStack.empty()) { error = "Mismatched parentheses."; return false; }
            opStack.pop();
        } else if (isOperator(c)) {
            while (!opStack.empty() && opStack.top() != '(' && getPrecedence(opStack.top()) >= getPrecedence(c)) {
                output += opStack.top(); output += ' '; opStack.pop();
            }
            opStack.push(c);
        } else {
            error = QString("Invalid character: %1").arg(c); return false;
        }
    }
    while (!opStack.empty()) {
        if (opStack.top() == '(') { error = "Mismatched parentheses."; return false; }
        output += opStack.top(); output += ' '; opStack.pop();
    }
    rpn = QString::fromStdString(output).trimmed();
    return true;
}

bool ExpressionServer::calculateRPN(const QString& rpn, const QMap<QString, double>& operands, double& finalResult, QString& error) {
    std::stack<double> calcStack;
    QStringList tokens = rpn.split(' ', Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        if (isOperator(token[0].toLatin1()) && token.length() == 1) {
            if (calcStack.size() < 2) { error = "Syntax error in RPN."; return false; }
            double b = calcStack.top(); calcStack.pop();
            double a = calcStack.top(); calcStack.pop();
            if (token == "+") calcStack.push(a + b);
            else if (token == "-") calcStack.push(a - b);
            else if (token == "*") calcStack.push(a * b);
            else if (token == "/") {
                if (b == 0.0) { error = "Division by zero."; return false; }
                calcStack.push(a / b);
            } else if (token == "^") {
                calcStack.push(pow(a, b));
            }
        } else {
            bool isNumber;
            double value = token.toDouble(&isNumber);
            if (isNumber) calcStack.push(value);
            else if (operands.contains(token)) calcStack.push(operands.value(token));
            else { error = "Unknown variable: " + token; return false; }
        }
    }
    if (calcStack.size() != 1) { error = "Invalid RPN expression."; return false; }
    finalResult = calcStack.top();
    return true;
}
