//encryptionutils.cpp
#include "encryptionutils.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QByteArray>
#include <QJsonParseError>

EncryptionUtils::EncryptionUtils(){
    m_algorithmPool={
    CryptoAlgorithm::AES_128,
    CryptoAlgorithm::AES_256,
    CryptoAlgorithm::XOR,
    CryptoAlgorithm::DES,
    CryptoAlgorithm::TripleDES,
    CryptoAlgorithm::Blowfish,
    CryptoAlgorithm::RC4,
    CryptoAlgorithm::ChaCha20,
    CryptoAlgorithm::Camellia_256,
    CryptoAlgorithm::AES_256
    };
}

QByteArray EncryptionUtils::generateRandomKey(int length){
    QByteArray key;
    for (int i = 0; i < length; i++) {
        key.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return key;
}

QByteArray EncryptionUtils::generateRandomIv(int length){
    QByteArray iv;
    for (int i = 0; i < length; i++) {
        iv.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return iv;
}

QByteArray EncryptionUtils::applyEncryption(const QByteArray &input, const EncryptionLayerMetadata& layer) {
    QByteArray result = input;

    switch (layer.algorithm) {
    case CryptoAlgorithm::XOR:
        for (int i = 0; i < result.size(); ++i)
            result[i] = result[i] ^ layer.key[i % layer.key.size()];
        break;

    default: {
        // reversible byte-shift based on key and iv
        int shift = static_cast<unsigned char>(layer.key.at(0)) % 13 + static_cast<unsigned char>(layer.iv.at(0)) % 7;
        for (int i = 0; i < result.size(); ++i)
            result[i] = static_cast<char>((result[i] + shift) % 256);
        break;
    }
    }

    return result;
}

QByteArray EncryptionUtils::applyDecryption(const QByteArray &input, const EncryptionLayerMetadata& layer) {
    QByteArray result = input;

    switch (layer.algorithm) {
    case CryptoAlgorithm::XOR:
        for (int i = 0; i < result.size(); ++i)
            result[i] = result[i] ^ layer.key[i % layer.key.size()];
        break;

    default: {
        int shift = static_cast<unsigned char>(layer.key.at(0)) % 13 + static_cast<unsigned char>(layer.iv.at(0)) % 7;
        for (int i = 0; i < result.size(); ++i)
            result[i] = static_cast<char>((result[i] - shift + 256) % 256);
        break;
    }
    }

    return result;
}

MessageEncryptionMetadata EncryptionUtils::createRandomLayeredMetadata(int layers){
    MessageEncryptionMetadata metadata;
    QRandomGenerator *rng=QRandomGenerator::global();

    for (int i = 0; i < layers; ++i) {
        int idx=rng->bounded(m_algorithmPool.size());
        CryptoAlgorithm algo=m_algorithmPool[idx];
        QByteArray key=generateRandomKey(16);
        QByteArray iv=generateRandomIv(16);

        EncryptionLayerMetadata layer{algo,key,iv};
        metadata.layers.push_back(layer);
    }
    return metadata;
}

QByteArray EncryptionUtils::encryptMessage(const QByteArray& plaintext,MessageEncryptionMetadata& metadata){
    metadata.layers.clear();
    QByteArray currentData=plaintext;
    QRandomGenerator *rng=QRandomGenerator::global();

    for (int i = 0; i < 5; ++i) {
        int idx=rng->bounded(m_algorithmPool.size());
        CryptoAlgorithm algo=m_algorithmPool[idx];

        QByteArray key=generateRandomKey(16);
        QByteArray iv=generateRandomIv(16);

        EncryptionLayerMetadata layer{algo,key,iv};
        currentData=applyEncryption(currentData,layer);
        metadata.layers.push_back(layer);
    }
    return currentData;
}

QByteArray EncryptionUtils::decryptMessage(const QByteArray& ciphertext,const MessageEncryptionMetadata& metadata){
    QByteArray currentData=ciphertext;
    for (int i = metadata.layers.size()-1; i >=0; --i) {
        const EncryptionLayerMetadata &layer=metadata.layers[i];
        currentData=applyDecryption(currentData,layer);
    }
    return currentData;
}

// Helper: Convert CryptoAlgorithm enum to integer for JSON
int cryptoAlgorithmToInt(CryptoAlgorithm algo) {
    return static_cast<int>(algo);
}

// Helper: Convert integer from JSON back to CryptoAlgorithm enum
CryptoAlgorithm intToCryptoAlgorithm(int value) {
    return static_cast<CryptoAlgorithm>(value);
}

// Serialize MessageEncryptionMetadata to a JSON string
QByteArray EncryptionUtils::serializeMetadata(const MessageEncryptionMetadata &metadata) {
    QJsonArray jsonLayers;
    for (const auto &layer : metadata.layers) {
        QJsonObject layerObj;
        layerObj["algorithm"] = cryptoAlgorithmToInt(layer.algorithm);
        layerObj["key"] = QString(layer.key.toBase64());
        layerObj["iv"] = QString(layer.iv.toBase64());
        jsonLayers.append(layerObj);
    }
    QJsonObject root;
    root["layers"] = jsonLayers;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

// Deserialize MessageEncryptionMetadata from JSON string
MessageEncryptionMetadata EncryptionUtils::deserializeMetadata(const QByteArray &data) {
    MessageEncryptionMetadata metadata;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        // Handle parse error (log or throw)
        return metadata;
    }
    if (!doc.isObject()) return metadata;
    QJsonObject root = doc.object();
    if (!root.contains("layers") || !root["layers"].isArray()) return metadata;

    QJsonArray jsonLayers = root["layers"].toArray();

    for (const auto &value : jsonLayers) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();
        EncryptionLayerMetadata layer;
        if (obj.contains("algorithm") && obj["algorithm"].isDouble()) {
            layer.algorithm = intToCryptoAlgorithm(obj["algorithm"].toInt());
        }
        if (obj.contains("key") && obj["key"].isString()) {
            layer.key = QByteArray::fromBase64(obj["key"].toString().toUtf8());
        }
        if (obj.contains("iv") && obj["iv"].isString()) {
            layer.iv = QByteArray::fromBase64(obj["iv"].toString().toUtf8());
        }
        metadata.layers.push_back(layer);
    }

    return metadata;
}
