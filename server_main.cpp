#include <QCoreApplication>
#include "expression_server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    ExpressionServer server;
    server.start(12345); // Сервер на порту 12345
    
    return app.exec();
}