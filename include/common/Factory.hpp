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

#include "CommonHeader.hpp"

#include <memory>

namespace common
{
/**
 * @brief Factory class for creating objects of a specific type.
 * 
 * This class provides a way to create objects of a specific type using a factory method.
 * The create function takes a variable number of arguments and returns a shared pointer to an object of the derived type.
 * 
 * @tparam Derived The type of object to be created.
 * @tparam Args The types of arguments to be passed to the create function.
 */
template <typename Derived>
class Factory
{
public :
    /**
     * @brief Creates an object of the derived type using the provided arguments.
     * 
     * @param args The arguments to be passed to the create function.
     * @return A shared pointer to an object of the derived type.
     */
    template <typename... Args>
    static auto create(Args... args) noexcept -> std::shared_ptr<Derived>
    {
        return Derived::__create(std::forward<Args>(args)...);
    }

protected :
    Factory() noexcept = default;

public :
    virtual ~Factory() noexcept = default;
};

/**
 * @brief Factory class for creating objects of a specific type.
 * 
 * This class provides a way to create objects of a specific type using a factory method.
 * The create function takes a variable number of arguments and returns a shared pointer to an object of the derived type.
 * 
 * @tparam Derived The type of object to be created.
 * @tparam Args The types of arguments to be passed to the create function.
 */
template <typename Derived, typename Deleter = std::default_delete<Derived>>
class UniqueFactory
{
public :
    /**
     * @brief Creates an object of the derived type using the provided arguments.
     * 
     * @param args The arguments to be passed to the create function.
     * @return A unique pointer to an object of the derived type.
     */
    template <typename... Args>
    static auto create(Args... args) noexcept -> std::unique_ptr<Derived, Deleter>
    {
        return Derived::__create(std::forward<Args>(args)...);
    }

protected :
    UniqueFactory() noexcept = default;

public :
    virtual ~UniqueFactory() noexcept = default;
};
} // namespace common