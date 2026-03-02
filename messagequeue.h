#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "message.h"

class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;

    //Push message in the queue thread-safe
    void push(const Message &m) {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            q_.push(m);
        }
        cv_.notify_one();
    }

    std::optional<Message> tryPop() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (q_.empty()) return std::nullopt;
        Message m = q_.front();
        q_.pop();
        return m;
    }

    Message waitPop() {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this]{ return !q_.empty() || stop_; });
        if (q_.empty()) return Message();
        Message m = q_.front();
        q_.pop();
        return m;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        std::queue<Message> empty;
        std::swap(q_, empty);
    }

    size_t size() {
        std::lock_guard<std::mutex> lk(mutex_);
        return q_.size();
    }

    void requestStop() {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
    }

private:
    std::queue<Message> q_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

#endif // MESSAGEQUEUE_H
