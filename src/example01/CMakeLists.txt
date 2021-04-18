find_package(Boost 1.75.0 REQUIRED COMPONENTS system thread coroutine context)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(example_asio main.cpp Application.cpp)
target_link_libraries(example_asio project_warnings project_options ${Boost_LIBRARIES})
