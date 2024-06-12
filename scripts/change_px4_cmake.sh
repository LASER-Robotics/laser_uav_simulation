#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/px4_firmware/src/modules/simulation/simulator_mavlink

cd "$SIMULATION_PATH"

FILE="$SIMULATION_PATH"/sitl_targets_gazebo-classic.cmake

MODELS_TEXT="x500"

sed -i "/set(models/a $MODELS_TEXT" $FILE
