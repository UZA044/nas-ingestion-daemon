cmake_minimum_required(VERSION 3.10)
  project(NasIngestionDaemon C)

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  # ---- options -------------------------------------------------
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED ON)

  # ---- library (the daemon core) --------------------------------
  add_library(nas-daemon
      src/logger.c
      src/config.c
      src/watcher.c
      src/detector.c
      src/router.c
      src/pipeline.c
  )

  target_include_directories(nas-daemon PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  # ---- test executable -------------------------------------------
  add_executable(test_logger
      tests/test_logger.c          # you will create this file
  )
  target_link_libraries(test_logger PRIVATE nas-daemon)

  add_executable(test_config_parser
      tests/test_config_parser.c
  )
  target_link_libraries(test_config_parser PRIVATE nas-daemon)

  add_executable(test_watcher
      tests/test_watcher.c
  )
  target_link_libraries(test_watcher PRIVATE nas-daemon)

  add_executable(test_detector
      tests/test_detector.c
  )
  target_link_libraries(test_detector PRIVATE nas-daemon)

  add_executable(test_router
    tests/test_router.c
  )
   target_link_libraries(test_router PRIVATE nas-daemon)

  add_executable(test_pipeline
    tests/test_pipeline.c
  )
  target_link_libraries(test_pipeline PRIVATE nas-daemon)

  


  # Optional: enable warnings
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
      target_compile_options(test_logger PRIVATE -Wall -Wextra -pedantic)
      target_compile_options(test_config_parser PRIVATE -Wall -Wextra -pedantic)
      target_compile_options(test_watcher PRIVATE -Wall -Wextra -pedantic)
      target_compile_options(test_detector PRIVATE -Wall -Wextra -pedantic)

  endif()