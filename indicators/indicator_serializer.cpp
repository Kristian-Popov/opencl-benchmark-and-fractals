#include "indicators/indicator_serializer.h"

#include "indicators/duration_indicator.h"
#include "indicators/element_processing_time_indicator.h"
#include "indicators/throughput_indicator.h"

#include <typeindex>
#include <typeinfo>

namespace
{
    static const std::unordered_map<std::type_index, std::string> ids = {
        {std::type_index( typeid( DurationIndicator ) ), "DurationIndicator"},
        {std::type_index( typeid( ElementProcessingTimeIndicator ) ), "ElementProcessingTimeIndicator"},
        {std::type_index( typeid( ThroughputIndicator ) ), "ThroughputIndicator"}
    };
}

namespace IndicatorSerializer
{
// TODO move this code to JSON reporter? It's too small and inconvenient to use
std::pair<std::string, nlohmann::json> Serialize( const std::shared_ptr<IndicatorInterface>& p )
{
    std::pair<std::string, nlohmann::json> result;
    {
        auto& iter = ids.find( typeid( *p ) );
        if( iter != ids.cend() )
        {
            result.first = iter->second;
            result.second = p->SerializeValue();
            return result;
        }
    }
    throw std::runtime_error( "Don't know how to serialize indicator" );
}

std::pair<std::string, std::shared_ptr<IndicatorInterface>> Deserialize( const nlohmann::json& ptree )
{
    std::pair<std::string, std::shared_ptr<IndicatorInterface>> result;
    //std::string indicator_id = ptree.get<std::string>(  )
    return result;
}

}
