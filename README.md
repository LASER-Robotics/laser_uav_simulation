# LASER UAV Simulation

This repository contains the necessary tools and scripts to simulate the **LASER UAV System**. The `laser_uav_simulation` package, which is part of the **LUS (LASER UAV System)**, provides all the functionalities to spawn and control a drone in a simulated environment using **Gazebo Classic**.

## Scripts

The scripts folder contains various helper scripts that automate common tasks within the simulation workflow. Here's how work the main scripts:

### `build_px4_firmware.sh`

This script compiles a custom version of the PX4 firmware, specifically tailored for a SITL (Software-In-The-Loop) simulation with Gazebo Classic for the **LUS** project.

Its main purpose is to replace the default ROS 2 communication bridge configuration with one specific to **LUS**. This ensures the simulated drone exchanges the correct data topics with the ROS 2 workspace. The script automates the entire process: cleaning, configuring, compiling the firmware, and then building the Gazebo plugin.

### `jinja_gen.py`

This Python script acts as a dynamic model generator for the **LUS** project.

Its main purpose is to create Gazebo SDF model files from a base Jinja template (`.sdf.jinja`). By passing command-line arguments, you can generate different UAV configurations on-the-fly, enabling or disabling specific sensors like cameras and LiDARs. This approach avoids the need to manually create and maintain dozens of static SDF files for every possible robot variation.

#### Key Functionalities

* **Dynamic Configuration:** The script uses command-line arguments (e.g., `--uav_model`, `--instance`, `--enable_livox`) to define the specific drone to be generated.
* **Templating Engine:** It uses the `Jinja2` library to inject these parameters into a master template file. The template contains logic to include or exclude sensor plugins based on the provided flags.
* **File Output:** The final, rendered SDF file is saved to the `/tmp/laser_uavs_description/sdf/` directory, ready to be spawned into a Gazebo simulation.

### `start_uxrce_protocol.sh`

This script launches the Micro-XRCE-DDS Agent, a critical piece of middleware for the **LUS (Laser UAV System)** project.

The primary role of this agent is to act as a **communication bridge** between the PX4 flight stack (running in SITL) and the ROS 2 network. PX4 uses the lightweight XRCE-DDS protocol, while ROS 2 uses the full DDS protocol. This agent seamlessly translates messages between the two environments.

In short, this script must be running for any ROS 2 node to communicate with the simulated UAVs. Without it, there is no way to receive sensor data from the drone or send commands to it.

### `spawn_drone.sh`

This is the primary script for launching a single, complete UAV agent into the simulation for the **LUS** project. It orchestrates the entire process, from model creation to launching the flight control software.

The script is designed to be configured through command-line arguments (for position) and environment variables (for drone type and sensors), making it highly flexible and reusable in complex, multi-drone scenarios.

#### Core Workflow

The script executes three main stages in sequence:

1.  **Generate Model:** First, it calls the `jinja_gen.py` script. Using environment variables like `$UAV_TYPE` and `$UAV_SENSORS`, it generates a custom SDF model file for the specific drone configuration required.

2.  **Spawn in Gazebo:** After waiting for the Gazebo server (`gzserver`) to be running, it uses the `gz model` command to spawn the newly generated SDF model into the simulation at the position and orientation passed as arguments (`x, y, z, yaw`).

3.  **Launch PX4 SITL:** Finally, it starts a dedicated PX4 Software-In-The-Loop instance for the spawned drone. It correctly configures the ROS 2 namespace and instance ID, ensuring the flight stack connects to the correct model in Gazebo and communicates properly on the ROS 2 network.

#### How to Use

This script is typically not run manually but is called by a ROS 2 launch file. It requires position/orientation as arguments and several environment variables to be set.

**Arguments:**
* `$1`: Initial X position.
* `$2`: Initial Y position.
* `$3`: Initial Z position.
* `$4`: Initial Yaw orientation (in radians).

**Environment Variables:**
* `$UAV_TYPE`: The base model of the drone (e.g., `x500`).
* `$UAV_NAME`: The unique name for the drone instance (e.g., `uav1`).
* `$UAV_SENSORS`: Command-line flags to pass to the generator script (e.g., `'--enable_livox --enable_d435i_front'`).

## Spawning a Single UAV

Follow the steps below to launch a single UAV into the **LUS (Laser UAV System)** simulation environment.

### 1. Start the Simulation

First, ensure a Gazebo simulation is open and running. The spawn script will wait for an active Gazebo process before proceeding.

*You can start an empty world or any other simulation environment required for your test.*

### 2. Configure and Spawn the Drone

In a **new terminal**, you will first configure the drone's specifications using environment variables and then execute the `spawn_drone.sh` script.

**First, set the variables:**
```bash
export UAV_NAME=uav1
export UAV_TYPE=x500
export UAV_SENSORS="--enable_vio"
export UAV_ESTIMATION_SOURCE="GNSS"
```
* **`UAV_NAME`**: The unique name and namespace for the drone (e.g., `uav1`).
* **`UAV_TYPE`**: The base airframe model to use (e.g., `x500`).
* **`UAV_SENSORS`**: A string of flags passed to the model generator to enable specific sensors.
* **`UAV_ESTIMATION_SOURCE`**: Used to configure the PX4 EKF2 estimator's primary data source (e.g., `GNSS`, `VIO`).

**Now, in the same terminal, execute the spawn script** with the desired position (x, y, z), orientation (yaw), and a unique ID:
```bash
~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/spawn_drone.sh 0.0 0.0 0.2 1.57 3
```
This script will automatically:
1.  Generate the custom SDF model file.
2.  Spawn the drone into Gazebo.
3.  **Launch the PX4 SITL software instance**, which will begin searching for the communication bridge.

### 3. Start the Communication Bridge

Finally, in a **third terminal**, start the Micro-XRCE-DDS Agent. This program is the bridge that enables communication between PX4 and ROS 2.

```bash
~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/start_uxrce_protocol.sh
```
The PX4 instance, which was launched in the previous step, was waiting for this agent. As soon as this script is executed, the connection will be established, and data will begin to flow from the drone to the ROS 2 topics.
