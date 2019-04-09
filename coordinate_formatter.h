#pragma once

#include <string>

// Formats coordinates to strings with appropriate padding
class CoordinateFormatter
{
public:
    CoordinateFormatter( size_t total_width, size_t total_height );

    std::string Format( size_t coord );
private:
    static int CalcDigits( size_t coord );

    int digits_;
};