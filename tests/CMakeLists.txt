add_executable(configparser_unittest ConfigParser_test.cc ${SRC_PATH}/ConfigParser.cc)
target_link_libraries(configparser_unittest ${Boost_LIBRARIES})

set(CONF_PATH ${PROJECT_SOURCE_DIR}/conf/MainConf.xml)
# message(STATUS ${CONF_PATH})
add_test(NAME configparser_unittest 
  COMMAND configparser_unittest ${CONF_PATH})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-access-control")
add_executable(clientserverparse_unittest Client_Server_Parse_test.cc)
target_link_libraries(clientserverparse_unittest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(clientserverparse_unittest ${Boost_LIBRARIES})
add_test(NAME clientserverparse_unittest
  COMMAND clientserverparse_unittest)

