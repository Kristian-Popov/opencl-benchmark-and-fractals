#include "svg_document.h"

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/trivial.hpp>

const long double SVGDocument::multiplier_ = 1000.0;

SVGDocument::SVGDocument()
{
    tree_.put( "svg.<xmlattr>.xmlns", "http://www.w3.org/2000/svg" );
    tree_.put( "svg.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink" );
    tree_.put( "svg.<xmlattr>.width", multiplier_ );
    tree_.put( "svg.<xmlattr>.height", multiplier_ );
}

void SVGDocument::AddLine( long double x1, long double y1, long double x2, long double y2 )
{
    boost::property_tree::ptree node;
    node.put( "<xmlattr>.x1", FormatValue( x1 ) );
    node.put( "<xmlattr>.y1", FormatValue( y1 ) );
    node.put( "<xmlattr>.x2", FormatValue( x2 ) );
    node.put( "<xmlattr>.y2", FormatValue( y2 ) );
    node.put( "<xmlattr>.style", "stroke: black" );
    tree_.add_child( "svg.line", node );
}

void SVGDocument::BuildAndWriteToDisk( const std::string& filename )
{
    try
    {
        boost::property_tree::write_xml( filename, tree_ );
    }
    catch(std::exception& e)
    {
        BOOST_LOG_TRIVIAL( error ) << "Caught exception when building SVG document " << filename << ": " << e.what();
        throw;
    }
}

std::string SVGDocument::FormatValue( long double v )
{
    return std::to_string( v * multiplier_ );
}