#!/bin/bash

x=$1
y=$2
z=$3

JINJA_GEN=~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/jinja_gen.py

$JINJA_GEN \
    --namespace payload \
    --uav_model payload \
    --instance 0 \
    --x "$x" \
    --y "$y" \
    --z "$z"

echo "Waiting for gazebo run."

while ! pgrep -x "gzserver" > /dev/null; do
    sleep 1
done

echo "Spawning payload."
echo "Position x: $x, y: $y, z: $z"

while gz model \
    --verbose \
    --spawn-file="/tmp/laser_uavs_description/sdf/payload_0.sdf" \
    --model-name=payload \
    -x "$x" \
    -y "$y" \
    -z "$z" |
    grep -q "An instance of Gazebo is not running."
do
    echo "gzserver not ready yet, trying again!"
    sleep 1
done
