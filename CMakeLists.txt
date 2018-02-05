cmake_minimum_required (VERSION 3.1)
project (os-shell)

set(APP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set(APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/)

set(APP_SRC
    ${APP_SRC_DIR}/shell.cpp
)

set(APP_EXEC_SRC
    ${APP_SRC_DIR}/shell-main.cpp
)

set(APP_TESTS_SRC
    ${APP_SRC_DIR}/tests/test_suite.cpp
)

# Require C++ 14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common flags for clang++ and clang++
set(CMAKE_CXX_FLAGS_CLANG_GCC_COMMON "-fPIC -Wmain -W -Wall -Wextra -Wformat -Wformat-security -Werror=format-security -pedantic -pedantic-errors -fstack-protector-strong")
set(CMAKE_CXX_FLAGS_CLANG_LIBCXX "-stdlib=libc++")
set(CMAKE_CXX_FLAGS_CLANG "-Weverything -Wthread-safety ${CMAKE_CXX_FLAGS_CLANG_LIBCXX}")
set(CMAKE_CXX_FLAGS_CLANG_SAN "-fsanitize=address,undefined,leak")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-g -O0 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON} ${CMAKE_CXX_FLAGS_CLANG} ${CMAKE_CXX_FLAGS_CLANG_SAN}")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON} ${CMAKE_CXX_FLAGS_CLANG} ${CMAKE_CXX_FLAGS_CLANG_SAN}")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON} ${CMAKE_CXX_FLAGS_CLANG}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON} ${CMAKE_CXX_FLAGS_CLANG}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON} ${CMAKE_CXX_FLAGS_CLANG}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-g -O0 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON}")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON}")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-O2 ${CMAKE_CXX_FLAGS_CLANG_GCC_COMMON}")
else()
  message(WARNING "Note that the CMAKE_CXX_FLAGS were not configured for compilers other that clang++ and g++.")
endif()

include_directories(${APP_INCLUDE_DIR})

add_library(shell-lib ${APP_SRC})

add_executable(shell-main ${APP_EXEC_SRC})
target_link_libraries(shell-main shell-lib)

add_executable(test_suite ${APP_TESTS_SRC})
target_link_libraries(test_suite shell-lib)
