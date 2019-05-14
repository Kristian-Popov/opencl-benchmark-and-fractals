#include "svg_document.h"

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/trivial.hpp>

SvgDocument::SvgDocument()
{
    tree_.put( "svg.<xmlattr>.xmlns", "http://www.w3.org/2000/svg" );
    tree_.put( "svg.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink" );
    tree_.put( "svg.<xmlattr>.stroke", "black" );
}

void SvgDocument::SetSize( long double width, long double height )
{
    tree_.put( "svg.<xmlattr>.width", width );
    tree_.put( "svg.<xmlattr>.height", height );
}

void SvgDocument::AddLine( long double x1, long double y1, long double x2, long double y2 )
{
    boost::property_tree::ptree node;
    node.put( "<xmlattr>.x1", FormatValue( x1 ) );
    node.put( "<xmlattr>.y1", FormatValue( y1 ) );
    node.put( "<xmlattr>.x2", FormatValue( x2 ) );
    node.put( "<xmlattr>.y2", FormatValue( y2 ) );
    tree_.add_child( "svg.line", node );
}

void SvgDocument::BuildAndWriteToDisk( const std::string& filename )
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

std::string SvgDocument::FormatValue( long double v )
{
    return std::to_string( v );
}