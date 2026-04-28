/**********************************************************************
MIT License

Copyright (c) 2025 Park Younghwan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************************************************/

#pragma once

#include <atomic>
#include <memory>
#include <functional>

namespace common
{
inline namespace v2
{

// Chase-Lev Work-Stealing Deque
// 소유자 스레드는 bottom에서 push/pop, 다른 스레드들은 top에서 steal
template<typename T>
class LockFreeWorkQueue
{
private:
    struct CircularArray
    {
        std::atomic<T*> _buffer;
        size_t _size;
        
        CircularArray(size_t size) : _size(size)
        {
            _buffer.store(new T[size]);
        }
        
        ~CircularArray()
        {
            delete[] _buffer.load();
        }
        
        T get(size_t index) const
        {
            return _buffer.load()[index % _size];
        }
        
        void put(size_t index, const T& item)
        {
            _buffer.load()[index % _size] = item;
        }
        
        std::unique_ptr<CircularArray> resize(size_t bottom, size_t top)
        {
            auto newArray = std::make_unique<CircularArray>(_size * 2);
            T* oldBuffer = _buffer.load();
            
            for (size_t i = top; i < bottom; ++i)
            {
                newArray->put(i, oldBuffer[i % _size]);
            }
            
            return newArray;
        }
    };    std::atomic<size_t> _top{0};
    std::atomic<size_t> _bottom{0};
    std::atomic<std::shared_ptr<CircularArray>> _array;
    
    // 성능 모니터링용 카운터들
    mutable std::atomic<size_t> _resizeCount{0};
    mutable std::atomic<size_t> _maxSize{0};

public:
    // 사용 패턴에 따른 초기 크기 설정
    explicit LockFreeWorkQueue(size_t initialSize = 256) 
        : _array(std::make_shared<CircularArray>(nextPowerOf2(initialSize)))
    {
    }

private:
    // 2의 거듭제곱으로 올림
    static constexpr size_t nextPowerOf2(size_t n) noexcept
    {
        if (n <= 1) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

public:    // 소유자 스레드가 bottom에 작업 추가
    void push(const T& item)
    {
        size_t bottom = _bottom.load(std::memory_order_relaxed);
        size_t top = _top.load(std::memory_order_acquire);
        
        auto array = _array.load(std::memory_order_relaxed);
          // 큐가 가득 찬 경우 크기 확장 (75% 사용률에서 확장)
        if (bottom - top >= array->_size * 3 / 4)
        {
            auto newArray = array->resize(bottom, top);
            _array.store(newArray, std::memory_order_release);
            array = newArray;
            _resizeCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        array->put(bottom, item);
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(bottom + 1, std::memory_order_relaxed);
        
        // 최대 크기 추적
        size_t currentSize = bottom - top + 1;
        size_t maxSize = _maxSize.load(std::memory_order_relaxed);
        while (currentSize > maxSize && 
               !_maxSize.compare_exchange_weak(maxSize, currentSize, std::memory_order_relaxed)) {
            // CAS 재시도
        }
    }

    // 소유자 스레드가 bottom에서 작업 제거
    bool pop(T& result)
    {
        size_t bottom = _bottom.load(std::memory_order_relaxed) - 1;
        auto array = _array.load(std::memory_order_relaxed);
        _bottom.store(bottom, std::memory_order_relaxed);
        
        std::atomic_thread_fence(std::memory_order_seq_cst);
        size_t top = _top.load(std::memory_order_relaxed);
        
        if (top <= bottom)
        {
            // 큐에 작업이 있음
            result = array->get(bottom);
            
            if (top == bottom)
            {
                // 마지막 작업인 경우 경쟁 상황 체크
                if (!_top.compare_exchange_strong(top, top + 1, 
                    std::memory_order_seq_cst, std::memory_order_relaxed))
                {
                    // steal에 의해 이미 가져감
                    _bottom.store(bottom + 1, std::memory_order_relaxed);
                    return false;
                }
                _bottom.store(bottom + 1, std::memory_order_relaxed);
            }
            return true;
        }
        else
        {
            // 큐가 비어있음
            _bottom.store(bottom + 1, std::memory_order_relaxed);
            return false;
        }
    }

    // 다른 스레드가 top에서 작업 훔쳐가기
    bool steal(T& result)
    {
        size_t top = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        size_t bottom = _bottom.load(std::memory_order_acquire);
        
        if (top < bottom)
        {
            auto array = _array.load(std::memory_order_consume);
            result = array->get(top);
            
            if (!_top.compare_exchange_strong(top, top + 1,
                std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                return false;  // 다른 스레드가 먼저 steal함
            }
            return true;
        }
        
        return false;  // 큐가 비어있음
    }

    // 큐가 비어있는지 확인 (정확하지 않을 수 있음, 힌트용)
    bool empty() const
    {
        size_t bottom = _bottom.load(std::memory_order_relaxed);
        size_t top = _top.load(std::memory_order_relaxed);
        return top >= bottom;
    }    // 큐의 대략적인 크기 (정확하지 않을 수 있음, 힌트용)
    size_t size() const
    {
        size_t bottom = _bottom.load(std::memory_order_relaxed);
        size_t top = _top.load(std::memory_order_relaxed);
        return bottom >= top ? bottom - top : 0;
    }
    
    // 성능 통계 조회
    size_t getResizeCount() const { return _resizeCount.load(std::memory_order_relaxed); }
    size_t getMaxSize() const { return _maxSize.load(std::memory_order_relaxed); }
    size_t getCapacity() const { return _array.load(std::memory_order_relaxed)->_size; }
};

} // v2
} // namespace common
