#pragma once

#include <boost/property_tree/ptree.hpp>

class SvgDocument
{
public:
    SvgDocument();

    /*
        Add line that starts at (x1, y1) and ends at (x2, y2)
    */
    void AddLine(long double x1, long double y1, long double x2, long double y2);
    void SetSize(long double width, long double height);

    void BuildAndWriteToDisk( const std::string& filename );
private:
    boost::property_tree::ptree tree_;

    std::string FormatValue(long double v);
};