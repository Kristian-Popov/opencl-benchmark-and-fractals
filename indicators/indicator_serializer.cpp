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

std::pair<std::string, boost::property_tree::ptree> Serialize( const std::shared_ptr<IndicatorInterface>& p )
{
    std::pair<std::string, boost::property_tree::ptree> result;
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

std::pair<std::string, std::shared_ptr<IndicatorInterface>> Deserialize( const boost::property_tree::ptree& ptree )
{
    std::pair<std::string, std::shared_ptr<IndicatorInterface>> result;
    //std::string indicator_id = ptree.get<std::string>(  )
    return result;
}

}
