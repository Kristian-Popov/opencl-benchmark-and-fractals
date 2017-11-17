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
        unsigned error = lodepng::encode( fileName, dataPtr, width, height, LCT_RGBA, 16 );
        if( error )
        {
            throw std::runtime_error("PNG image build failed");
        }
    }
};