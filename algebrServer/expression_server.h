#ifndef EXPRESSION_SERVER_H
#define EXPRESSION_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QElapsedTimer>
#include <QMap>
#include <QStack>
#include <QVector>
#include "Date.h"

enum class MessageType : quint16 {
    C2S_EXPRESSION_SUBMISSION,
    C2S_COEFFICIENTS_SUBMISSION,
    S2C_RPN_MATCH_REQUEST_COEFFS,
    S2C_RPN_MISMATCH_SEND_CORRECT,
    S2C_EXPRESSION_ERROR,
    S2C_FINAL_RESULT,
    S2C_CALCULATION_ERROR,
    S2C_PROTOCOL_ERROR
};

class ExpressionServer : public QObject
{
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
    void historyUpdated(const QVector<HistoryRecord> &history);
    void logMessage(const QString &message, const QString &color = "black");

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

    struct BinaryLogEntry {
        char expression[80];
        char rpn[80];
        unsigned char hour;
        unsigned char minute;
        unsigned char second;
    };

    struct CoefficientEntry {
        char name[8];
        double value;
    };

    bool isOpeningBracket(QChar c) const;
    bool isClosingBracket(QChar c) const;
    QChar getMatchingOpeningBracket(QChar closingBracket) const;
    QChar getMatchingClosingBracket(QChar openingBracket) const;
    bool isOperator(QChar c) const;
    int getPrecedence(QChar op) const;
    QChar getMatchingBracket(QChar bracket) const;
    bool isAllowedSymbol(QChar c) const;

    bool convertToRPN(const QString &expression, QString &rpn, QString &error);
    bool calculateRPN(const QString &rpn, const QMap<QString, double> &operands,
                      double &result, QString &error);
    void handleExpressionSubmission(QTcpSocket *socket, QDataStream &in);
    void handleCoefficientsSubmission(QTcpSocket *socket, QDataStream &in);
    void saveToBinaryLog(const QString &expr, const QString &rpn);
    void saveCoefficientsToBinary(const QMap<QString, double> &coeffs);
    void sendResponse(QTcpSocket *socket, MessageType type, const QString &payload = "");
    void addHistoryRecord(const QString &type, const QString &expr,
                          const QString &rpn, const QString &res, qint64 time);

    QTcpServer *tcpServer;
    QMap<QTcpSocket*, ClientInfo> clients;
    QVector<HistoryRecord> historyRecords;
};

#endif // EXPRESSION_SERVER_H
