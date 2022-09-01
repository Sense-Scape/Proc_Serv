#ifndef CIRCULARBUFFER
#define CIRCULARBUFFER

#include <mutex>
#include <memory>

template <class T>
class CircularBuffer
{

public:
    explicit CircularBuffer(unsigned size) : buf_(std::unique_ptr<T[]>(new T[size])),
        max_size_(size)
    { /* empty */
    }
    ~CircularBuffer() {};

    void put(T item);
    T get();
    void reset();
    bool empty() const;
    bool full() const;
    unsigned capacity() const;
    unsigned size() const;

private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    unsigned head_ = 0;
    unsigned tail_ = 0;
    const unsigned max_size_;
    bool full_ = 0;
};

template <class T>
void CircularBuffer<T>::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
}

template <class T>
bool CircularBuffer<T>::empty() const
{
    // if head and tail are equal, we are empty
    return (!full_ && (head_ == tail_));
}

template <class T>
bool CircularBuffer<T>::full() const
{
    // If tail is ahead the head by 1, we are full
    return full_;
}

template <class T>
unsigned CircularBuffer<T>::capacity() const
{
    return max_size_;
}

template <class T>
unsigned CircularBuffer<T>::size() const
{
    unsigned size = max_size_;

    if (!full_)
    {
        if (head_ >= tail_)
        {
            size = head_ - tail_;
        }
        else
        {
            size = max_size_ + head_ - tail_;
        }
    }

    return size;
}

template <class T>
void CircularBuffer<T>::put(T item)
{
    std::lock_guard<std::mutex> lock(mutex_);

    buf_[head_] = item;

    if (full_)
    {
        tail_ = (tail_ + 1) % max_size_;
    }

    head_ = (head_ + 1) % max_size_;

    full_ = head_ == tail_;
}

template <class T>
T CircularBuffer<T>::get()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (empty())
    {
        return T();
    }

    // Read data and advance the tail (we now have a free space)
    auto val = buf_[tail_];
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;

    return val;
}

#endif