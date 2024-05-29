#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/px4_firmware/Tools/simulation/gazebo-classic/sitl_gazebo-classic/models

cd "$SIMULATION_PATH"
ln -fs "$MY_PATH"/models/* ./
