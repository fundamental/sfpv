cmake_minimum_required (VERSION 2.8)
project (sfpv)

set (sfpv_VERSION_MAJOR 0)
set (sfpv_VERSION_MINOR 1)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
#LLVM

#include(cotire)


#Clang (I have no idea why ncurses suddenly became a dep of libclang)
#      (And now zlib is suddenly on that list too...)
set (CLANG_LIBRARIES
         clang
         clangDriver
         clangFrontend
         clangParse
         clangSema
         clangEdit
         clangLex
         clangAST
         clangAnalysis
         clangSerialization
         clangBasic
         ncurses
         z)

add_definitions (-fno-rtti -Wno-unknown-warning-option)
set (CMAKE_CXX_COMPILER clang++)

execute_process(COMMAND llvm-config --libs
    OUTPUT_VARIABLE LLVM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --cxxflags
    OUTPUT_VARIABLE LLVM_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --version
    OUTPUT_VARIABLE LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --bindir
    OUTPUT_VARIABLE LLVM_BIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --libdir
    OUTPUT_VARIABLE LLVM_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions (-DCLANG_LOCATION="${LLVM_BIN_DIR}/clang")
link_directories (${LLVM_LIB_DIR})

if (LLVM_VERSION VERSION_LESS "3.5")
    add_definitions(-DOLD_LLVM)
endif ()


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_FLAGS} -std=c++11")
message(status "Build Flags '${CMAKE_CXX_FLAGS}'")

add_definitions (-DVERSION="${sfpv_VERSION_MAJOR}.${sfpv_VERSION_MINOR}")
add_executable (sfpv src/Errors.cpp src/GraphBuilder.cpp src/Deductions.cpp
    src/TranslationUnit.cpp src/Calls.cpp src/Callees.cpp src/sfpv.cpp)
target_link_libraries (sfpv  ${CLANG_LIBRARIES}  ${LLVM_LIBRARIES} dl pthread)
#cotire (sfpv)

add_executable (sfpf src/Errors.cpp src/GraphBuilder.cpp src/ReverseDeductions.cpp
    src/TranslationUnit.cpp src/Calls.cpp src/Callees.cpp src/sfpf.cpp)
target_link_libraries (sfpf  ${CLANG_LIBRARIES}  ${LLVM_LIBRARIES} dl pthread)
#cotire (sfpf)

######################################################################
#               Tests to verify it works                             #
######################################################################
set(SRC ${CMAKE_CURRENT_SOURCE_DIR})
enable_testing ()

#Verify one file basic behavior
add_test (test-one sfpv ${SRC}/test/test-input.c)
set_property (TEST test-one
    PROPERTY PASS_REGULAR_EXPRESSION
    "undefined_function.*baz|baz.*undefined_function")

#Verify that nothing dies with very complex headers
add_test (header-includes sfpv -C "-D__STDC_CONSTANT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -D__STDC_LIMIT_MACROS=1" ${SRC}/src/sfpv.cpp)

#Check that information is translated over multiple translation units
add_test (multi-translation-unit sfpv ${SRC}/test/test-input.cpp ${SRC}/test/test-input2.cpp)
set_property (TEST multi-translation-unit PROPERTY PASS_REGULAR_EXPRESSION
    "error: Realtime safety violation, TestClass::baz.*warning: Function undefined_and_used was deduced")

#Verify that functions can be externally annotated
add_test (manual-whitelist sfpv "-W" ${SRC}/data/whitelist.txt ${SRC}/test/math-safety.cpp)

#Verify that virtual methods are handled correctly
add_test (virtual-calls sfpv  ${SRC}/test/virtual-methods.cpp)

#Verify that 'operator new' was found
add_test (op-new sfpv -v ${SRC}/test/cpp-new-detection.cpp)
set_property (TEST op-new PROPERTY PASS_REGULAR_EXPRESSION
    "operator new.*operator delete")

add_test (malloc-blacklist sfpv "-B" ${SRC}/data/blacklist.txt ${SRC}/test/malloc-blacklist.c)
set_property (TEST malloc-blacklist PROPERTY PASS_REGULAR_EXPRESSION
    "error: Realtime safety violation, malloc called from a safe function, but it is an function interacting with Memory")

add_test (func-ptr sfpv ${SRC}/test/function-pointer.c)
set_property (TEST func-ptr PROPERTY PASS_REGULAR_EXPRESSION
    "error:")
#add_test (lambda-call sfpv ${SRC}/test/lambda-call.cpp)
#set_property (TEST lambda-call PROPERTY PASS_REGULAR_EXPRESSION
#    "error: Realtime safety violation, malloc_of_doooooooooom called from a safe function, but it was annotated non-realtime")
add_test (func-ptr-parameters sfpv ${SRC}/test/function-pointer-call.cpp)
set_property (TEST func-ptr-parameters PROPERTY PASS_REGULAR_EXPRESSION
    "error: Realtime safety violation, malloc_of_doooooooooom called from a safe function, but it was annotated non-realtime")
