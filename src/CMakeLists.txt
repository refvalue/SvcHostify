set(target_version 0.2.0)
set(target_name svchostify)
set(package_name SvcHostify)

add_library(${target_name} SHARED)

file(
    GLOB_RECURSE miu_sources
    CONFIGURE_DEPENDS
    *.ixx
    *.cxx
)

file(
    GLOB_RECURSE private_sources
    CONFIGURE_DEPENDS
    *.cpp
)

target_sources(
    ${target_name}
    PUBLIC
    FILE_SET CXX_MODULES
    FILES ${miu_sources}
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
    CppEssence::cpp-essence-jni-support
    ${JNI_LIBRARIES}
)

target_compile_features(
    ${target_name}
    PRIVATE
    cxx_std_23
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
    TARGETS ${target_name}
    VERSION ${target_version}
    PACKAGE_NAME ${package_name}
    PATH_VARS CMAKE_INSTALL_PREFIX
)
