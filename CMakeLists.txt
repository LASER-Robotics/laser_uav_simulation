cmake_minimum_required(VERSION 3.8)
project(laser_uav_simulation)

# =========================================================
# C++ configuration
# =========================================================

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

if (
  CMAKE_COMPILER_IS_GNUCXX
  OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
)
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
  )
endif ()

# =========================================================
# Dependencies
# =========================================================

find_package(ament_cmake REQUIRED)

find_package(gazebo REQUIRED)
find_package(gazebo_ros REQUIRED)

find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# Gazebo Classic may provide library names instead of
# complete library paths.
link_directories(
  ${GAZEBO_LIBRARY_DIRS}
)

# =========================================================
# Payload connection plugin
# =========================================================

set(
  payload_connection_plugin_name
  payload_connection_plugin
)

add_library(
  ${payload_connection_plugin_name}
  SHARED
  src/payload_connection_plugin.cpp
)

target_include_directories(
  ${payload_connection_plugin_name}
  PRIVATE
    ${GAZEBO_INCLUDE_DIRS}
)

ament_target_dependencies(
  ${payload_connection_plugin_name}
  gazebo_ros
  rclcpp
  std_msgs
)

target_link_libraries(
  ${payload_connection_plugin_name}
  ${GAZEBO_LIBRARIES}
)

# Some Gazebo installations export additional compilation
# flags through GAZEBO_CXX_FLAGS.
if (GAZEBO_CXX_FLAGS)
  separate_arguments(
    GAZEBO_CXX_FLAGS_LIST
    NATIVE_COMMAND
    "${GAZEBO_CXX_FLAGS}"
  )

  target_compile_options(
    ${payload_connection_plugin_name}
    PRIVATE
      ${GAZEBO_CXX_FLAGS_LIST}
  )
endif ()

# =========================================================
# Installation
# =========================================================

install(
  TARGETS
    ${payload_connection_plugin_name}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  PROGRAMS
    scripts/jinja_gen.py
    scripts/autostart.py
  DESTINATION
    lib/${PROJECT_NAME}
)

install(
  DIRECTORY
    scripts
    models
    config
    launch
  DESTINATION
    share/${PROJECT_NAME}
)

# =========================================================
# Tests
# =========================================================

if (BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif ()

ament_export_dependencies(
  gazebo_ros
  rclcpp
  std_msgs
)

ament_package()
