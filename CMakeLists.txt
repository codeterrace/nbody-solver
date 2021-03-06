cmake_minimum_required(VERSION 3.10)

project(nbody VERSION 0.1)

enable_testing()

configure_file(config.h.in config.h)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Could not find pthread library.")
endif()

set (CMAKE_C_FLAGS "-Wall")

set (CT_LIB_SOURCES
  src/pool.c
  src/queue.c
  src/barrier.c
  src/threadpool.c
  src/tictoc.c
  src/error.c
  src/task.c
  )

add_library(ct_lib STATIC ${CT_LIB_SOURCES})

target_include_directories(ct_lib PUBLIC
  "${PROJECT_SOURCE_DIR}/src"
  )

target_link_libraries(ct_lib Threads::Threads)

add_subdirectory(tests)
add_subdirectory(examples EXCLUDE_FROM_ALL)

set (NBODY_SOURCES
  src/bhtree.c
  src/nbody.c
  )

add_executable(nbody ${NBODY_SOURCES})

target_include_directories(nbody PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/src"
  )

target_link_libraries(nbody ct_lib m)

#add_custom_command (TARGET nbody POST_BUILD
#  COMMAND objdump -CSlr nbody > nbody.s
#  )

