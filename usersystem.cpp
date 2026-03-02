//usersystem.cpp
#include "usersystem.h"
#include "crypto_helpers.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

UserSystem::UserSystem(ChatDatabaseHandler* dbHandler) : db(dbHandler){}

QString UserSystem::generateSalt(int length){
    const QString chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    QString salt;
    for (int i = 0; i < length; i++) {
        int index=QRandomGenerator::global()->bounded(chars.length());
        salt.append(chars.at(index));
    }
    return salt;
}

QString UserSystem::hashPassword(const QString& password,const QString& salt){
    QByteArray salts = secureRandomBytes(16);
    QByteArray dk = pbkdf2_hmac_sha256(password.toUtf8(), salts, 100000, 32);
    QByteArray store = salts + dk; // 16 + 32 = 48 bytes
    return QString::fromLatin1(store.toHex());
}

bool UserSystem::verifyPassword(const QString &password, const QString& storedHex) {
    QByteArray stored = QByteArray::fromHex(storedHex.toLatin1());
    if (stored.size() < 48) return false;
    QByteArray salt = stored.left(16);
    QByteArray dk = stored.mid(16,32);
    QByteArray test = pbkdf2_hmac_sha256(password.toUtf8(), salt, 100000, dk.size());
    return test == dk;
}


void UserSystem::setUserOnline(const QString& username){
    activeSessions[username]={username,true,QDateTime::currentDateTime()};
}

void UserSystem::setUserOffline(const QString& username){
    activeSessions.erase(username);
}

bool UserSystem::isUserOnline(const QString& username) const {
    auto it = activeSessions.find(username);
    return it != activeSessions.end() && it->second.loggedIn;
}

bool UserSystem::registerUser(const QString& username, const QString& email, const QString& password){
    // Store format: 16-byte random salt + 32-byte PBKDF2 output as hex
    QByteArray salts = secureRandomBytes(16);
    QByteArray dk = pbkdf2_hmac_sha256(password.toUtf8(), salts, 100000, 32);
    QByteArray store = salts + dk; // 48 bytes
    QString hexStore = QString::fromLatin1(store.toHex());
    return db->registerUser(username, email, hexStore);
}

bool UserSystem::loginUser(const QString& username, const QString& password){
    QString storedHash=db->getPasswordHash(username);
    if(storedHash.isEmpty()){
        return false;
    }    

    if(!verifyPassword(password,storedHash)){
        int attempts=db->getFailedAttempts(username)+1;
        db->updateFailedAttempts(username,attempts);

        if(attempts>=3){
            db->deleteUndeliveredMessagesForUser(username);
            db->resetFailedAttempts(username);
        }
        return false;
    }
    db->resetFailedAttempts(username);
    activeSessions[username]={username,true};
    setUserOnline(username);
    return true;
}

void UserSystem::logoutUser(const QString& username){
    activeSessions.erase(username);
    setUserOffline(username);
}
