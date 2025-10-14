#!/usr/bin/env bash

cd ~/git/laser_uav_system/ros_packages/px4_firmware
# make distclean
# make px4_sitl_default 
cd build/px4_sitl_default/build_gazebo-classic
cmake ../../../Tools/simulation/gazebo-classic/sitl_gazebo-classic
make
