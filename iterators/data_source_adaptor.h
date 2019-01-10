#pragma once

#include <memory>
#include <stdexcept>

#include "iterators/data_source.h"

/*
    Adopts data source to STL iterator so it can be used in STL algorithms
*/
template<typename T>
class DataSourceAdaptor: public std::iterator<std::input_iterator_tag, T>
{
public:
    explicit DataSourceAdaptor( std::unique_ptr<DataSource<T>>&& data_source )
        : data_source_( data_source )
    {
        if( !data_source )
        {
            throw std::invalid_argument( "DataSourceAdaptor cannot be constructed from an empty data source pointer." );
        }
    }

    explicit DataSourceAdaptor( const std::shared_ptr<DataSource<T>>& data_source )
        : data_source_( data_source )
    {
        if( !data_source )
        {
            throw std::invalid_argument( "DataSourceAdaptor cannot be constructed from an empty data source pointer." );
        }
    }

    bool operator==( const DataSourceAdaptor<T>& rhs )
    {
        return data_source_ == rhs.data_source_;
    }

    bool operator!=( const DataSourceAdaptor<T>& rhs )
    {
        return !( *this == rhs );
    }

    T operator*()
    {
        return data_source_->Get();
    }

    DataSourceAdaptor<T>& operator++()
    {
        data_source_->Increment();
        return *this;
    }

    const DataSourceAdaptor<T> operator++( int )
    {
        DataSourceAdaptor<T> temp = *this;
        ++( *this );
        return temp;
    }
private:
    std::shared_ptr<DataSource<T>> data_source_;
};