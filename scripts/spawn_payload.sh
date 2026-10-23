#!/bin/bash

x=$1
y=$2
z=$3
roll=$4
pitch=$5
yaw=$6

# Agora deslocamos 6 argumentos em vez de 3
shift 6

JINJA_GEN="$(dirname "$(readlink -f "$0")")/jinja_gen.py"

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
  -z "$z" \
  -R "$roll" \
  -P "$pitch" \
  -Y "$yaw"
