#pragma once

#include "nlohmann/json.hpp"

class IndicatorInterface;

namespace IndicatorSerializer
{
    std::pair<std::string, nlohmann::json> Serialize( const std::shared_ptr<IndicatorInterface>& p );
    std::pair<std::string, std::shared_ptr<IndicatorInterface>> Deserialize( const nlohmann::json& ptree );
}
