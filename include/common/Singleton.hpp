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

#include "NonCopyable.hpp"

namespace common
{
/**
 * @brief Template class for implementing the Singleton design pattern.
 * 
 * This class provides a way to implement the Singleton design pattern, which ensures that only one instance of a class is created, and provides a global point of access to that instance.
 * 
 * @tparam T The type of the Singleton instance.
 */
template<typename Derived>
class Singleton : public NonCopyable
{
    friend Derived;
    
public :
    /**
     * @brief Gets the Singleton instance.
     * 
     * @return The Singleton instance.
     */
    static auto get_instance() -> std::shared_ptr<Derived>
    {
        static std::shared_ptr<Derived> instance;
        if (instance == nullptr)
        {
            instance = std::make_shared<Derived>();
        }
        return instance;
    }

private :
    /**
     * @brief Private constructor to prevent instantiation.
     */
    Singleton() = default;

    /**
     * @brief Private assignment operator to prevent assignment.
     * 
     * @param other The other Singleton instance.
     * @return A reference to the current Singleton instance.
     */
    virtual ~Singleton() = default;
};
} // namespace common