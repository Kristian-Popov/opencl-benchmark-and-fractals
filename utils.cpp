#include "utils.h"

#include "fixture.h"

namespace Utils
{
    std::string ReadFile( const std::string& fileName )
    {
        std::ifstream stream( fileName );
        std::stringbuf buf;
        if( !stream.is_open() )
        {
            throw std::runtime_error( "File not found" );
        }
        stream >> &buf;
        stream.close();
        return buf.str();
    }

    std::string FormatQuantityString( int value )
    {
        std::string result;
        if (value % 1000000 == 0)
        {
            result = std::to_string(value / 1000000) + "M";
        }
        else if( value % 1000 == 0 )
        {
            result = std::to_string( value / 1000 ) + "K";
        }
        else
        {
            result = std::to_string( value);
        }
        return result;
    }
}
