#pragma once

#include <stdexcept>

class DataVerificationFailedException: public std::runtime_error
{
public:
    DataVerificationFailedException( const std::string& what )
        : std::runtime_error( what )
    {}

    DataVerificationFailedException( const char* what )
        : std::runtime_error( what )
    {}
};