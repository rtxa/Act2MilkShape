cmake_minimum_required(VERSION 3.5)

project(act2milkshape
    DESCRIPTION "Converts a Genesis3D Actor file (.act) to MilkShape format (.ms3d)"
    HOMEPAGE_URL "https://github.com/rtxa/act2milkshape"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)

add_executable(act2milkshape main.cpp)

add_subdirectory(lib)

target_link_libraries(act2milkshape PUBLIC GenesisSDK)

set_target_properties(act2milkshape PROPERTIES
    LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
)
