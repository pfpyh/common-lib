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

#include "common/Singleton.hpp"
#include "common/threading/Thread.hpp"

#include <memory>
#include <unordered_set>

namespace common::lifecycle
{
namespace base
{
/**
 * @brief Base class for all managed resources.
 * 
 * Provides a common interface for resource management.
 */
class BaseResource
{
public :
    virtual ~BaseResource() = default;
};
} // namespace base

/**
 * @brief Singleton manager for tracking and auditing resources.
 * 
 * Manages the lifecycle of all Resource instances, providing tracking and audit capabilities.
 */
class COMMON_LIB_API ResourceManager : public LazySingleton<ResourceManager>
{
private :
    std::mutex _lock;
    std::unordered_set<base::BaseResource*> _resources;

public :
    /**
     * @brief Registers a resource for tracking.
     * 
     * @param resource Pointer to the resource to track.
     */
    auto track(base::BaseResource* resource) noexcept -> void;

    /**
     * @brief Unregisters a resource from tracking.
     * 
     * @param resource Pointer to the resource to release.
     */
    auto release(base::BaseResource* resource) noexcept -> void;

    /**
     * @brief Performs an audit of all tracked resources.
     * 
     * Logs information about currently tracked resources for debugging purposes.
     */
    auto audit() -> void;
};

/**
 * @brief RAII wrapper for managing shared resources with weak ownership.
 * 
 * Provides automatic tracking and lifecycle management for resources.
 * Uses weak_ptr to avoid circular dependencies while maintaining resource tracking.
 * 
 * @note This feature is only active when STRICT_MODE is enabled.
 *       It is called at the very end of application shutdown, so resources must be explicitly
 *       released before that point.
 * 
 * @note Due to performance overhead, this tracking is only enabled in STRICT_MODE.
 *       It is recommended to enable it during development and debugging, and disable it in Release builds.
 * 
 * @note Some common-lib features (e.g., threading::Thread) internally manage Resource tracking.
 * 
 * @tparam ResourceType The type of resource to manage.
 */
template<typename ResourceType>
class Resource : public base::BaseResource
{
private :
    std::weak_ptr<ResourceType> _resource;

public :
    Resource() noexcept = default;
    ~Resource() noexcept { release(); }

public :
    /**
     * @brief Retrieves the managed resource as a shared pointer.
     * 
     * @return Shared pointer to the resource if still valid, nullptr otherwise.
     */
    auto get_ptr() noexcept -> std::shared_ptr<ResourceType>
    { 
        return _resource.lock();
    }

    /**
     * @brief Registers a resource for tracking and management.
     * 
     * @param resource Shared pointer to the resource to track.
     */
    auto track(std::shared_ptr<ResourceType> resource) noexcept -> void
    {
        _resource = resource;
        ResourceManager::get_instance()->track(this);
    }

    /**
     * @brief Releases the resource and unregisters it from tracking.
     * 
     * Called automatically on destruction.
     */
    auto release() noexcept -> void
    {
        ResourceManager::get_instance()->release(this);
        _resource.reset();
    }
};

extern template Resource<threading::Thread>::Resource() noexcept;
} // namespace common::lifecycle