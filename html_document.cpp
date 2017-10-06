#include "html_document.h"
#include <utils.h>

// TODO this class would benefit from HTML template library

const char* HTMLDocument::intro_ = R"(
<!DOCTYPE html>
<html>
<head>
<title>OpenCL benchmark results</title>
<meta charset="UTF-8">
<style>
table, th, td {
    border: 1px solid black;
}
</style>
</head>
<body>
)";

const char* HTMLDocument::outro_ = R"(
</body>
</html>
)";

HTMLDocument::HTMLDocument( const char* fileName )
    : stream_(fileName)
{
    stream_ << intro_;
}

void HTMLDocument::AddTable( const std::vector<std::vector<CellDescription>>& rows )
{
    stream_ << "<table>";
    for (const std::vector<CellDescription>& row: rows)
    {
        stream_ << "<tr>";
        for (const CellDescription& cell: row)
        {
            const char* tag = cell.header ? "th" : "td";
            stream_ << "<" << tag;

            EXCEPTION_ASSERT( cell.rowSpan >= 1 );
            if (cell.rowSpan > 1)
            {
                stream_ << " rowspan=\"" << cell.rowSpan << "\" ";
            }

            EXCEPTION_ASSERT( cell.colSpan >= 1 );
            if( cell.colSpan > 1 )
            {
                stream_ << " colspan=\"" << cell.colSpan << "\" ";
            }

            stream_<< ">";
            stream_ << cell.text;
            stream_ << "</" << tag << ">";
        }
        stream_ << "</tr>" << std::endl;
    }
    stream_ << "</table>" << std::endl;
}

void HTMLDocument::AddHeader( const std::string& text, int level /* = 1 */ )
{
    EXCEPTION_ASSERT( level >= 1 && level <= 6 );
    stream_ << "<h" << level << ">" << text << "</h" << level << ">" << std::endl;
}

void HTMLDocument::BuildAndWriteToDisk()
{
    stream_ << outro_;
    stream_.flush();
}
