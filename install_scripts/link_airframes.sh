#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/px4_firmware/ROMFS/px4fmu_common/init.d-posix/airframes

cd "$SIMULATION_PATH"
ln -fs "$MY_PATH"/ROMFS/px4fmu_common/init.d-posix/airframes/* ./

FILE="$SIMULATION_PATH"/CMakeLists.txt

TEXT="		#custom models\n	22000_gazebo-classic_laser_x500"

sed -i "$(($(wc -l < $FILE) - 1))i $TEXT" $FILE
