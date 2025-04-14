#include "common/logging/Logger.hpp"

#include <iostream>

namespace common
{
auto Logger::debug(const std::string& log) -> void
{
    std::cout << "[DEBUG] " << log << std::endl;
}

auto Logger::error(const std::string& log) -> void
{
    std::cout << "[ERROR] " << log << std::endl;
}

auto Logger::info(const std::string& log) -> void
{
    std::cout << "[INFO] " << log << std::endl;
}
} // namespace common