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

#if defined(WINDOWS)
#include "common/Singleton.hpp"

#include <mutex>

namespace common::detail
{
struct RegistryEntry
{
    std::once_flag flag;
    std::shared_ptr<void> instance;
};

auto SingletonRegistry::get(std::type_index type,
                            std::function<std::shared_ptr<void>()> factory) -> std::shared_ptr<void>
{
    static std::mutex registry_mutex;
    static std::unordered_map<std::type_index, std::shared_ptr<RegistryEntry>> registry;

    std::shared_ptr<RegistryEntry> entry;
    {
        std::lock_guard<std::mutex> lock(registry_mutex);
        auto it = registry.find(type);
        if(it == registry.end())
        {
            entry = std::make_shared<RegistryEntry>();
            registry[type] = entry;
        }
        else { entry = it->second; }
    }

    std::call_once(entry->flag, [&]() {
        entry->instance = factory();
    });

    return entry->instance;
}
} // namespace common::detail
#endif