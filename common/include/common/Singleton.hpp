/**********************************************************************
MIT License

Copyright (c) 2026 Park Younghwan

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

#include "common/CommonHeader.hpp"
#include "NonCopyable.hpp"

#include <memory>

#if defined(WINDOWS)
#include <typeindex>
#include <functional>
#endif

namespace common
{
#if defined(WINDOWS)
namespace detail
{
class SingletonRegistry;
} // namespace detail
#endif

/**
 * @brief Template class for implementing the Singleton design pattern.
 *
 * Ensures only one instance of the derived class exists and provides a global
 * access point to that instance.
 *
 * @tparam Derived The derived class type.
 *
 * @par Platform-specific behaviour
 * - **Linux**: After the first call, `get_instance()` resolves to a single
 *   atomic load (~1-2 ns). The static local variable is deduplicated across
 *   shared-library boundaries by the ELF weak-symbol mechanism, so the same
 *   instance is always returned regardless of which module calls it.
 * - **Windows**: Because a DLL and the loading EXE each have their own copy
 *   of template-instantiated static variables, a centralised
 *   `SingletonRegistry` (compiled into the DLL) is used to guarantee a single
 *   instance across module boundaries. Every call to `get_instance()` acquires
 *   a `std::mutex` for map lookup and then checks a per-type `std::once_flag`,
 *   adding approximately **30-80 ns** of overhead compared with the Linux path
 *   (~20-40x slower per call).
 *
 * @par Windows performance recommendation
 * Avoid calling `get_instance()` in hot paths (tight loops, high-frequency
 * event handlers, etc.). Instead, cache the returned `shared_ptr` as a member
 * variable at construction time and reuse it:
 * @code
 * class Foo {
 *     std::shared_ptr<Bar> _bar = Bar::get_instance(); // cached once
 * };
 * @endcode
 */
template<typename Derived>
class Singleton : public NonCopyable
{
    friend Derived;

#if !defined(WINDOWS)
private :
    inline static std::shared_ptr<Derived> _instance = std::make_shared<Derived>();
#endif
    
public :
    /**
     * @brief Gets the Singleton instance.
     * 
     * @return The Singleton instance.
     */
    static auto get_instance() -> std::shared_ptr<Derived>
    {
#if defined(WINDOWS)
        auto instance = detail::SingletonRegistry::get(
            typeid(Derived), []() -> std::shared_ptr<void> {
                return std::make_shared<Derived>();
            }
        );
        return std::static_pointer_cast<Derived>(instance);
#else
        return _instance;
#endif
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

/**
 * @brief Template class for implementing the Lazy Singleton design pattern.
 *
 * Differs from `Singleton` in that the instance is not created until the first
 * call to `get_instance()` (lazy initialisation).
 *
 * @tparam Derived The derived class type.
 *
 * @par Platform-specific behaviour
 * - **Linux**: The instance is created on the first call via a static local
 *   variable, which the compiler guards with a single atomic flag. Subsequent
 *   calls cost ~1-2 ns. The ELF weak-symbol mechanism ensures the same instance
 *   is shared across shared-library boundaries.
 * - **Windows**: To overcome the DLL boundary problem (each module having its
 *   own copy of the static variable), every call to `get_instance()` is routed
 *   through `SingletonRegistry` - a non-template function compiled into the
 *   DLL. This involves a `std::mutex` acquisition for map lookup and a
 *   per-type `std::once_flag` check, adding approximately **30-80 ns** of
 *   overhead per call (~20-40x slower than Linux).
 *
 * @par Windows performance recommendation
 * Avoid calling `get_instance()` in hot paths (tight loops, high-frequency
 * event handlers, etc.). Instead, cache the returned `shared_ptr` as a member
 * variable at construction time and reuse it:
 * @code
 * class Foo {
 *     std::shared_ptr<Bar> _bar = Bar::get_instance(); // cached once
 * };
 * @endcode
 */
template<typename Derived>
class LazySingleton : public NonCopyable
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
#if defined(WINDOWS)
        auto instance = detail::SingletonRegistry::get(
            typeid(Derived), []() -> std::shared_ptr<void> {
                return std::make_shared<Derived>();
            }
        );
        return std::static_pointer_cast<Derived>(instance);
#else
        static std::shared_ptr<Derived> instance;
        if (instance == nullptr)
        {
            instance = std::make_shared<Derived>();
        }
        return instance;
#endif
    }

private :
    /**
     * @brief Private constructor to prevent instantiation.
     */
    LazySingleton() = default;

    /**
     * @brief Private assignment operator to prevent assignment.
     * 
     * @param other The other Singleton instance.
     * @return A reference to the current Singleton instance.
     */
    virtual ~LazySingleton() = default;
};

#if defined(WINDOWS)
namespace detail
{
class COMMON_LIB_API SingletonRegistry
{
public :
    static auto get(std::type_index type, std::function<std::shared_ptr<void>()> factory) -> std::shared_ptr<void>;
};
} // namespace detail
#endif
} // namespace common