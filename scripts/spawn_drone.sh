#!/usr/bin/env bash

if [[ -z "$1" || -z "$2" || -z "$3" || -z "$4" ]]; then
    echo "Error: the first 4 parameters are required!"
    echo "The parameters, in order, are: x, y, z, and yaw."
    exit 1
fi

uavs_available=("x500" "zmr250")

if [[ ! " ${uavs_available[@]} " =~ " $UAV_TYPE " ]]; then
    echo "The UAV type '$UAV_TYPE' is not available."
    exit 1
fi

instance=$(echo "$UAV_NAME" | sed -n 's/^uav\([0-9]\+\)$/\1/p')
instance=$((instance - 1))

~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/jinja_gen.py --namespace $UAV_NAME --uav_model $UAV_TYPE --instance $instance --x $1 --y $2 --z $3 --yaw $4 $UAV_SENSORS

echo "Waiting for gazebo run."
while ! pgrep -x "gzserver" > /dev/null; do
    sleep 1
done
sleep 5
echo "Spawning the uav $UAV_TYPE with namespace $UAV_NAME and sensors: $UAV_SENSORS."
echo "In Position x: $1, y: $2, z: $3, orientation: $4."

while gz model --verbose --spawn-file="/tmp/laser_uavs_description/sdf/${UAV_TYPE}_$instance.sdf" --model-name=${UAV_NAME}_${UAV_TYPE}_$5 -x $1 -y $2 -z $3 -Y $4 | grep -q "An instance of Gazebo is not running."; do
	echo "gzserver not ready yet, trying again!"
	sleep 1
done

export PX4_UXRCE_DDS_NS=$UAV_NAME
export PX4_SIM_MODEL=$UAV_TYPE

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/

~/git/laser_uav_system/ros_packages/px4_firmware/build/px4_sitl_default/bin/px4 -i $instance -s ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/etc/init.d-posix/rcS
