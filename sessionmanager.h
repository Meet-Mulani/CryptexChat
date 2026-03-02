#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <unordered_map>
#include <mutex>
#include <QString>
#include <optional>
#include <QDateTime>

struct Session{
    QString username;
    int socketDescriptor=-1;
    QDateTime lastActive;
    bool online=false;
};

class SessionManager{
public:
    SessionManager()=default;
    void setOnline(const QString& username, int socketFd);
    void setOffline(const QString& username);
    bool isOnline(const QString& username);
    std::optional<Session> getSession(const QString& username);
    void removeSession(const QString& username);
    std::unordered_map<QString,Session> snapshot();

private:
    std::unordered_map<QString,Session> sessions_;
    mutable std::mutex mutex_;
};

#endif //SESSIONMANAGER_H
