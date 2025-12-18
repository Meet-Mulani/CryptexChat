🔐 CryptexChat – Secure Messaging with Layered Encryption

CryptexChat is a Qt-based desktop chat application that emphasizes security, privacy, and structured communication. It implements a layered encryption model and leverages multiple data structures for efficient message handling, session management, and real-time communication.

📱 UI Design Credit: The chat interface design is based on the work of Saw Zi Dunn(https://github.com/SawZiDunn/).

🧠 Key Features
Layered Encryption – Multiple encryption layers applied sequentially for enhanced security

End-to-End Security – AES-CBC encryption with secure key exchange

Offline Messaging – Queue-based storage for undelivered messages

Session Management – Hash table-based user session tracking

Thread-Safe Communication – Mutex-protected message queues for inter-thread messaging

Database Indexing – B-tree optimized message retrieval

Clean & Modern UI – Qt-based interface with dark theme and intuitive navigation

🏗️ Data Structures in Action
Vectors – Encryption layer and algorithm pool management

QList – Message history retrieval and database result buffering

Stack – UI page navigation and encryption/decryption processing

Queue – Offline message storage and thread-safe message passing

Hash Tables – Session management and client-socket mapping

Trees – UI widget hierarchy and JSON metadata structuring

Graphs – Implicit user connection modeling and session state transitions

🛠️ Tech Stack
C++ with Qt Framework

SQLite for persistent storage

AES-CBC for message encryption

STL & Qt Containers for data structure implementations

📁 Project Highlights
Modular architecture with clear separation of concerns

Real-time chat with online/offline status updates

Secure user authentication and session handling

Scalable design supporting multiple concurrent users
