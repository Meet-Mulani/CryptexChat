#include "chatdbhandler.h"
#include "usersystem.h"
#include "encryptionutils.h"
#include "crypto_helpers.h"
#include <QSqlRecord>
#include <QVariant>

ChatDatabaseHandler::ChatDatabaseHandler(QObject *parent)
    : QObject(parent), dbInitialized(false) {}

ChatDatabaseHandler::~ChatDatabaseHandler() {
    if (db.isOpen()) db.close();
}

bool ChatDatabaseHandler::initialize() {
    if (dbInitialized) return true;

    db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "Database connection invalid or closed:" << db.lastError().text();
        return false;
    }

    // Ensure tables exist
    QSqlQuery query(db);
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE,
            email TEXT,
            password_hash TEXT,
            failed_attempts INTEGER DEFAULT 0
        )
    )");
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER,
            recipient_id INTEGER,
            ciphertext TEXT,
            metadata TEXT,
            timestamp TEXT,
            delivered INTEGER DEFAULT 0,
            FOREIGN KEY(sender_id) REFERENCES users(id),
            FOREIGN KEY(recipient_id) REFERENCES users(id)
        )
    )");
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_recipient ON messages(recipient_id)");

    dbInitialized = true;
    return true;
}

QString ChatDatabaseHandler::loginUser(const QString &username, const QString &password) {
    // Prefer using UserSystem for verification; this legacy path retained for compatibility
    if (!dbInitialized) return {};

    QSqlQuery query(db);
    query.prepare("SELECT password_hash, failed_attempts FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec() || !query.next()) return {};

    QString storedHex = query.value(0).toString();
    int attempts = query.value(1).toInt();

    // Verify using the same PBKDF2 scheme as UserSystem
    QByteArray stored = QByteArray::fromHex(storedHex.toLatin1());
    bool ok = false;
    if (stored.size() >= 48) {
        QByteArray salt = stored.left(16);
        QByteArray dk = stored.mid(16,32);
        QByteArray test = pbkdf2_hmac_sha256(password.toUtf8(), salt, 100000, dk.size());
        ok = (test == dk);
    }

    if (!ok) {
        attempts++;
        updateFailedAttempts(username, attempts);

        if (attempts >= 3) {
            deleteUndeliveredMessagesForUser(username);
            resetFailedAttempts(username);
            qDebug() << "3 failed attempts reached. Undelivered messages deleted.";
        }
        return {};
    }

    resetFailedAttempts(username);
    return username;
}

bool ChatDatabaseHandler::registerUser(const QString &username, const QString &email, const QString &passwordHash) {
    if (!dbInitialized) return false;

    QSqlQuery check(db);
    check.prepare("SELECT id FROM users WHERE username = :username OR email = :email");
    check.bindValue(":username", username);
    check.bindValue(":email", email);
    if (check.exec() && check.next()) return false;

    QSqlQuery insert(db);
    insert.prepare("INSERT INTO users (username, email, password_hash) VALUES (:u, :e, :p)");
    insert.bindValue(":u", username);
    insert.bindValue(":e", email);
    insert.bindValue(":p", passwordHash);
    return insert.exec();
}

bool ChatDatabaseHandler::updateFailedAttempts(const QString &username, int attempts) {
    QSqlQuery q(db);
    q.prepare("UPDATE users SET failed_attempts = :a WHERE username = :u");
    q.bindValue(":a", attempts);
    q.bindValue(":u", username);
    return q.exec();
}

bool ChatDatabaseHandler::resetFailedAttempts(const QString &username) {
    return updateFailedAttempts(username, 0);
}

int ChatDatabaseHandler::getFailedAttempts(const QString &username) {
    QSqlQuery q(db);
    q.prepare("SELECT failed_attempts FROM users WHERE username = :u");
    q.bindValue(":u", username);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}

QString ChatDatabaseHandler::getPasswordHash(const QString &username) {
    QSqlQuery q(db);
    q.prepare("SELECT password_hash FROM users WHERE username = :u");
    q.bindValue(":u", username);
    if (q.exec() && q.next()) return q.value(0).toString();
    return {};
}

QString ChatDatabaseHandler::userExists(const QString &username) {
    if (!dbInitialized) return {};
    QSqlQuery q(db);
    q.prepare("SELECT username FROM users WHERE username = :u LIMIT 1");
    q.bindValue(":u", username);
    if (q.exec() && q.next()) return q.value(0).toString();
    return {};
}

bool ChatDatabaseHandler::markMessageDelivered(int messageId) {
    QSqlQuery q(db);
    q.prepare("UPDATE messages SET delivered = 1 WHERE id = :id");
    q.bindValue(":id", messageId);
    return q.exec();
}

bool ChatDatabaseHandler::deleteUndeliveredMessagesForUser(const QString &username) {
    QSqlQuery q(db);
    q.prepare(R"(
        DELETE FROM messages
        WHERE receiver_id = (SELECT id FROM users WHERE username = :u)
        AND delivered = 0
    )");
    q.bindValue(":u", username);
    return q.exec();
}

void ChatDatabaseHandler::enqueueOfflineMessage(const Message &msg) {
    QMutexLocker locker(&queueMutex);
    offlineMessageQueue.push(msg);

    // Encrypt plaintext and serialize metadata
    EncryptionUtils enc;
    MessageEncryptionMetadata md;
    QByteArray ciphertext = enc.encryptMessage(msg.content.toUtf8(), md);
    QByteArray metadataJson = enc.serializeMetadata(md);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO messages (sender_id, recipient_id, ciphertext, metadata, timestamp, delivered)
        VALUES ((SELECT id FROM users WHERE username = :s),
                (SELECT id FROM users WHERE username = :r),
                :c, :m, :t, 0)
    )");
    q.bindValue(":s", msg.sender);
    q.bindValue(":r", msg.receiver);
    q.bindValue(":c", QString(ciphertext.toBase64()));
    q.bindValue(":m", QString::fromUtf8(metadataJson));
    q.bindValue(":t", msg.timestamp.toString(Qt::ISODate));
    if (!q.exec()) qDebug() << "Failed to queue message:" << q.lastError().text();
}

