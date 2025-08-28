#!/usr/bin/env bash

cd ~/git/laser_uav_system/ros_packages/px4_firmware
rm ./src/modules/uxrce_dds_client/dds_topics.yaml
cp ~/git/laser_uav_system/ros_packages/laser_uav_simulation/px4_dds_topics.yaml ./src/modules/uxrce_dds_client/dds_topics.yaml
make distclean
make px4_sitl_default 
cd build/px4_sitl_default/build_gazebo-classic
cmake ../../../Tools/simulation/gazebo-classic/sitl_gazebo-classic
make
