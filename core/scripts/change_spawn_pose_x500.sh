#!/bin/bash

cd ~/git/laser_uav_system/ros_packages/px4_firmware/Tools/simulation/gazebo-classic/

sed -i '130s/-x 1\.01 -y 0\.98 -z 0\.83/-x $spawn_pose_x500_x -y $spawn_pose_x500_y -z $spawn_pose_x500_z/' sitl_run.sh

if ! grep -q "spawn_pose_x500_x" ~/.bashrc; then
    echo -e "\n\n#coordenadas x500 \nexport spawn_pose_x500_x=0.0\nexport spawn_pose_x500_y=0.0\nexport spawn_pose_x500_z=1.5" >> ~/.bashrc
fi
