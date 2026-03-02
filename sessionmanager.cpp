#include "sessionmanager.h"

void SessionManager::setOnline(const QString& username,int socketFd){
    std::lock_guard<std::mutex> lk(mutex_);
    Session s;
    s.username=username;
    s.socketDescriptor=socketFd;
    s.lastActive=QDateTime::currentDateTime();
    s.online=true;
    sessions_[username]=s;
}

void SessionManager::setOffline(const QString& username){
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(username);
    if (it != sessions_.end()) {
        it->second.online = false;
        it->second.socketDescriptor = -1;
        it->second.lastActive = QDateTime::currentDateTimeUtc();
    }
}

bool SessionManager::isOnline(const QString& username) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(username);
    return it != sessions_.end() && it->second.online;
}

std::optional<Session> SessionManager::getSession(const QString& username) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(username);
    if (it == sessions_.end()) return std::nullopt;
    return it->second;
}

void SessionManager::removeSession(const QString& username) {
    std::lock_guard<std::mutex> lk(mutex_);
    sessions_.erase(username);
}

std::unordered_map<QString, Session> SessionManager::snapshot() {
    std::lock_guard<std::mutex> lk(mutex_);
    return sessions_;
}
