# Generic test that uses conan libs
add_executable(game main.cpp)
target_link_libraries(
  game
  PRIVATE
    solarized_colors
    project_options
    project_warnings
    CONAN_PKG::docopt.cpp
    CONAN_PKG::imgui-sfml
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    CONAN_PKG::Outcome)

add_library(solarized_colors
    solarized.cpp)
  target_link_libraries(
    solarized_colors
    PRIVATE
      CONAN_PKG::imgui-sfml
    )

  configure_file(${CMAKE_SOURCE_DIR}/assets/cross.png ${CMAKE_BINARY_DIR}/assets/cross.png COPYONLY)
  configure_file(${CMAKE_SOURCE_DIR}/assets/circle.png ${CMAKE_BINARY_DIR}/assets/circle.png COPYONLY)
