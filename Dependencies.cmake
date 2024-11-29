include_guard()

find_package(Threads REQUIRED)
find_package(CppEssence REQUIRED HINTS ${ES_CPP_ESSENCE_ROOT})
find_package(CppEssenceJniSupport REQUIRED HINTS ${ES_CPP_ESSENCE_ROOT})
find_package(JNI REQUIRED)
