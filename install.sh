#!/bin/bash

sudo apt-get install ros-humble-realsense2-*
sudo apt install ros-humble-librealsense2-*
sudo apt install ros-humble-gazebo-*

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts

./add_new_model.sh
./link_models.sh
./link_worlds.sh

cd ~/laser_uav_system_ws/
colcon build --symlink-install --packages-select realsense_ros realsense_gazebo_plugin
