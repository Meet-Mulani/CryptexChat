#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVector>

struct EncryptionLayerMetadata{
    int algorithmId;
    QByteArray key;
    QByteArray iv;
};

struct MessageEncryptionMetadata{
    QVector<EncryptionLayerMetadata> layers;
    QByteArray hmac;
};

struct Message{
    qint64 id=-1;
    QString sender;
    QString receiver;
    QByteArray ciphertext;
    MessageEncryptionMetadata metadata;
    QDateTime timestamp;
    bool delivered=false;
};

#endif // MESSAGE_H
