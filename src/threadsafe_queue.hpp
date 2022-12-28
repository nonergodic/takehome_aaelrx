#ifndef THREADEDSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <deque>
#include <mutex>
#include <condition_variable>
//#include <thread>

template <typename T>
class ThreadsafeQueue
{
  public:

    void push(T const & item);
    T pop();

    size_t const size() const;

  private:
    std::condition_variable cond_;
    std::mutex mut_;
    std::deque<T> items_;
};

// ------------------------ IMPLEMENTATION ----------------------------------------

template <typename T>
void ThreadsafeQueue<T>::push(const T& item) {
  {
    auto lock = std::scoped_lock(mut_);
    items.push_back(item);
  }
  cond_.notify_one();
}

template <typename T>
T ThreadsafeQueue<T>::pop()
{
    auto lock = std::unique_lock(mut);
    cond.wait(lock, [](){return items.size()});

    T item = items.front();
    items.pop_front();
    return item;
}

template <typename T>
size_t const ThreadsafeQueue<T>::size() const
{
    auto lock = std::unique_lock(mut);
    return items.size();
}

#endif