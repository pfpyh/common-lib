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
template <typename Derived>
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
    static auto create(Args... args) noexcept -> std::unique_ptr<Derived>
    {
        return Derived::__create(std::forward<Args>(args)...);
    }

protected :
    UniqueFactory() noexcept = default;

public :
    virtual ~UniqueFactory() noexcept = default;
};
} // namespace common