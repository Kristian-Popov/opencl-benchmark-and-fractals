#pragma once

#include <boost/property_tree/ptree.hpp>

class SVGDocument
{
public:
    SVGDocument();

    void AddLine(long double x1, long double y1, long double x2, long double y2);

    void BuildAndWriteToDisk( const std::string& filename );
private:
    static const long double multiplier_;
    boost::property_tree::ptree tree_;

    std::string FormatValue(long double v);
};