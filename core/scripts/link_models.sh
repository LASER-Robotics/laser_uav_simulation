#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/px4_firmware/Tools/simulation/gazebo-classic/sitl_gazebo-classic/models

cd "$SIMULATION_PATH"

for item in "$MY_PATH"/models/*; 
do
    # Check if the item is a directory
    if [ -d "$item" ]; then
        # Extract the directory name
        dir_name=$(basename "$item")
        # Create a symbolic link in the simulation directory
        ln -fs "$item" ./
    fi
done
