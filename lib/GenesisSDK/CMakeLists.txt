add_library(GenesisSDK INTERFACE)

target_link_libraries(GenesisSDK INTERFACE
    general ${CMAKE_CURRENT_SOURCE_DIR}/lib/genesis.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/lib/genesisd.lib
    legacy_stdio_definitions.lib 
) # Keeps compatibility with GENESIS.LIB compiled with old C compiler (_NO_CRT_STDIO_INLINE)

target_include_directories(GenesisSDK INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)