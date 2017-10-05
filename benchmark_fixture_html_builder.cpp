#include "benchmark_fixture_html_builder.h"

#include "html_document.h"

HTMLDocument BenchmarkFixtureHTMLBuilder::Build( const std::vector<BenchmarkFixtureResultForPlatform>& results, const char* fileName )
{
    HTMLDocument document(fileName);
    std::vector<std::vector<HTMLDocument::CellDescription>> rows;
    typedef HTMLDocument::CellDescription CellDescription;
    rows.push_back( { CellDescription("Platform name", true, 2), CellDescription( "Device name", true, 2 ), CellDescription( "Result, us", true ) } );
    document.AddTable(rows);
    return document;
}