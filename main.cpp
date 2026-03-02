#include "mainwindow.h"
#include "setup_db.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    setup_chat_db(); // setup database
    // Ensure DB-dependent components can create tables/indexes
    ChatDatabaseHandler dbHandler;
    dbHandler.initialize();
    MainWindow w;

    w.show();
    return a.exec();
}