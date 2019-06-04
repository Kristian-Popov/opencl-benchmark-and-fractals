project( RunOnceDebug )

add_executable( RunOnceDebug
    run_once_debug_main.cpp
    ../fixture_factories.cpp
)

target_link_libraries( RunOnceDebug commonlib )
