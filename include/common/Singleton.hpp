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
class COMMON_LIB_API Singleton : public NonCopyable
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