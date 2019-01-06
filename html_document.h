#pragma once

#include <sstream>
#include <vector>
#include <string>

class HtmlDocument
{
public:
    struct CellDescription
    {
        std::string text;
        bool header = false;
        int rowSpan = 1;
        int colSpan = 1;

        CellDescription( const std::string& text_, bool header_ = false, int rowSpan_ = 1, int colSpan_ = 1 )
            : text(text_)
            , rowSpan(rowSpan_)
            , colSpan( colSpan_ )
            , header(header_)
        {}
    };

    HtmlDocument( const std::string& file_name );

    void AddHeader( const std::string& text, int level = 1 );
    void AddTable( const std::vector<std::vector<CellDescription>>& rows );

    void BuildAndWriteToDisk();

private:
    std::ostringstream stream_;
    std::string file_name_;
    static const char* intro_;
    static const char* outro_;
};

