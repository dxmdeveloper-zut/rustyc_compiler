cmake_minimum_required(VERSION 3.10)
project(compiler C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)


find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Generate lexer
FLEX_TARGET(Lexer src/z5.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
# Generate parser
BISON_TARGET(Parser src/def.yy ${CMAKE_CURRENT_BINARY_DIR}/def.tab.cc
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/def.tab.hh)

# Add generated files to the project
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

# Create executable
add_executable(compiler
        ${FLEX_Lexer_OUTPUTS}
        ${BISON_Parser_OUTPUTS}
        src/MainStack.hpp
        src/MainStack.cpp
        src/Compiler.hpp
        src/Compiler.cpp
        src/HashMap.hpp
        src/common.hpp
        src/RegisterManager.cpp
        src/RegisterManager.hpp
)

# Link with the lex library
target_link_libraries(compiler PRIVATE l)

# Include generated headers
target_include_directories(compiler PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Clean up generated files (optional, for 'make clean')
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c;${CMAKE_CURRENT_BINARY_DIR}/def.tab.cc;${CMAKE_CURRENT_BINARY_DIR}/def.tab.hh"
)