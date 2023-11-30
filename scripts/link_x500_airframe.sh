#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/PX4-Autopilot/ROMFS/px4fmu_common/init.d-posix/airframes

cd "$SIMULATION_PATH"
ln -fs "$MY_PATH"/ROMFS/px4fmu_common/init.d-posix/airframes/22000_gz_x500_v2 ./

file="$SIMULATION_PATH"/CMakeLists.txt

if  grep -q "Reserve for custom models" "$file"; then
  sed -i "/\Reserve for custom models/ a\\  22000_gz_x500_v2" "$file"
fi
