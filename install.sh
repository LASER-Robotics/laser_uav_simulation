#!/bin/bash

sudo apt install ros-humble-gazebo-*

cd third_party/livox_sdk
mkdir build
cd build
cmake .. && make -j
sudo make install

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/core/scripts

./add_new_model.sh
./link_models.sh
./link_worlds.sh

cd ~/laser_uav_system_ws/
colcon build --symlink-install --packages-select realsense_gazebo_plugin laser_uav_simulation ros2_livox_simulation livox_sdk2 livox_ros_driver2
