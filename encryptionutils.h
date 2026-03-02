//encryptionutils.h
#ifndef ENCRYPTIONUTILS_H
#define ENCRYPTIONUTILS_H

#pragma once
#include <QString>
#include <QVector>
#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>

enum class CryptoAlgorithm{
    AES_128,
    AES_256,
    XOR,
    DES,
    TripleDES,
    Blowfish,
    RC4,
    ChaCha20,
    Camellia_256,
};

struct EncryptionLayerMetadata{
    CryptoAlgorithm algorithm;
    QByteArray key;
    QByteArray iv;
};

struct MessageEncryptionMetadata{
    QVector<EncryptionLayerMetadata> layers;
    QByteArray encryptedLayerMetadata;
};

class EncryptionUtils{
public:
    EncryptionUtils();
    QByteArray encryptMessage(const QByteArray& plaintext, MessageEncryptionMetadata& metadata);
    QByteArray decryptMessage(const QByteArray& ciphertext, const MessageEncryptionMetadata& metadata);
    MessageEncryptionMetadata createRandomLayeredMetadata(int layers = 5);
    QByteArray serializeMetadata(const MessageEncryptionMetadata &metadata);
    MessageEncryptionMetadata deserializeMetadata(const QByteArray &data);

private:
    QVector<CryptoAlgorithm> m_algorithmPool;
    QByteArray generateRandomKey(int length);
    QByteArray generateRandomIv(int length);
    QByteArray applyEncryption(const QByteArray& input,const EncryptionLayerMetadata& layer);
    QByteArray applyDecryption(const QByteArray& input,const EncryptionLayerMetadata& layer);
};

#endif // ENCRYPTIONUTILS_H
