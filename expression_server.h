#ifndef EXPRESSION_SERVER_H
#define EXPRESSION_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QMap>
#include <QDebug>
#include <queue>
#include <stack>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <sstream>

class ExpressionServer : public QObject
{
    Q_OBJECT
public:
    explicit ExpressionServer(QObject *parent = nullptr);
    void start(quint16 port);

private slots:
    void newConnection();
    void readData();
    void clientDisconnected();

private:
    bool convertToRPN(const QString& expression, std::string& rpn);
    bool calculateRPN(const std::string& rpn, const QMap<QString, double>& operands, double& result);
    void processRequest(QTcpSocket* socket, const QByteArray& data);
    
    QTcpServer *tcpServer;
};

#endif // EXPRESSION_SERVER_H
