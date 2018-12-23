#include "indicators/indicator_serializer.h"

#include "indicators/duration_indicator.h"
#include "indicators/indicator_interface.h"

namespace
{
    const char* const kDurationIndicatorId = "DurationIndicator";
}

namespace IndicatorSerializer
{

std::pair<std::string, boost::property_tree::ptree> Serialize( const std::shared_ptr<IndicatorInterface>& p )
{
    std::pair<std::string, boost::property_tree::ptree> result;
    {
        auto& ptr = std::dynamic_pointer_cast<DurationIndicator>( p );
        if( ptr )
        {
            result.first = kDurationIndicatorId;
            result.second = ptr->SerializeValue();
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
