set(target_version 0.1.0)
set(target_name svchostify)
set(package_name SvcHostify)

add_library(${target_name} SHARED)

file(
    GLOB private_sources
    CONFIGURE_DEPENDS
    *.hpp
    *.cpp
    abstract/*.hpp
    workers/*.hpp
    workers/*.cpp
)

target_sources(
    ${target_name}
    PRIVATE
    ${private_sources}
)

target_compile_definitions(
    ${target_name}
    PRIVATE
    IS_SVCHOSTIFY_IMPL=1
    UNICODE=1
)

target_include_directories(
    ${target_name}
    PRIVATE
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(
    ${target_name}
    PRIVATE
    Threads::Threads
    CppEssence::cpp-essence
    CppEssenceJniSupport::cpp-essence-jni-support
    ${JNI_LIBRARIES}
)

set_target_properties(
    ${target_name}
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(
        ${target_name}
        PRIVATE
        atomic
    )
endif()

include(ESPackaging)

es_make_install_package(
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    PACKAGE_NAME ${package_name}
    PATH_VARS CMAKE_INSTALL_PREFIX
)
