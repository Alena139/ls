cmake_minimum_required (VERSION 2.1)
project(app)

include_directories(include)
#add_definitions(-D_GLIBCXX_DEBUG)

if (${CMAKE_MAJOR_VERSION} GREATER 3)
  set(CMAKE_STANDARD_REQUIRED 11)
  set(CMAKE_CXX_STANDARD 11)
  if (SANITIZE EQUAL 1)
    set(CMAKE_CXX_FLAGS "-fsanitize=thread")
  endif (SANITIZE EQUAL 1)
else (${CMAKE_MAJOR_VERSION} GREATER 3)
  #set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-unused-function -g")
  set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")
endif (${CMAKE_MAJOR_VERSION} GREATER 3)

add_executable(app main.cpp crc.cpp)
#set(CMAKE_BUILD_TYPE Debug)
