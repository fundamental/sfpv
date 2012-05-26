cmake_minimum_required (VERSION 2.8)
project (sfpv)

set (sfpv_VERSION_MAJOR 0)
set (sfpv_VERSION_MINOR 1)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
#LLVM
find_package(LLVM 3.1 REQUIRED)

include_directories (${LLVM_INCLUDE_DIRS})
link_directories (${LLVM_LIBRARY_DIRS})
add_definitions (${LLVM_DEFINITIONS})
add_definitions (-DCLANG_LOCATION="${LLVM_INSTALL_PREFIX}/bin/clang")

#Clang
set (CLANG_LIBRARIES clangFrontend clangDriver clangSerialization  clangParse
    clangSema  clangAnalysis  clangRewrite  clangEdit  clangAST  clangLex
    clangBasic)
add_definitions (-fno-rtti)
set (CMAKE_CXX_COMPILER clang++)

add_definitions (-DVERSION="${sfpv_VERSION_MAJOR}.${sfpv_VERSION_MINOR}")
add_executable (sfpv src/Errors.cpp src/GraphBuilder.cpp
    src/TranslationUnit.cpp src/Types.cpp src/main.cpp)
target_link_libraries (sfpv  ${CLANG_LIBRARIES} ${LLVM_LIBRARIES})

######################################################################
#               Tests to verify it works                             #
######################################################################
set(SRC ${CMAKE_CURRENT_SOURCE_DIR})
enable_testing ()

#Verify one file basic behavior
add_test (test-one sfpv ${SRC}/test/test-input.c)
set_property (TEST test-one
    PROPERTY PASS_REGULAR_EXPRESSION
    "error: Realtime safety violation, baz called from a safe function.*Function undefined_function was deduced to need to be safe")

#Verify that nothing dies with very complex headers
add_test (header-includes sfpv ${SRC}/src/main.cpp)

#Check that information is translated over multiple translation units
add_test (multi-translation-unit sfpv ${SRC}/test/test-input.cpp ${SRC}/test/test-input2.cpp)
set_property (TEST multi-translation-unit PROPERTY PASS_REGULAR_EXPRESSION
    "error: Realtime safety violation, TestClass::baz.*warning: Function undefined_and_used was deduced")

#Verify that functions can be externally annotated
add_test (manual-whitelist sfpv "-W" ${SRC}/data/whitelist.txt ${SRC}/test/math-safety.cpp)

#Verify that virtual methods are handled correctly
add_test (virtual-calls sfpv  ${SRC}/test/virtual-methods.cpp)
