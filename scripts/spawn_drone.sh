#!/usr/bin/env bash

if [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" ]]; then
    echo "Error: the first 4 parameters are required!"
    echo "The parameters, in order, are: x, y, z, and yaw."
    exit 1
fi

uavs_available=("x500")

if [[ ! " ${uavs_available[@]} " =~ " $uav_type " ]]; then
    echo "The UAV type '$uav_type' is not available."
    exit 1
fi

~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/jinja_gen.py $uav_name $uav_type $uav_sensors

echo "Waiting for gazebo run."
while ! pgrep -x "gzserver" > /dev/null; do
    sleep 1
done
sleep 5
echo "Spawning the uav $uav_type with namespace $uav_name and sensors: $uav_sensors."
echo "In Position x: $1, y: $2, z: $3, orientation: $4."

while gz model --verbose --spawn-file="/tmp/laser_uavs_description/sdf/${uav_type}.sdf" --model-name=${uav_name}_${uav_type} -x $1 -y $2 -z $3 -Y $4 | grep -q "An instance of Gazebo is not running."; do
	echo "gzserver not ready yet, trying again!"
	sleep 1
done

export PX4_UXRCE_DDS_NS=$uav_name
export PX4_SIM_MODEL=$uav_type

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/

~/git/laser_uav_system/ros_packages/px4_firmware/build/px4_sitl_default/bin/px4 -s ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/etc/init.d-posix/rcS
