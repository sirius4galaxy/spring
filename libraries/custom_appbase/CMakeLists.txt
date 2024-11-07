file(GLOB HEADERS "include/eosio/chain/*.hpp" )

add_library(custom_appbase application.cpp ${HEADERS} )
target_include_directories(custom_appbase PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(custom_appbase PUBLIC appbase eosio_chain fc )

add_subdirectory(tests)
