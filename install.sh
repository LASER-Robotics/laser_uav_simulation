#!/bin/bash

sudo apt-get install ros-humble-realsense2-*
sudo apt install ros-humble-librealsense2-*
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
colcon build --symlink-install --packages-select realsense2_camera realsense2_camera_msgs realsense2_description realsense_gazebo_plugin laser_uav_simulation ros2_livox_simulation livox_sdk2 livox_ros_driver2
