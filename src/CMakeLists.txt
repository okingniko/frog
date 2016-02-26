add_executable(frog_sample FrogSample.cc ConfigParser.cc)
target_link_libraries(frog_sample ${Boost_LIBRARIES})
target_link_libraries(frog_sample ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS frog_sample 
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR})

