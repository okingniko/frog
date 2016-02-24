add_executable(configparser_unittest ConfigParser_test.cc ${SRC_PATH}/ConfigParser.cc)
target_link_libraries(configparser_unittest ${Boost_LIBRARIES})

set(CONF_PATH ${PROJECT_SOURCE_DIR}/conf/MainConf.xml)
message(STATUS ${CONF_PATH})
add_test(NAME configparser_unittest 
  COMMAND configparser_unittest ${CONF_PATH})

