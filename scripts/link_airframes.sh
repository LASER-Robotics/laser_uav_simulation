#!/bin/bash

# get the path to this script
MY_PATH="$(dirname "$(dirname "$(realpath "$0")")")"
SIMULATION_PATH=~/git/laser_uav_system/ros_packages/px4_firmware/ROMFS/px4fmu_common/init.d-posix/airframes

cd "$SIMULATION_PATH" || { echo "Failed to change directory to $SIMULATION_PATH"; exit 1; }
ln -fs "$MY_PATH"/ROMFS/airframes/* ./

FILE="$SIMULATION_PATH"/CMakeLists.txt

TEXT_COMMENT="#custom models"
TEXT_MODEL_1="22000_gazebo-classic_x500"
# TEXT_MODEL_2="22001_gazebo-classic_"

#sed -i "$(($(wc -l < $FILE)))i $TEXT" $FILE

# Check if the string already exists in the file
if ! grep -q "$TEXT_COMMENT" "$FILE"; then
    sed -i "$(($(wc -l < $FILE)))i \ \n  $TEXT_COMMENT" "$FILE"
fi

if ! grep -q "$TEXT_MODEL_1" "$FILE"; then
  sed -i "$(($(wc -l < $FILE)))i \  $TEXT_MODEL_1" "$FILE"
fi

# if ! grep -q "$TEXT_MODEL_2" "$FILE"; then
#     sed -i "$(($(wc -l < $FILE)))i $TEXT_MODEL_2" "$FILE"
# fi
