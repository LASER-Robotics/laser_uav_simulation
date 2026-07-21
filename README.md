# DEPRECATED

# LASER UAV Simulation

This repository contains the necessary tools and scripts to simulate the **LASER UAV System**. The `laser_uav_simulation` package, which is part of the **LUS (LASER UAV System)**, provides all the functionalities to spawn a drone in a simulated environment using **Gazebo Classic**.

## Spawning UAVs

The spawning process in the **LUS** simulation is orchestrated by a ROS 2 launch file that reads a YAML configuration. This allows for easily defining multiple drones with different configurations (types, sensors, positions) in a single file.

### 1. Configuration File (`.yaml`)

To spawn drones, you must define them in a YAML file. The launch file iterates through this list and spawns each drone in sequence.

**Example Configuration:**
```yaml
/**:
  ros__parameters:
    - id: 1
      type: "lr7pro"                 # Options: "x500", "lr7pro"
      pose_spawn: [0.0, 0.0, 0.2, 0.0] # [X, Y, Z, Yaw]
      sensors: "--enable_ground_truth"

    - id: 2
      type: "x500"
      pose_spawn: [1.0, 0.0, 0.2, 0.0]
      sensors: ""
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
| `--enable_garmin` | Enables rangefinder sensor simulation. |
| `--enable_ground_truth` | Publishes the exact ground truth pose (useful for validation/debugging). |

**Parameters:**

  * **`id`**: A unique integer identifier. This defines the namespace (e.g., `1` becomes `uav1`).
  * **`type`**: The airframe model name.
  * **`pose_spawn`**: The initial position and orientation `[x, y, z, yaw]`.
  * **`sensors`**: A list of sensor flags to pass to the model generator (e.g., `"--enable_livox"`).

### 2. Launching the Simulation

Use the `spawn_drones.launch.py` file to spawn the agents defined in your configuration file. 

**Important:** This command only spawns the UAV models. The simulation world must be launched beforehand using a separate launch file.

This launch file also automatically starts the `MicroXRCEAgent` required for PX4 communication.

**Command:**

```bash
ros2 launch laser_uav_simulation spawn_drones.launch.py spawn_drones_file:=<path_to_your_yaml>
