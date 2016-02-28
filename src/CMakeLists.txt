add_executable(frog_sample FrogSample.cc ConfigParser.cc)
target_link_libraries(frog_sample ${Boost_LIBRARIES})
target_link_libraries(frog_sample ${CMAKE_THREAD_LIBS_INIT})

add_executable(client_sample ClientSample.cc)
target_link_libraries(client_sample ${Boost_LIBRARIES})
target_link_libraries(client_sample ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS frog_sample client_sample
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR})

