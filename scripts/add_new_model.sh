#!/bin/bash

./link_models.sh
./link_worlds.sh
./change_px4_cmake.sh
./link_airframes.sh

cd ~/git/laser_uav_system/ros_packages/px4_firmware

sudo rm -r build
mkdir build && cd build
cmake ..
make topic_bridge_files
make
cd ..
bash ./Tools/setup/ubuntu.sh
make px4_sitl
