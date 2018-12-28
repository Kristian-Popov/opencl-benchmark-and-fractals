#include "duration.h"

#include "boost/compute.hpp"

Duration::Duration( const boost::compute::event& event )
    : duration_( event.duration<InternalType>() )
{
}

std::string Duration::Serialize() const
{
    return std::to_string( duration_.count() );
}