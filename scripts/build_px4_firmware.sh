#!/usr/bin/env bash

if [ "${GITHUB_ACTIONS}" == "true" ]; then
    BASE_DIR="$GITHUB_WORKSPACE"
else
    BASE_DIR="$(pwd)"
fi

cd $BASE_DIR/git/laser_uav_system/ros_packages/px4_firmware
make distclean
make px4_sitl_default 
cd build/px4_sitl_default/build_gazebo-classic
cmake ../../../Tools/simulation/gazebo-classic/sitl_gazebo-classic
make
