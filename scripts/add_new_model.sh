#!/bin/bash

./change_px4_cmake.sh
./link_airframe.sh

cd ~/git/laser_uav_system/ros_packages/px4_firmware

rm -r build
mkdir build && cd build
cmake ..
make topic_bridge_files
make
cd ..
bash ./Tools/setup/ubuntu.sh
make px4_sitl