bool ChatDatabaseHandler::sendDirectMessage(const QString &sender, const QString &recipient, const QString &content, UserSystem &userSystem) {
    if (!dbInitialized || content.isEmpty()) return false;

    if (!userSystem.isUserOnline(recipient)) {
        enqueueOfflineMessage({sender, recipient, content, QDateTime::currentDateTime()});
        return true;
    }

    // Encrypt plaintext and serialize metadata for immediate delivery record
    EncryptionUtils enc;
    MessageEncryptionMetadata md;
    QByteArray ciphertext = enc.encryptMessage(content.toUtf8(), md);
    QByteArray metadataJson = enc.serializeMetadata(md);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO messages (sender_id, recipient_id, ciphertext, metadata, timestamp, delivered)
        VALUES ((SELECT id FROM users WHERE username = :s),
                (SELECT id FROM users WHERE username = :r),
                :c, :m, :t, 1)
    )");
    q.bindValue(":s", sender);
    q.bindValue(":r", recipient);
    q.bindValue(":c", QString(ciphertext.toBase64()));
    q.bindValue(":m", QString::fromUtf8(metadataJson));
    q.bindValue(":t", QDateTime::currentDateTime().toString(Qt::ISODate));
    return q.exec();
}

bool ChatDatabaseHandler::dequeueOfflineMessage(Message &msg) {
    QMutexLocker locker(&queueMutex);
    if (offlineMessageQueue.empty()) return false;
    msg = offlineMessageQueue.front();
    offlineMessageQueue.pop();
    return true;
}

QList<std::tuple<QString, QString, QString, QDateTime>> ChatDatabaseHandler::getDirectMessageHistory(const QString &u1, const QString &u2, int limit) {
    QList<std::tuple<QString, QString, QString, QDateTime>> messages;
    if (!dbInitialized) return messages;

    QSqlQuery q(db);
    q.prepare(R"(
        SELECT s.username, r.username, m.ciphertext, m.metadata, m.timestamp
        FROM messages m
        JOIN users s ON m.sender_id = s.id
        JOIN users r ON m.recipient_id = r.id
        WHERE (s.username = :u1 AND r.username = :u2)
           OR (s.username = :u2 AND r.username = :u1)
        ORDER BY m.timestamp DESC LIMIT :l
    )");
    q.bindValue(":u1", u1);
    q.bindValue(":u2", u2);
    q.bindValue(":l", limit);

    if (q.exec()) {
        while (q.next()) {
            const QString senderName = q.value(0).toString();
            const QString receiverName = q.value(1).toString();
            const QString ciphertextB64 = q.value(2).toString();
            const QString metadataStr = q.value(3).toString();
            const QDateTime ts = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);

            QString plaintext;
            if (!metadataStr.isEmpty()) {
                EncryptionUtils enc;
                MessageEncryptionMetadata md = enc.deserializeMetadata(metadataStr.toUtf8());
                QByteArray cipher = QByteArray::fromBase64(ciphertextB64.toUtf8());
                QByteArray plain = enc.decryptMessage(cipher, md);
                plaintext = QString::fromUtf8(plain);
            } else {
                // Fallback for legacy rows stored as plaintext
                plaintext = ciphertextB64;
            }

            messages.append({senderName, receiverName, plaintext, ts});
        }
        std::reverse(messages.begin(), messages.end());
    }
    return messages;
}
