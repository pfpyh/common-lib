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
#include <array>
#include <vector>
#include <memory>

namespace common
{
/**
 * @brief Double buffering container
 * 
 * This container provides thread-safe access to a collection of items without using locks.
 * It maintains two buffers and switches between them atomically when new items are added.
 * This design allows readers to access a stable snapshot while writers prepare the next version.
 * 
 * @warning This implementation is only safe for single-writer / multi-reader scenarios. 
 *          For high-performance critical applications, consider implementing optimized 
 *          lock-free data structures directly rather than using this general-purpose container.
 * 
 * @tparam Ty The type of elements stored in the buffer
 * @tparam Size Number of buffers to maintain (minimum 2, default 2)
 */
template <typename Ty, size_t Size = 2>
class DoublingBuffer
{
    static_assert(Size >= 2, "DoublingBuffer requires at least 2 buffers.");
    static_assert(Size <= 255, "DoublingBuffer requires smaller than 255 buffers.");

private :
    std::atomic<uint32_t> _generation{0};
    std::atomic<uint8_t> _activatedIndex{0};
    std::array<std::shared_ptr<std::vector<Ty>>, Size> _buffers;

public :
    /**
     * @brief Default constructor
     * 
     * Initializes the first buffer with an empty vector.
     */
    DoublingBuffer()
    {
        _buffers[0] = std::make_shared<std::vector<Ty>>();
    }

    /**
     * @brief Adds a new item to the buffer using lock-free double buffering
     * 
     * Creates a copy of the current active buffer with the new item added,
     * then atomically switches to the new buffer. This ensures readers always
     * see a consistent snapshot while avoiding locks.
     * 
     * @param item The item to add (moved into the buffer)
     * 
     * @note This operation has O(n) complexity due to buffer copying.
     *       Consider batching operations for better performance with large datasets.
     */
    auto push(Ty&& item) -> void
    {
        uint8_t currentIndex = _activatedIndex.load();
        uint8_t selectedIndex = (currentIndex + 1) < Size ? (currentIndex + 1) : 0;
        
        _buffers[selectedIndex] = std::make_shared<std::vector<Ty>>(*_buffers[currentIndex]);
        _buffers[selectedIndex]->push_back(std::forward<Ty>(item));
        
        _activatedIndex.store(selectedIndex);
        _generation.fetch_add(1);
    }

    auto push(Ty& item) -> void
    {
        uint8_t currentIndex = _activatedIndex.load();
        uint8_t selectedIndex = (currentIndex + 1) < Size ? (currentIndex + 1) : 0;
        
        _buffers[selectedIndex] = std::make_shared<std::vector<Ty>>(*_buffers[currentIndex]);
        _buffers[selectedIndex]->push_back(item);
        
        _activatedIndex.store(selectedIndex);
        _generation.fetch_add(1);
    }

    /**
     * @brief Removes item at the specified index from the currently active buffer
     * 
     * Creates a copy of the current active buffer with the item at the specified index removed,
     * then atomically switches to the new buffer.
     * 
     * @param index The index of the item to remove from the active buffer
     * @return bool True if item was removed, false if index was invalid or buffer was empty
     * 
     * @note This operation has O(n) complexity due to buffer copying.
     *       Index is validated against the currently active buffer size.
     */
    auto erase(size_t index) -> bool
    {
        uint8_t currentIndex = _activatedIndex.load();
        uint8_t selectedIndex = (currentIndex + 1) < Size ? (currentIndex + 1) : 0;
        
        if (index >= _buffers[currentIndex]->size()) { return false; }
        
        auto buffer = std::make_shared<std::vector<Ty>>();
        buffer->reserve(_buffers[currentIndex]->size() - 1);
        
        for (size_t i = 0; i < _buffers[currentIndex]->size(); ++i) 
        {
            if (i == index) { continue; }
            buffer->push_back((*_buffers[currentIndex])[i]);
        }

        _buffers[selectedIndex] = buffer;
        
        _activatedIndex.store(selectedIndex);
        _generation.fetch_add(1);
        return true;
    }

    /**
     * @brief Gets a shared pointer to the currently active buffer
     * 
     * Returns a snapshot of the current buffer state. The returned shared_ptr
     * ensures the buffer remains valid even if the active buffer changes.
     * 
     * @return std::shared_ptr<std::vector<Ty>> Shared pointer to the active buffer
     * 
     * @note The returned buffer is immutable from the caller's perspective.
     *       Concurrent push() operations will not affect the returned buffer.
     */
    auto get_buffer() -> std::shared_ptr<std::vector<Ty>>
    {
        return _buffers[_activatedIndex.load()];
    }

    /**
     * @brief Gets the current generation number
     * 
     * The generation number is incremented with each buffer modification operation (push/erase),
     * allowing clients to detect when the buffer has been updated.
     * 
     * @return uint32_t Current generation number
     */
    auto get_generation() const -> uint32_t
    {
        return _generation.load();
    }
};
}