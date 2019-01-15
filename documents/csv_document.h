#pragma once

#include <fstream>
#include <vector>

class CSVDocument
{
public:
    CSVDocument( const char* file_name )
        : stream_( file_name )
    {}

    CSVDocument( const std::string& file_name )
        : stream_( file_name )
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

