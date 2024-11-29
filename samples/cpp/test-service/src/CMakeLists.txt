set(target_version 0.1.0)
set(target_name refvalue-test-service)

add_library(${target_name} SHARED)

file(
    GLOB private_sources
    CONFIGURE_DEPENDS
    *.hpp
    *.cpp
)

target_sources(
    ${target_name}
    PRIVATE
    ${private_sources}
)

set_target_properties(
    ${target_name}
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
