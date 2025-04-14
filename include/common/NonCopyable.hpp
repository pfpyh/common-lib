#pragma once

#include "CommonHeader.hpp"

namespace common
{
/**
 * @brief Base class that prevents objects of derived classes from being copied or assigned.
 * 
 * This class is a base class that prevents objects of derived classes from being copied or assigned.
 * It does this by deleting the copy constructor and copy assignment operator.
 */
class COMMON_LIB_API NonCopyable
{
public :
    /**
     * @brief Default constructor.
     */
    NonCopyable() = default;

    /**
     * @brief Default destructor.
     */
    virtual ~NonCopyable() = default;

    /**
     * @brief Deleted copy constructor.
     * 
     * @param other The object to be copied.
     */
    NonCopyable(const NonCopyable&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * 
     * @param other The object to be assigned.
     * @return A reference to the current object.
     */
    NonCopyable& operator=(const NonCopyable&) = delete;
};
} // namespace common