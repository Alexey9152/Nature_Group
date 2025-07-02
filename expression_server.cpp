#include "expression_server.h"

ExpressionServer::ExpressionServer(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &ExpressionServer::newConnection);
}

void ExpressionServer::start(quint16 port)
{
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started on port" << port;
    }
}

void ExpressionServer::newConnection()
{
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &ExpressionServer::readData);
    connect(socket, &QTcpSocket::disconnected, this, &ExpressionServer::clientDisconnected);
    qDebug() << "New client connected";
}

void ExpressionServer::readData()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    processRequest(socket, data);
}

void ExpressionServer::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socket->deleteLater();
        qDebug() << "Client disconnected";
    }
}

void ExpressionServer::processRequest(QTcpSocket* socket, const QByteArray& data)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 requestSize;
    QString expression;
    int operandCount;
    
    in >> requestSize >> expression >> operandCount;

    QMap<QString, double> operands;
    for (int i = 0; i < operandCount; ++i) {
        QString name;
        double value;
        in >> name >> value;
        operands[name] = value;
    }

    std::string rpn;
    if (!convertToRPN(expression, rpn)) {
        QByteArray response;
        QDataStream out(&response, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << quint32(1) << QString("Error in expression");
        socket->write(response);
        return;
    }

    double result;
    if (!calculateRPN(rpn, operands, result)) {
        QByteArray response;
        QDataStream out(&response, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << quint32(1) << QString("Calculation error");
        socket->write(response);
        return;
    }

    QByteArray response;
    QDataStream out(&response, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(0) << result;
    socket->write(response);
}

bool ExpressionServer::convertToRPN(const QString& expression, std::string& B)
{
    std::queue<char> exprQueue;
    for (QChar c : expression) {
        exprQueue.push(c.toLatin1());
    }
    
    char a;
    std::stack<char> stack1;
    bool indicator = true;
    char pred = 0;
    
    while (!exprQueue.empty() && indicator) {
        a = exprQueue.front();
        exprQueue.pop();

        if (a == '(' || a == '[' || a == '{') {
            if (pred != '+' && pred != '-' && pred != '*' && pred != '/' && pred != '('
                && pred != '[' && pred != '{' && pred != 0) {
                indicator = false;
            } else {
                stack1.push(a);
            }
        } else if ((a == '+' || a == '-' || a == '*' || a == '/') && exprQueue.empty()) {
            indicator = false;
        } else if (a == ')') {
            if (pred == '(') {
                indicator = false;
            } else {
                if (stack1.empty()) {
                    indicator = false;
                } else {
                    while (stack1.top() != '(' && indicator) {
                        if (stack1.empty()) {
                            indicator = false;
                        } else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            indicator = false;
                        } else if (stack1.top() == '[') {
                            indicator = false;
                        } else if (stack1.top() == '{') {
                            indicator = false;
                        } else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator)
                        stack1.pop();
                }
            }
        } else if (a == ']') {
            if (pred == '[') {
                indicator = false;
            } else {
                if (stack1.empty()) {
                    indicator = false;
                } else {
                    while (stack1.top() != '[' && indicator) {
                        if (stack1.empty()) {
                            indicator = false;
                        } else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            indicator = false;
                        } else if (stack1.top() == '(') {
                            indicator = false;
                        } else if (stack1.top() == '{') {
                            indicator = false;
                        } else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator)
                        stack1.pop();
                }
            }
        } else if (a == '}') {
            if (pred == '{') {
                indicator = false;
            } else {
                if (stack1.empty()) {
                    indicator = false;
                } else {
                    while (stack1.top() != '{' && indicator) {
                        if (stack1.empty()) {
                            indicator = false;
                        } else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            indicator = false;
                        } else if (stack1.top() == '[') {
                            indicator = false;
                        } else if (stack1.top() == '(') {
                            indicator = false;
                        } else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator)
                        stack1.pop();
                }
            }
        } else if ((a == '+' || a == '-' || a == '*' || a == '/') && stack1.empty() && pred != 0) {
            stack1.push(a);
        } else if ((a == '+' || a == '-')) {
            if (pred == 0 || pred == '(' || pred == '[' || pred == '{') {
                stack1.push(a);
                B.push_back('0');
                B.push_back(' ');
            } else if (pred == '*' || pred == '/' || pred == '+' || pred == '-') {
                indicator = false;
            } else {
                while (!stack1.empty() && stack1.top() != '(' && stack1.top() != '['
                       && stack1.top() != '{') {
                    B.push_back(stack1.top());
                    B.push_back(' ');
                    stack1.pop();
                }
                stack1.push(a);
            }
        } else if ((a == '*' || a == '/')) {
            if (pred == 0 || pred == '(' || pred == '[' || pred == '{') {
                indicator = false;
            } else if (pred == '*' || pred == '/' || pred == '+' || pred == '-') {
                indicator = false;
            } else {
                while (!stack1.empty() && stack1.top() != '(' && stack1.top() != '['
                       && stack1.top() != '{' && stack1.top() != '+' && stack1.top() != '-') {
                    B.push_back(stack1.top());
                    B.push_back(' ');
                    stack1.pop();
                }
                stack1.push(a);
            }
        } else {
            if (pred == ')' || pred == ']' || pred == '}') {
                indicator = false;
            } else if (pred != 0 && pred != '(' && pred != ')' && pred != '[' && pred != ']'
                       && pred != '{' && pred != '}' && pred != '+' && pred != '-' && pred != '*'
                       && pred != '/') {
                if (!B.empty())
                    B.pop_back();
            }
            B.push_back(a);
            B.push_back(' ');
        }
        pred = a;
    }

    while (!stack1.empty() && indicator) {
        if (stack1.top() == '(') {
            indicator = false;
        } else if (stack1.top() == '[') {
            indicator = false;
        } else if (stack1.top() == '{') {
            indicator = false;
        } else {
            B.push_back(stack1.top());
            B.push_back(' ');
            stack1.pop();
        }
    }

    return indicator && !B.empty();
}

bool ExpressionServer::calculateRPN(const std::string& rpn, const QMap<QString, double>& operands, double& finalResult)
{
    std::string B = rpn;
    B += ' ';
    std::stack<double> stack2;
    std::string token;
    size_t pos = 0;
    bool indicator = true;

    while ((pos = B.find(' ')) != std::string::npos && indicator) {
        token = B.substr(0, pos);
        B.erase(0, pos + 1);

        if (token.empty())
            continue;

        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stack2.size() < 2) {
                return false;
            }

            double b = stack2.top();
            stack2.pop();
            double a = stack2.top();
            stack2.pop();
            double result = 0;

            if (token == "+")
                result = a + b;
            else if (token == "-")
                result = a - b;
            else if (token == "*")
                result = a * b;
            else if (token == "/") {
                if (b == 0) {
                    return false;
                }
                result = a / b;
            }
            stack2.push(result);
        } else if (std::all_of(token.begin(), token.end(), [](char c) {
                       return std::isdigit(c) || c == '.' || c == ',';
                   })) {
            std::replace(token.begin(), token.end(), ',', '.');
            try {
                stack2.push(std::stod(token));
            } catch (const std::exception &) {
                return false;
            }
        } else {
            QString tokenQ = QString::fromStdString(token);
            if (!operands.contains(tokenQ)) {
                return false;
            }
            double value = operands[tokenQ];
            stack2.push(value);
        }
    }

    if (stack2.size() != 1) {
        return false;
    }
    
    finalResult = stack2.top();
    return true;
}
