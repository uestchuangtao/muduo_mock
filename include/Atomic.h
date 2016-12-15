#ifndef ATOMIC_H
#define ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace detail
{
template <typename T>
class AtomicIntegerT : boost::noncopyable
{
  public:
    AtomicIntegerT()
        : M_value(0)
    {
    }

    T get()
    {
        return __sync_val_compare_and_swap(&M_value, 0, 0);
    }

    T getAndAdd(T x)
    {
        return __sync_fetch_and_add(&M_value, x);
    }

    T addAndGet(T x)
    {
        return getAndAdd(x) + x;
    }

    T incrementAndGet()
    {
        return addAndGet(1);
    }

    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

    T getAndSet(T newValue)
    {
        return __sync_lock_test_and_set(&M_value, newValue);
    }

  private:
    volatile T M_value;
};
}

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64; 

#endif