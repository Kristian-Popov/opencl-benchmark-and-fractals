#pragma once

#include <vector>

class CSVDocument
{
public:
    CSVDocument( const char* fileName )
        : stream_(fileName)
    {}

    template<typename T>
    void AddValue( T value )
    {
        stream_ << value << ";";
    }

    template<>
    void AddValue<char*>( char* value )
    {
        stream_ << "\"" << value << "\";";
    }

    void FinishRow()
    {
        stream_ << std::endl;
    }

    template<typename T>
    void AddValues( const std::vector<std::vector<T>>& values )
    {
        for (const auto& row: values )
        {
            for( T value : row )
            {
                AddValue( value );
            }
            FinishRow();
        }
    }

    void BuildAndWriteToDisk()
    {
        stream_.flush();
    }

private:
    std::ofstream stream_;
};

