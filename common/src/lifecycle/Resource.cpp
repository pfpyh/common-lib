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

#include "common/lifecycle/Resource.hpp"
#include "common/Exception.hpp"

namespace common::lifecycle
{
auto ResourceManager::track(base::BaseResource* resource) noexcept -> void
{
    std::lock_guard<std::mutex> scopedLock(_lock);
    _resources.insert(resource);
}

auto ResourceManager::release(base::BaseResource* resource) noexcept -> void
{
    std::lock_guard<std::mutex> scopedLock(_lock);
    _resources.erase(resource);
}

auto ResourceManager::audit() -> void
{
    std::lock_guard<std::mutex> scopedLock(_lock);
    if(!_resources.empty())
    {
        throw BadHandlingException("unreleases resource");
    }
}

template <>
Resource<threading::Thread>::~Resource() { release(); }

template <>
auto Resource<threading::Thread>::track(std::shared_ptr<threading::Thread> t) noexcept -> void
{ 
    _resource = t;
    ResourceManager::get_instance()->track(this);
}

template <>
auto Resource<threading::Thread>::get_ptr() noexcept -> std::shared_ptr<threading::Thread>
{
    return _resource.lock();
}
} // namespace common::lifecycle