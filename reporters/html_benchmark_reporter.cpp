#include "html_benchmark_reporter.h"

#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <iterator>

#include "html_document.h"
#include "utils.h"
#include "devices/device_interface.h"
#include "devices/platform_interface.h"
#include "fixtures/fixture_id.h"
#include "indicators/duration_indicator.h"

const std::unordered_map<long double, std::string> HtmlBenchmarkReporter::avgDurationsUnits =
{
    { 1e-9, "nanoseconds" },
    { 1e-6, "microseconds" },
    { 1e-3, "milliseconds" },
    { 1, "seconds" },
    { 60, "minutes" },
    { 3600, "hours" }
};

const std::unordered_map<long double, std::string> HtmlBenchmarkReporter::durationPerElementUnits =
{
    {1e-9, "nanoseconds/elem."},
    {1e-6, "microseconds/elem."},
    {1e-3, "milliseconds/elem."},
    {1, "seconds/elem."},
    {60, "minutes/elem."},
    {3600, "hours/elem."}
};

const std::unordered_map<long double, std::string> HtmlBenchmarkReporter::elementsPerSecUnits =
{
    { 1, "elem./second" },
    { 1e+3, "thousands elem./second"},
    { 1e+6, "millions elem./second"},
    { 1e+9, "billions elem./second"}
};

HtmlBenchmarkReporter::HtmlBenchmarkReporter( const std::string& file_name )
    : document_( std::make_shared<HtmlDocument>( file_name ) )
{}

void HtmlBenchmarkReporter::AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results )
{
    document_->AddHeader( results.fixture_family->name );

    DurationIndicator duration_indicator( results );

    std::vector<std::vector<HtmlDocument::CellDescription>> rows = {
        { // First row
            HtmlDocument::CellDescription { "Platform name", true, 2 },
            HtmlDocument::CellDescription { "Device name", true, 2 },
            HtmlDocument::CellDescription{ "Algorithm", true, 2 },
        },
        {} // Second row
    };

    const std::vector<OperationStep>& steps = results.fixture_family->operation_steps;
    int step_count = static_cast<int>( steps.size() ); // TODO make sure it fits
    if( step_count > 1 )
    {
        step_count++; // Add a separate column for total duration
    }
    rows.at( 0 ).push_back( HtmlDocument::CellDescription { "Duration, seconds", true, 1, step_count } ); // TODO select unit
    for( OperationStep step: steps )
    {
        rows.at( 1 ).push_back( OperationStepDescriptionRepository::Get( step ) );
    }
    if( step_count > 1 )
    {
        rows.at( 1 ).push_back( HtmlDocument::CellDescription{ "Total duration" } );
    }

    std::unordered_map<FixtureId, DurationIndicator::FixtureCalculatedData> data = duration_indicator.GetCalculatedData();

    std::vector<FixtureId> ids;
    std::transform( data.cbegin(), data.cend(), std::back_inserter( ids ),
        [] ( const auto& p ) { return p.first; }
    );
    std::sort( ids.begin(), ids.end(), [] ( const FixtureId& lhs, const FixtureId& rhs ) {
        {
            std::string lhs_platform_name = std::shared_ptr<PlatformInterface>( lhs.device()->platform() )->Name();
            std::string rhs_platform_name = std::shared_ptr<PlatformInterface>( rhs.device()->platform() )->Name();
            if( lhs_platform_name < rhs_platform_name )
            {
                return true;
            }
            else if( rhs_platform_name < lhs_platform_name )
            {
                return false;
            }
        }
        {
            const std::string& lhs_device_name = lhs.device()->Name();
            const std::string& rhs_device_name = rhs.device()->Name();
            if( lhs_device_name < rhs_device_name )
            {
                return true;
            }
            else if( rhs_device_name < lhs_device_name )
            {
                return false;
            }
        }
        {
            if( lhs.algorithm() < rhs.algorithm() )
            {
                return true;
            }
        }
        return false;
    } );
    for( FixtureId& id: ids )
    {
        const DurationIndicator::FixtureCalculatedData& measurements = data.at( id );
        std::shared_ptr<PlatformInterface> platform{ id.device()->platform() };
        EXCEPTION_ASSERT( platform->GetDevices().size() < std::numeric_limits<int>::max() );
        int device_count = static_cast<int>( platform->GetDevices().size() );

        std::vector<HtmlDocument::CellDescription> row = {
            HtmlDocument::CellDescription{ platform->Name() },
            HtmlDocument::CellDescription{ id.device()->Name() },
            HtmlDocument::CellDescription{ id.algorithm() }
        };

        if( measurements.failure_reason )
        {
            row.push_back( HtmlDocument::CellDescription{ measurements.failure_reason.value(), false, 1, step_count } );
        }
        else
        {
            for( OperationStep step : steps )
            {
                double val{ 0.0 };
                auto iter = measurements.step_durations.find( step );
                if( iter != measurements.step_durations.end() )
                {
                    val = iter->second.AsSeconds();
                }

                row.push_back( HtmlDocument::CellDescription { Utils::SerializeNumber( val ) } );
            }

            if( step_count > 1 )
            {
                row.push_back( HtmlDocument::CellDescription { Utils::SerializeNumber( measurements.total_duration.AsSeconds() ) } );
            }
        }

        rows.push_back( row );
    }

    document_->AddTable( rows );
}

void HtmlBenchmarkReporter::Flush()
{
    document_->BuildAndWriteToDisk();
}
