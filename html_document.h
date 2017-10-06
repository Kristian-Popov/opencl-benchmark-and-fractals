#pragma once

#include <fstream>
#include <vector>
#include <string>

class HTMLDocument
{
public:
    struct CellDescription
    {
        std::string text;
        int rowSpan = 1;
        int colSpan = 1;
        bool header = false;

        CellDescription( const std::string& text_, bool header_ = false, int rowSpan_ = 1, int colSpan_ = 1 )
            : text(text_)
            , rowSpan(rowSpan_)
            , colSpan( colSpan_ )
            , header(header_)
        {}
    };

    HTMLDocument( const char* fileName );

    void AddHeader( const std::string& text, int level = 1 );
    void AddTable( const std::vector<std::vector<CellDescription>>& rows );

    void BuildAndWriteToDisk();

private:
    std::ofstream stream_;
    static const char* intro_;
    static const char* outro_;
};

