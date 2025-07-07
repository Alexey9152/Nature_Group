#ifndef EXPRESSION_SERVER_H
#define EXPRESSION_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QMap>
#include <QVector>
#include <QElapsedTimer>
#include "Date.h"

enum class MessageType : quint16 {
    C2S_EXPRESSION_SUBMISSION,   C2S_COEFFICIENTS_SUBMISSION,
    S2C_RPN_MATCH_REQUEST_COEFFS, S2C_RPN_MISMATCH_SEND_CORRECT,
    S2C_EXPRESSION_ERROR,         S2C_FINAL_RESULT,
    S2C_CALCULATION_ERROR,        S2C_PROTOCOL_ERROR
};

class ExpressionServer : public QObject {
    Q_OBJECT
public:
    struct HistoryRecord {
        Date dateTime;
        QString requestType;
        QString expression;
        QString rpn;
        QString result;
        qint64 processingTime;
    };

    explicit ExpressionServer(QObject *parent = nullptr);
    void start(quint16 port);

public slots:
    void saveHistoryToFile();

signals:
    void historyUpdated(const QVector<HistoryRecord>& history);
    void logMessage(const QString& message, const QString& color = "black");

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    struct ClientInfo {
        enum State { WaitingForExpression, WaitingForCoefficients };
        State state = WaitingForExpression;
        QString expression;
        QString rpn;
        QElapsedTimer timer;
    };

    void handleExpressionSubmission(QTcpSocket* socket, QDataStream& in);
    void handleCoefficientsSubmission(QTcpSocket* socket, QDataStream& in);
    int getPrecedence(char op);
    bool isOperator(char c);
    bool convertToRPN(const QString& expression, QString& rpn, QString& error);
    bool calculateRPN(const QString& rpn, const QMap<QString, double>& operands, double& result, QString& error);
    void sendResponse(QTcpSocket* socket, MessageType type, const QString& payload = "");
    void addHistoryRecord(const QString& type, const QString& expr, const QString& rpn, const QString& res, qint64 time);

    QTcpServer *tcpServer;
    QHash<QTcpSocket*, ClientInfo> clients;
    QVector<HistoryRecord> history;
};

#endif // EXPRESSION_SERVER_H
