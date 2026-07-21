#!/bin/bash

instance=$(echo "$1" | sed -n 's/^uav\([0-9]\+\)$/\1/p')
instance=$((instance - 1))

~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/jinja_gen.py --namespace $1 --uav_model $2 --flight_controller $3 --instance $instance --x $4 --y $5 --z $6 --yaw $7 $8

echo "Waiting for gazebo run."
while ! pgrep -x "gzserver" > /dev/null; do
    sleep 1
done
sleep 5
echo "Spawning the uav $2 with namespace $1 and fcu: $3, sensors: $8."
echo "In Position x: $4, y: $5, z: $6, orientation: $7."

while gz model --verbose --spawn-file="/tmp/laser_uavs_description/sdf/$2_$instance.sdf" --model-name=$1_$2_$instance -x $4 -y $5 -z $6 -Y $7 | grep -q "An instance of Gazebo is not running."; do
	echo "gzserver not ready yet, trying again!"
	sleep 1
done

export PX4_UXRCE_DDS_NS=$1
export PX4_SIM_MODEL=$2

cd ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/

if [ "$3" == "px4" ]; then
    ~/git/laser_uav_system/ros_packages/px4_firmware/build/px4_sitl_default/bin/px4 -i $instance -s ~/git/laser_uav_system/ros_packages/laser_uav_simulation/ROMFS/etc/init.d-posix/rcS
elif [ "$3" == "ap" ]; then
    ros2 launch ardupilot_sitl sitl_dds_udp.launch.py transport:=udp4 synthetic_clock:=True wipe:=False model:=gazebo-iris speedup:=1 slave:=0 instance:=0 defaults:=$(ros2 pkg prefix ardupilot_sitl)/share/ardupilot_sitl/config/default_params/copter.parm,$(ros2 pkg prefix ardupilot_sitl)/share/ardupilot_sitl/config/default_params/dds_udp.parm sim_address:=127.0.0.1 master:=tcp:127.0.0.1:5760 sitl:=127.0.0.1:5501
fi
