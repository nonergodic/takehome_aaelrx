#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>

#include "common.hpp"

struct OutOfCapacity : public virtual Exception {};
struct StopRequested : public virtual Exception {};

template <typename>
struct DiscardOnNoCapacity {
  using PushGuard = std::scoped_lock<std::mutex>;

  template <typename... args>
  bool on_at_capacity(args...) {
    return false;
  }

  void on_push(std::condition_variable_any & cond) {
    cond.notify_one();
  }

  void on_pop(std::condition_variable_any &) {}
};

template <typename>
struct ThrowOnNoCapacity {
  using PushGuard = std::scoped_lock<std::mutex>;

  template <typename... args>
  bool on_at_capacity(args...) {
    throw make_exception<OutOfCapacity>("Out of capacity");
  }

  void on_push(std::condition_variable_any & cond) {
    cond.notify_one();
  }

  void on_pop(std::condition_variable_any &) {}
};

template <typename T>
struct WaitUntilCapacityAvailable
{
  using PushGuard = std::unique_lock<std::mutex>;

  bool on_at_capacity(
    std::unique_lock<std::mutex> & lock,
    std::condition_variable_any & cond,
    std::queue<T> & items,
    size_t capacity
  ) {
    cond.wait(lock, [&]() {return items.size() < capacity;});
    return true;
  }

  bool on_at_capacity(
    std::stop_token stop,
    std::unique_lock<std::mutex> & lock,
    std::condition_variable_any & cond,
    std::queue<T> & items,
    size_t capacity
  ) {
    cond.wait(lock, stop, [&]() {return items.size() < capacity;});
    return true;
  }

  void on_push(std::condition_variable_any & cond) {
    cond.notify_all();
  }

  void on_pop(std::condition_variable_any & cond) {
    cond.notify_all();
  }
};

//rather incomplete ThreadsafeQueue class
// - no timed waiting
// - could have try_pop returning std::optional
template <
  typename T,
  template<class> typename AtMaxCapacityPolicy = WaitUntilCapacityAvailable
>
class ThreadsafeQueue : public AtMaxCapacityPolicy<T>
{
  public:
    ThreadsafeQueue(size_t capacity) : capacity_{capacity} {}
    //can't be safely destroyed while in use given current impl
    //implicit move/copy ctors/assignments rightfully implicitly deleted because of mutex member

    void push(T && item) {
      {
        auto lock = typename AtMaxCapacityPolicy<T>::PushGuard{mut_};
        if (items_.size() == capacity_)
          if (!this->on_at_capacity(lock, cond_, items_, capacity_))
            return;
        items_.push(std::move(item));
      }
      this->on_push(cond_);
    }

    std::enable_if_t<std::is_same_v<AtMaxCapacityPolicy<T>, WaitUntilCapacityAvailable<T>>>
    push(std::stop_token stop, T && item) {
      {
        auto lock = typename AtMaxCapacityPolicy<T>::PushGuard{mut_};
        if (items_.size() == capacity_)
          this->on_at_capacity(stop, lock, cond_, items_, capacity_);
        if (stop.stop_requested())
          return;
        items_.push(std::move(item));
      }
      this->on_push(cond_);
    }

    T pop() {
      auto locked_part = [&]() {
        auto lock = std::unique_lock{mut_};
        cond_.wait(lock, [&]() {return !items_.empty();});
        auto item = std::move(items_.front());
        items_.pop();
        return item;
      };
      auto item = locked_part();
      this->on_pop(cond_);
      return item;
    }

    T pop(std::stop_token stop) {
      auto locked_part = [&]() {
        auto lock = std::unique_lock{mut_};
        cond_.wait(lock, stop, [&]() {return !items_.empty();});
        if (stop.stop_requested())
          throw make_exception<StopRequested>("");
        auto item = std::move(items_.front());
        items_.pop();
        return item;
      };
      auto item = locked_part();
      this->on_pop(cond_);
      return item;
    }

    auto empty() const {
      auto lock = std::scoped_lock{mut_};
      return items_.empty();
    }

    auto size() const {
      auto lock = std::scoped_lock{mut_};
      return items_.size();
    }

  private:
    size_t capacity_;
    std::mutex mutable mut_;
    std::condition_variable_any cond_;
    std::queue<T> items_;
};

#endif
