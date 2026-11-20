cmake_minimum_required(VERSION 3.15)
project(mayscord VERSION 0.1.0 LANGUAGES C)
find_package(CURL 7.86 REQUIRED)
add_library(mayscord
  src/core/bot.c
  src/util/json.c
  src/rest/http.c
  src/api/messages.c
  src/api/guilds.c
  src/api/interactions.c
  src/gateway/gateway.c
)
target_compile_features(mayscord PUBLIC c_std_99)
target_include_directories(mayscord PRIVATE src/internal)
target_include_directories(mayscord PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(mayscord PUBLIC CURL::libcurl)
option(MAYSCORD_BUILD_EXAMPLES "Build examples" ON)
option(MAYSCORD_BUILD_TESTS "Build tests" ON)
if(MAYSCORD_BUILD_EXAMPLES)
  add_executable(hello examples/hello.c)
  target_link_libraries(hello PRIVATE mayscord)
  add_executable(ping examples/ping.c)
  target_link_libraries(ping PRIVATE mayscord)
endif()
if(MAYSCORD_BUILD_TESTS)
  enable_testing()
  add_executable(test_mayscord tests/test.c)
  target_link_libraries(test_mayscord PRIVATE mayscord)
  add_test(NAME mayscord COMMAND test_mayscord)
endif()
install(TARGETS mayscord ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install(FILES include/mayscord.h DESTINATION include)
