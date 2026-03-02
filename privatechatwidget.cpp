#include "privatechatwidget.h"
#include <QDateTime>
#include <QTimer>

PrivateChatWidget::PrivateChatWidget(const QString &currentUserEmail, const QString &recipientEmail, const QString &recipientName, ChatDatabaseHandler &dbHandler, UserSystem &userSystem, QWidget *parent)
    : QWidget(parent), userEmail(currentUserEmail), recipientEmail(recipientEmail), recipientName(recipientName), dbHandler(dbHandler), userSystem(&userSystem)
{
    setupUI();
    // Set the recipient's email in the UI
    partnerNameLabel->setText(recipientName);
    partnerEmailLabel->setText(recipientEmail);
    loadChatHistory();

    // Setup refresh timer for chat history
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &PrivateChatWidget::loadChatHistory);
    refreshTimer->start(8000);
}

void PrivateChatWidget::setupUI()
{
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    // Set widget background
    setStyleSheet("QWidget { background-color: #2d2d2d; }");

    // Create header
    QWidget *headerWidget = new QWidget();
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setSpacing(5);

    QHBoxLayout *topHeaderLayout = new QHBoxLayout();

    chatPartnerLabel = new QLabel("Chat with: ");
    chatPartnerLabel->setFont(QFont("Arial", 12, QFont::Bold));
    chatPartnerLabel->setStyleSheet("color: #ffffff;");

    partnerNameLabel = new QLabel();
    partnerNameLabel->setFont(QFont("Arial", 12));
    partnerNameLabel->setStyleSheet("color: #ffffff;");

    partnerEmailLabel = new QLabel();
    partnerEmailLabel->setFont(QFont("Arial", 10));
    partnerEmailLabel->setStyleSheet("color: #9e9e9e;");

    leaveChatButton = new QPushButton("Back to Menu");
    leaveChatButton->setFixedWidth(120);
    leaveChatButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #424242;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #616161;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #2a82da;"
        "}"
        );

    topHeaderLayout->addWidget(chatPartnerLabel);
    topHeaderLayout->addWidget(partnerNameLabel);
    topHeaderLayout->addStretch();
    topHeaderLayout->addWidget(leaveChatButton);

    headerLayout->addLayout(topHeaderLayout);
    headerLayout->addWidget(partnerEmailLabel);

    // Create chat display
    chatHistoryDisplay = new QTextEdit();
    chatHistoryDisplay->setReadOnly(true);
    chatHistoryDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1d1d1d;"
        "   border: 1px solid #424242;"
        "   border-radius: 8px;"
        "   padding: 10px;"
        "   color: #ffffff;"
        "}"
        "QScrollBar:vertical {"
        "   background-color: #2d2d2d;"
        "   width: 12px;"
        "   margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: #424242;"
        "   min-height: 30px;"
        "   border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: #616161;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        );
    chatHistoryDisplay->setMinimumHeight(400);

    // Create message input area
    QWidget *messageWidget = new QWidget();
    QHBoxLayout *messageLayout = new QHBoxLayout(messageWidget);
    messageLayout->setSpacing(10);

    messageInputField = new QLineEdit();
    messageInputField->setPlaceholderText("Type your message here...");
    messageInputField->setStyleSheet(
        "QLineEdit {"
        "   background-color: #1d1d1d;"
        "   border: 1px solid #424242;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "   color: #ffffff;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #2a82da;"
        "}"
        );

    sendMessageButton = new QPushButton("Send");
    sendMessageButton->setFixedWidth(100);
    sendMessageButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2a82da;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3994ec;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1e5c99;"
        "}"
        );

    messageLayout->addWidget(messageInputField);
    messageLayout->addWidget(sendMessageButton);

    // Add widgets to main layout
    layout->addWidget(headerWidget);
    layout->addWidget(chatHistoryDisplay);
    layout->addWidget(messageWidget);

    // Connect signals
    connect(sendMessageButton, &QPushButton::clicked, this, &PrivateChatWidget::sendMessage);
    connect(messageInputField, &QLineEdit::returnPressed, this, &PrivateChatWidget::sendMessage);
    connect(leaveChatButton, &QPushButton::clicked, this, &PrivateChatWidget::backToMenuRequested);
}

void PrivateChatWidget::sendMessage()
{
    QString message = messageInputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Send message using UserSystem for online/offline logic
    if (dbHandler.sendDirectMessage(userEmail, recipientEmail, message, *userSystem)) {
        addOutgoingMessage(message);
        messageInputField->clear();
    } else {
        QMessageBox::warning(this, "Send Error", "Failed to send message. Please try again.");
    }
}

void PrivateChatWidget::loadChatHistory()
{
    // Get message history from database
    auto messages = dbHandler.getDirectMessageHistory(userEmail, recipientEmail, 50);

    // Clear existing messages
    clearChatHistory();

    // Add messages to chat display
    for (const auto &msg : messages) {
        QString senderUsername = std::get<0>(msg);
        QString receiverUsername = std::get<1>(msg);
        QString content = std::get<2>(msg);
        QDateTime timestamp = std::get<3>(msg);

        if (senderUsername == userEmail) {
            addOutgoingMessage(content, timestamp);
        } else {
            addIncomingMessage(senderUsername, receiverUsername, content, timestamp);
        }
    }

    // Scroll to bottom
    scrollToBottom();
}

void PrivateChatWidget::clearChatHistory()
{
    chatHistoryDisplay->clear();
}

void PrivateChatWidget::addSystemMessage(QDateTime msgTimestamp)
{
    QString timestampStr = formatTimestamp(msgTimestamp);
    QString html = QString("<div style='text-align: center; color: #9e9e9e; font-style: italic; margin: 10px 0;'>"
                          "Chat started %1</div>").arg(timestampStr);
    chatHistoryDisplay->append(html);
}

void PrivateChatWidget::addIncomingMessage(const QString &sender, const QString &email, const QString &message, const QDateTime &timestamp)
{
    QString timestampStr = formatTimestamp(timestamp);
    QString html = QString("<div style='margin: 10px 0;'>"
                          "<div style='color: #9e9e9e; font-size: small;'>%1 - %2</div>"
                          "<div style='background-color: #424242; color: white; padding: 10px; border-radius: 10px; margin-left: 0; margin-right: 50px;'>"
                          "%3</div></div>")
                          .arg(sender, timestampStr, message.toHtmlEscaped());
    chatHistoryDisplay->append(html);
    scrollToBottom();
}

void PrivateChatWidget::addOutgoingMessage(const QString &message, const QDateTime &timestamp)
{
    QString timestampStr = formatTimestamp(timestamp);
    QString html = QString("<div style='margin: 10px 0; text-align: right;'>"
                          "<div style='color: #9e9e9e; font-size: small;'>You - %1</div>"
                          "<div style='background-color: #2a82da; color: white; padding: 10px; border-radius: 10px; margin-left: 50px; margin-right: 0;'>"
                          "%2</div></div>")
                          .arg(timestampStr, message.toHtmlEscaped());
    chatHistoryDisplay->append(html);
    scrollToBottom();
}

void PrivateChatWidget::scrollToBottom()
{
    QScrollBar *scrollbar = chatHistoryDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

QString PrivateChatWidget::formatTimestamp(const QDateTime &timestamp)
{
    return timestamp.toString("hh:mm AP");
}