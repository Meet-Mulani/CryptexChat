// chatdbhandler.h
#ifndef CHATDBHANDLER_H
#define CHATDBHANDLER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QPair>
#include <QDateTime>
#include <QDebug>
#include <tuple>
#include <queue>
#include <QMutex>

struct Message {
    QString sender;
    QString receiver;
    QString content;
    QDateTime timestamp;
};

class UserSystem;

class ChatDatabaseHandler : public QObject
{
    Q_OBJECT

public:
    explicit ChatDatabaseHandler(QObject *parent = nullptr);
    ~ChatDatabaseHandler();

    // Database setup
    bool initialize();

    // User operations
    QString loginUser(const QString &username, const QString &password);
    QString userExists(const QString & username);
    QString addUser(const QString& username,const QString& hash);
    QString getPasswordHash(const QString& username);
    bool registerUser(const QString &username, const QString &email, const QString &password);
    bool updateFailedAttempts(const QString& username, int attempts);
    bool resetFailedAttempts(const QString& username);
    int getFailedAttempts(const QString& username);

    // Message operations
    void enqueueOfflineMessage(const Message& msg);
    bool sendDirectMessage(const QString &sender, const QString &recipient, const QString &content, UserSystem& userSystem);
    bool markMessageDelivered(int messageId);
    bool deleteUndeliveredMessagesForUser(const QString& username);
    bool dequeueOfflineMessage(Message& msg);
    bool saveMessage(int senderId, int recipientId, const QString &ciphertext, const QString &metadata);
    bool getOfflineMessages(int userId, QVector<Message> &messages, QVector<QString> &metadata);

    // Using std::tuple<sender_name, sender_email, content, timestamp>
    QList<std::tuple<QString, QString, QString, QDateTime>> getDirectMessageHistory(const QString &user1, const QString &user2, int limit);

private:
    QSqlDatabase db;
    bool dbInitialized;

    bool executeQuery(const QString &sql);
    bool checkTableExists(const QString &tableName);
    std::queue<Message> offlineMessageQueue;
    mutable QMutex queueMutex;
};

#endif // CHATDBHANDLER_H
