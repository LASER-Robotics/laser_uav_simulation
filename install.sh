#!/bin/bash

sudo apt-get install ros-humble-realsense2-* -y
sudo apt install ros-humble-librealsense2-* -y
sudo apt install ros-humble-gazebo-* -y

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts

./add_new_model.sh

cd ~/laser_uav_system_ws/src
ln -s ./laser_uav_simulation/third_party ./

