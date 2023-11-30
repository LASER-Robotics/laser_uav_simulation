#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/PX4-Autopilot/Tools/simulation/gz/models

cd "$SIMULATION_PATH"
ln -fs "$MY_PATH"/models/x500_v2 ./
