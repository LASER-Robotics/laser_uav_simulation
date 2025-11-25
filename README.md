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

## Spawning UAVs

The spawning process in the **LUS** simulation is now orchestrated by a ROS 2 launch file that reads a YAML configuration. This allows for easily defining multiple drones with different configurations (types, sensors, positions) in a single file.

### 1. Configuration File (`.yaml`)

To spawn drones, you must define them in a YAML file. The launch file iterates through this list and spawns each drone in sequence.

**Example Configuration:**
```yaml
/**:
  ros__parameters:
    - id: 1
      type: "lr7pro"                 # Options: "x500", "lr7pro"
      pose_spawn: [0.0, 0.0, 0.2, 0.0] # [X, Y, Z, Yaw]
      sensors: ["--enable_ground_truth"] 

    - id: 2
      type: "x500"
      pose_spawn: [1.0, 0.0, 0.2, 0.0]
      sensors: []
````

**Available Sensors:**

The `sensors` list allows you to attach specific hardware or enable data streams for each drone. You can combine multiple sensors by adding them to the list (e.g., `["--enable_livox", "--enable_d435i_front"]`).

| Flag | Description |
| :--- | :--- |
| `--enable_d435_front` | Adds a Realsense D435 depth camera facing **forward**. |
| `--enable_d435i_front`| Adds a Realsense D435i (with IMU) depth camera facing **forward**. |
| `--enable_d435_down` | Adds a Realsense D435 depth camera facing **downward**. |
| `--enable_d435i_down` | Adds a Realsense D435i (with IMU) depth camera facing **downward**. |
| `--enable_livox` | Adds a Livox Mid360 LiDAR sensor. |
| `--enable_vio` | Enables Visual Inertial Odometry sensor simulation. |
| `--enable_ground_truth` | Publishes the exact ground truth pose (useful for validation/debugging). |

**Parameters:**

  * **`id`**: A unique integer identifier. This defines the namespace (e.g., `1` becomes `uav1`).
  * **`type`**: The airframe model name.
  * **`pose_spawn`**: The initial position and orientation `[x, y, z, yaw]`.
  * **`sensors`**: A list of sensor flags to pass to the model generator (e.g., `"--enable_livox"`).

### 2\. Launching the Simulation

Use the `spawn_drones.launch.py` file to spawn the agents defined in your config file. This launch file also automatically starts the `MicroXRCEAgent` required for PX4 communication.

**Command:**

```bash
ros2 launch laser_uav_simulation spawn_drones.launch.py spawn_drones_file:=<path_to_your_yaml>
```

**Example:**

```bash
ros2 launch laser_uav_simulation spawn_drones.launch.py spawn_drones_file:=$(ros2 pkg prefix --share laser_uav_simulation)/config/spawn_multi_drones.yaml
```

-----

### Backend Script: `spawn_drone.sh`

This script is the low-level worker called by the Python launch file for each drone entry in the YAML. It handles the specific sequence of generating the SDF, spawning it in Gazebo, and launching the PX4 SITL instance.

#### Core Workflow

1.  **Generate Model:** Calls `jinja_gen.py` with the arguments provided by the launch file to generate a custom SDF.
2.  **Spawn in Gazebo:** Waits for the Gazebo server and spawns the model using `gz model`.
3.  **Launch PX4 SITL:** Starts the PX4 instance with the correct instance ID (`-i`) and namespace (`PX4_UXRCE_DDS_NS`), ensuring unique communication channels for each drone.

#### Arguments (Internal Use)

While mostly used by the launch file, the script accepts the following order of arguments:

  * `$1`: Namespace (e.g., `uav1`)
  * `$2`: UAV Model (e.g., `x500`)
  * `$3`: X Position
  * `$4`: Y Position
  * `$5`: Z Position
  * `$6`: Yaw Orientation
  * `$7...`: Sensor flags

<!-- end list -->

### 3. Start the Communication Bridge

Finally, in a **third terminal**, start the Micro-XRCE-DDS Agent. This program is the bridge that enables communication between PX4 and ROS 2.

```bash
~/git/laser_uav_system/ros_packages/laser_uav_simulation/scripts/start_uxrce_protocol.sh
```
The PX4 instance, which was launched in the previous step, was waiting for this agent. As soon as this script is executed, the connection will be established, and data will begin to flow from the drone to the ROS 2 topics.
