```
sudo apt-get install ros-$ROS_DISTRO-realsense2-*
```
```
sudo apt install ros-$ROS_DISTRO-librealsense2*
```
```
sudo apt install ros-$ROS_DISTRO-gazebo*
```

```
cd ~/laser_uav_system_ws/src
git clone https://github.com/IntelRealSense/realsense-ros.git -b ros2-development
git clone https://github.com/pal-robotics/realsense_gazebo_plugin.git -b foxy-devel
cd ~/laser_uav_system_ws
colcon build --symlink-install
```

```
cd ~/git/laser_uav_system/ros_packages/px4_firmware
rm -r build
mkdir build && cd build
cmake ..
make topic_bridge_files
make
cd ..
bash ./Tools/setup/ubuntu.sh
make px4_sitl
```
