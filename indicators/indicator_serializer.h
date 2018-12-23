#pragma once

#include <boost/property_tree/ptree.hpp>

class IndicatorInterface;

namespace IndicatorSerializer
{
    std::pair<std::string, boost::property_tree::ptree> Serialize( const std::shared_ptr<IndicatorInterface>& p );
    std::pair<std::string, std::shared_ptr<IndicatorInterface>> Deserialize( const boost::property_tree::ptree& ptree );
}
