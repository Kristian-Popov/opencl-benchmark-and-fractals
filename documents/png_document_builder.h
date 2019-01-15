#pragma once

#include <vector>

#include "lodepng/source/lodepng.h"
#include "boost/compute.hpp"

class PNGDocumentBuilder
{
public:
    static void BuildRGBA64Bit( const std::string& fileName, const std::vector<cl_ushort4>& data,
        size_t width, size_t height )
    {
        const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>( data.data() );
        Build(fileName, dataPtr, width, height, LCT_RGBA, 16);
    }

    static void BuildGrayscale16Bit( const std::string& fileName, const std::vector<cl_ushort>& data,
        size_t width, size_t height )
    {
        const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>( data.data() );
        Build( fileName, dataPtr, width, height, LCT_GREY, 16 );
    }
private:
    static void Build( const std::string& fileName, const unsigned char* dataPtr,
        size_t width, size_t height, LodePNGColorType colorType, unsigned bitdepth )
    {
        unsigned error = lodepng::encode( fileName, dataPtr, width, height, colorType, bitdepth );
        if( error )
        {
            throw std::runtime_error( "PNG image build failed" );
        }
    }
};