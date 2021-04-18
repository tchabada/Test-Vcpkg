find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

add_executable(example_qt main.cpp ExampleWidget.cpp)
target_link_libraries(example_qt project_warnings project_options Qt5::Widgets)
