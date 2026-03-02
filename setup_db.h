#ifndef SETUP_DB_H
#define SETUP_DB_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

void setup_chat_db() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("chat_database.db");

    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return;
    }

    // Create users table
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "username TEXT UNIQUE NOT NULL,"
                    "email TEXT UNIQUE NOT NULL,"
                    "password_hash TEXT NOT NULL,"
                    "failed_attempts INTEGER DEFAULT 0)")) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
    }

    // Create messages table
    if (!query.exec("CREATE TABLE IF NOT EXISTS messages ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "sender_id INTEGER NOT NULL,"
                    "recipient_id INTEGER,"
                    "ciphertext TEXT NOT NULL,"
                    "metadata TEXT NOT NULL,"
                    "timestamp DATETIME NOT NULL,"
                    "delivered INTEGER DEFAULT 0,"
                    "FOREIGN KEY(sender_id) REFERENCES users(id),"
                    "FOREIGN KEY(recipient_id) REFERENCES users(id))")) {
        qDebug() << "Failed to create messages table:" << query.lastError().text();
    }

}

#endif // SETUP_DB_H
