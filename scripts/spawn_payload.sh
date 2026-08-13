#!/bin/bash

x=$1
y=$2
z=$3

shift 3

JINJA_GEN=~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/jinja_gen.py

"$JINJA_GEN" \
  --namespace payload \
  --uav_model payload \
  --instance 0 \
  --x "$x" \
  --y "$y" \
  --z "$z" \
  "$@"

echo "Waiting for Gazebo."

while ! pgrep -x "gzserver" > /dev/null; do
  sleep 1
done

gz model \
  --verbose \
  --spawn-file="/tmp/laser_uavs_description/sdf/payload_0.sdf" \
  --model-name=payload \
  -x "$x" \
  -y "$y" \
  -z "$z"
