#ifndef SHARED_QUEUE_HPP
#define SHARED_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>
#include <set>

template<typename T>
class shQueue {
   std::queue<T> queue_;
   std::set<T> set_;
   mutable std::mutex m_;
   std::condition_variable data_cond_;

   shQueue& operator=(const shQueue&) = delete;
   shQueue(const shQueue& other) = delete;

public:
   shQueue() {}

   void push(T item) {
      {
         std::lock_guard<std::mutex> lock(m_);
         if (set_.find(item) == set_.end())
         {
             queue_.push(std::move(item));
             set_.emplace(queue_.back());
         }
      }
      data_cond_.notify_one();
   }

   /// \return immediately, with true if successful retrieval
   bool try_and_pop(T& popped_item) {
      std::lock_guard<std::mutex> lock(m_);
      if (queue_.empty()) {
         return false;
      }
      popped_item = std::move(queue_.front());
      queue_.pop();
      return true;
   }

   /// Try to retrieve, if no items, wait till an item is available and try again
   void wait_and_pop(T& popped_item) {
      std::unique_lock<std::mutex> lock(m_);
      while (queue_.empty()) {
        data_cond_.wait(lock);
      }
      popped_item = std::move(queue_.front());
      queue_.pop();
   }

   bool empty() const {
      std::lock_guard<std::mutex> lock(m_);
      return queue_.empty();
   }

   unsigned size() const {
      return queue_.size();
   }

   void clear()
   {
      std::queue<T> empty;
      std::swap(queue_, empty);
      std::set<T> empty_s;
      std::swap(set_, empty_s);
   }
};

#endif // SHARED_QUEUE_HPP
