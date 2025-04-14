#include "hal/gnss/NeoM6.hpp"

#include <sstream>

namespace common::hal
{
auto string_split(const std::string& message) -> std::vector<std::string>
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(message);
    while(std::getline(iss, token, ','))
    {
        tokens.push_back(token);
    }
    return tokens;
}

auto to_decimal_degree(const std::string& degreesMinutes, const std::string& direction) -> double
{
    if (degreesMinutes.empty()) { return 0.0; }

    auto dotPos = degreesMinutes.find('.');
    int32_t degrees = std::stoi(degreesMinutes.substr(0, dotPos - 2));
    double minutes = std::stod(degreesMinutes.substr(dotPos - 2));
    double decimalDegrees = degrees + (minutes / 60.0);

    if (direction == "S" || direction == "W") {
        decimalDegrees *= -1;
    }
    return decimalDegrees;
}

auto GPGGA(const std::vector<std::string>& tokens, PositionData& data) -> bool
{
    if (tokens.size() > 5 && tokens[0] == "$GPGGA")
    {
        data._utc = std::stod(tokens[1]);
        data._latitude = to_decimal_degree(tokens[2], tokens[3]);
        data._longitude = to_decimal_degree(tokens[4], tokens[5]);
        return true;
    }
    return false;
}

auto GPRMC(const std::vector<std::string>& tokens, PositionData& data) -> bool
{
    return false;
}

auto GPVTG(const std::vector<std::string>& tokens, PositionData& data) -> bool
{
    return false;
}

auto NeoM6::parse(const std::string& message) noexcept -> PositionData
{
    const std::vector<std::string> tokens(string_split(message));

    PositionData data;

    if(GPGGA(tokens, data))
    {
        data._type = (data._type | SupportType::GPGGA);
    }

    if(GPRMC(tokens, data))
    {
        data._type = (data._type | SupportType::GPRMC);
    }

    if(GPVTG(tokens, data))
    {
        data._type = (data._type | SupportType::GPVTG);
    }

    return data;
}
} // namespace common::hal