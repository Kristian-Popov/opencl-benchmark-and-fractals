#pragma once

#include <string>
#include <memory>

#include "devices/device_interface.h"

class FixtureId
{
public:
	// TODO process family name at all?
    FixtureId( const std::string& family_name, const std::shared_ptr<DeviceInterface>& device, const std::string& algorithm )
        : family_name_( family_name )
        , device_( device )
        , algorithm_( algorithm )
    {
    }

    bool operator<( const FixtureId& rhs ) const
    {
        // First sort by family name
        // TODO move this generic comparison logic to a separate function
        if( family_name_ < rhs.family_name_ )
        {
            return true;
        }
        else if ( rhs.family_name_ < family_name_ )
        {
            return false;
        }

        if( device_ < rhs.device_ )
        {
            return true;
        }
        else if( rhs.device_ < device_ )
        {
            return false;
        }

        if( algorithm_ < rhs.algorithm_ )
        {
            return true;
        }
        return false;
    }

    bool operator==( const FixtureId& rhs ) const
    {
        return family_name_ == rhs.family_name_ &&
            device_ == rhs.device_ &&
            algorithm_ == rhs.algorithm_;
    }

    bool operator!=( const FixtureId& rhs ) const
    {
        return !( *this == rhs );
    }

    std::string family_name() const
    {
        return family_name_;
    }

    std::shared_ptr<DeviceInterface> device() const
    {
        return device_;
    }

    std::string algorithm() const
    {
        return algorithm_;
    }

    std::string Serialize() const
    {
		std::string result = device_->UniqueName();
		if( !algorithm_.empty() )
		{
			result += ", " + algorithm_;
		}
		return result;
	}

private:
    std::string family_name_;
    std::shared_ptr<DeviceInterface> device_;
    std::string algorithm_;
};

namespace std
{
    template<>
    struct hash<FixtureId>
    {
        size_t operator()( const FixtureId& f ) const
        {
            // Based on https://stackoverflow.com/a/27952689
            // TODO is this function any good?
            size_t result = 0;
            size_t family_name_hash = hash<std::string>()( f.family_name() );
            size_t device_hash = hash<std::shared_ptr<DeviceInterface>>()( f.device() );
            size_t algorithm_hash = hash<std::string>()( f.algorithm() );
            return ( family_name_hash << 2 ) + ( family_name_hash << 1 ) + ( device_hash << 1 ) + device_hash +
                algorithm_hash;
        }
    };
}

