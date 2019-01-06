#include "duration.h"

#include "boost/compute.hpp"

Duration::Duration( const boost::compute::event& event )
    : duration_( event.duration<InternalType>() )
{
}

std::string Duration::Serialize() const
{
    // TODO is it a good choice? Or Utils::SerializeNumber is needed?
    return std::to_string( duration_.count() );
}

double Duration::AsSeconds() const
{
    return std::chrono::duration_cast<std::chrono::duration<double>>( duration_ ).count();
}