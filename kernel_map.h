#pragma once

#include <string>
#include <unordered_map>
#include "boost/compute.hpp"

class KernelMap
{
public:
    KernelMap(
        const std::vector<std::string>& kernelNames, 
        boost::compute::context& context,
        const std::string& source,
        const std::string& compilerOptions )
    {
        for( const std::string& kernelName: kernelNames )
        {
            data_.insert( std::make_pair(
                kernelName,
                Utils::BuildKernel( kernelName, context, source, compilerOptions )
            ) );
        }
    }

    boost::compute::kernel& Get( const std::string& kernelName )
    {
        return data_.at( kernelName );
    }
private:
    std::unordered_map<std::string, boost::compute::kernel> data_;
};