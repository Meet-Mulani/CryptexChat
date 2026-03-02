#include "crypto_helpers.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <algorithm>
#include <cstring>
extern "C" {
#include "tiny_aes.h" // provides AES_init_ctx, AES_ECB_encrypt/decrypt or similar functions
}

QByteArray hmacSha256(const QByteArray &key, const QByteArray &data) {
    const int blockSize = 64;
    QByteArray k = key;
    if (k.size() > blockSize) k = QCryptographicHash::hash(k, QCryptographicHash::Sha256);
    if (k.size() < blockSize) k.append(QByteArray(blockSize - k.size(), '\0'));
    QByteArray o_key_pad, i_key_pad;
    for (int i=0;i<blockSize;i++){
        o_key_pad.append(k[i] ^ 0x5c);
        i_key_pad.append(k[i] ^ 0x36);
    }
    QByteArray inner = QCryptographicHash::hash(i_key_pad + data, QCryptographicHash::Sha256);
    QByteArray mac = QCryptographicHash::hash(o_key_pad + inner, QCryptographicHash::Sha256);
    return mac;
}

QByteArray pbkdf2_hmac_sha256(const QByteArray &password, const QByteArray &salt, int iterations, int dkLen) {
    const int hLen = 32;
    int l = (dkLen + hLen - 1) / hLen;
    QByteArray dk;
    for (int i = 1; i <= l; ++i) {
        QByteArray int_i;
        int_i.append((char)((i >> 24) & 0xFF));
        int_i.append((char)((i >> 16) & 0xFF));
        int_i.append((char)((i >> 8) & 0xFF));
        int_i.append((char)((i) & 0xFF));
        QByteArray U = hmacSha256(password, salt + int_i);
        QByteArray T = U;
        for (int j = 1; j < iterations; ++j) {
            U = hmacSha256(password, U);
            for (int k = 0; k < hLen; ++k) T[k] = T[k] ^ U[k];
        }
        dk.append(T);
    }
    dk.truncate(dkLen);
    return dk;
}

QByteArray secureRandomBytes(int n) {
    QByteArray out;
    out.resize(n);
    for (int i=0;i<n;i+=4) {
        quint32 r = QRandomGenerator::global()->generate();
        int take = qMin(4, n - i);
        for (int j=0;j<take;j++) out[i+j] = char((r >> (8*j)) & 0xFF);
    }
    return out;
}

static void xor_block(uint8_t *dst, const uint8_t *a, const uint8_t *b, int n){
    for(int i=0;i<n;i++) dst[i] = a[i] ^ b[i];
}

QByteArray aesCbcEncrypt(const QByteArray &key32, const QByteArray &iv16, const QByteArray &plaintext) {
    QByteArray data = plaintext;
    int pad = 16 - (data.size() % 16);
    data.append(QByteArray(pad, char(pad)));
    QByteArray out;
    out.resize(data.size());
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, reinterpret_cast<const uint8_t*>(key32.constData()));
    QByteArray prev = iv16;
    for (int i=0;i<data.size(); i+=16) {
        QByteArray block = data.mid(i,16);
        QByteArray x(16, '\0');
        xor_block(reinterpret_cast<uint8_t*>(x.data()),
                  reinterpret_cast<const uint8_t*>(block.constData()),
                  reinterpret_cast<const uint8_t*>(prev.constData()),16);
        uint8_t tmp[16];
        memcpy(tmp, x.constData(), 16);
        AES_ECB_encrypt(&ctx, tmp);
        memcpy(out.data()+i, tmp, 16);
        prev = QByteArray(reinterpret_cast<const char*>(tmp), 16);
    }
    return out;
}

QByteArray aesCbcDecrypt(const QByteArray &key32, const QByteArray &iv16, const QByteArray &ciphertext) {
    QByteArray out;
    out.resize(ciphertext.size());
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, reinterpret_cast<const uint8_t*>(key32.constData()));
    QByteArray prev = iv16;
    for (int i=0;i<ciphertext.size(); i+=16) {
        uint8_t tmp[16];
        memcpy(tmp, ciphertext.constData()+i, 16);
        AES_ECB_decrypt(&ctx, tmp);
        QByteArray block(reinterpret_cast<char*>(tmp),16);
        QByteArray plain(16,'\0');
        xor_block(reinterpret_cast<uint8_t*>(plain.data()),
                  reinterpret_cast<const uint8_t*>(block.constData()),
                  reinterpret_cast<const uint8_t*>(prev.constData()),16);
        memcpy(out.data()+i, plain.constData(), 16);
        prev = QByteArray(ciphertext.constData()+i,16);
    }
    int pad = static_cast<unsigned char>(out.constData()[out.size()-1]);
    if (pad > 0 && pad <=16) out.chop(pad);
    return out;
}

QString toHex(const QByteArray &b){ return QString::fromLatin1(b.toHex()); }
QByteArray fromHex(const QString &s){ return QByteArray::fromHex(s.toLatin1()); }

QJsonObject makeLayerMetadata(int algId, const QByteArray &iv, const QByteArray &hmac) {
    QJsonObject o;
    o["alg"] = algId;
    o["iv"] = QString::fromLatin1(iv.toHex());
    o["hmac"] = QString::fromLatin1(hmac.toHex());
    return o;
}
