#pragma once
#include <QByteArray>
#include <QString>
#include <QJsonObject>

QByteArray hmacSha256(const QByteArray &key, const QByteArray &data);
QByteArray pbkdf2_hmac_sha256(const QByteArray &password, const QByteArray &salt, int iterations, int dkLen);
QByteArray aesCbcEncrypt(const QByteArray &key32, const QByteArray &iv16, const QByteArray &plaintext);
QByteArray aesCbcDecrypt(const QByteArray &key32, const QByteArray &iv16, const QByteArray &ciphertext);
QByteArray secureRandomBytes(int n);
QString toHex(const QByteArray &b);
QByteArray fromHex(const QString &s);
QJsonObject makeLayerMetadata(int algId, const QByteArray &iv, const QByteArray &hmac);
