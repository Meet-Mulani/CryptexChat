//usersystem.h
#ifndef USERSYSTEM_H
#define USERSYSTEM_H

#include <QString>
#include <unordered_map>
#include "chatdbhandler.h"

struct UserSession{
    QString username;
    bool loggedIn;
    QDateTime lastActive;
};

class UserSystem{
public:
    UserSystem(ChatDatabaseHandler* dbHandler);
    QString generateSalt(int length);
    bool registerUser(const QString& username, const QString& email, const QString& password);
    bool loginUser(const QString& username,const QString& password);
    bool isUserOnline(const QString& username) const;
    void logoutUser(const QString& username);
    void setUserOnline(const QString& username);
    void setUserOffline(const QString& username);

private:
    ChatDatabaseHandler* db;
    std::unordered_map<QString,UserSession> activeSessions;

    QString hashPassword(const QString& password,const QString& salt);
    bool verifyPassword(const QString& password, const QString& storedHash);
};

#endif // USERSYSTEM_H
