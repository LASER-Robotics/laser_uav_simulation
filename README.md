# LASER UAV Simulation

`laser_uav_simulation` provides the Gazebo Classic launch files, UAV models and startup scripts used to test the LASER UAV System (LUS). It spawns x500 or lr7pro vehicles, starts their SITL firmware and supplies a tmux session for the flight stack.

This package is marked as deprecated in the existing project documentation. The instructions below describe the Gazebo Classic setup in this repository.

## Run

Install LUS with simulation support first. The scripts expect the standard checkout at `~/git/laser_uav_system` and the workspace at `~/laser_uav_system_ws`.

For the supplied PX4 session:

```bash
export REAL_UAV=false
cd ~/laser_uav_system_ws/src/laser_uav_simulation/tmux/one_drone_px4
./start.sh
```

The session uses tmuxinator and starts Gazebo, the vehicle, hardware API, estimation, control, planning and the terminal interface. Its `configs/` directory contains the spawn list, planner selection and monitored topics.

### Start the world and vehicle separately

Start Gazebo in one terminal:

```bash
ros2 launch laser_uav_simulation gazebo.launch.py world:=custom_empty real_time:=1.0
```

In another terminal, use the supplied PX4 spawn configuration:

```bash
ros2 launch laser_uav_simulation spawn_drones.launch.py spawn_drones_file:=$HOME/laser_uav_system_ws/src/laser_uav_simulation/tmux/one_drone_px4/configs/spawn.yaml
```

Spawning starts the model and its firmware. Start the hardware API and managers separately when not using the tmux session. For PX4, the spawner also starts `MicroXRCEAgent udp4 -p 8888`.

## Configuration

### World launch

| Argument | Default | Meaning |
| --- | --- | --- |
| `world` | `custom_empty` | World name without `.world`, loaded from `laser_gazebo_resources/worlds/`. |
| `real_time` | `1.0` | PX4 simulation speed factor, passed through `PX4_SIM_SPEED_FACTOR`. |
| `verbose` | `true` | Gazebo server logging. |
| `gazebo_config` | Installed `config/gazebo.yaml` | Gazebo ROS parameter file. |

`config/gazebo.yaml` sets `publish_rate: 200.0` Hz for the simulation clock.

### Vehicle spawn file

`spawn_drones_file` is a required path to a YAML file with the following structure:

```yaml
/**/**:
  ros__parameters:
    - id: 1
      type: "x500"
      fcu: "px4"
      pose_spawn: [0.0, 0.0, 0.2, 0.0]
      sensors: "--enable_ground_truth"
```

| Field | Meaning |
| --- | --- |
| `id` | Positive vehicle ID. `1` produces namespace `uav1` and PX4 instance `0`. Use a unique ID for each vehicle. |
| `type` | Airframe: `x500` or `lr7pro`. |
| `fcu` | Firmware: `px4` or `ap`. Set it explicitly; some older examples in `config/` omit it. |
| `pose_spawn` | Initial `[x, y, z, yaw]`, in metres and radians. Must contain four values. |
| `sensors` | Space-separated model-generator flags in one string. Use `""` for none. |

Use one firmware family per spawn file. The agent startup is selected from the final vehicle's `fcu`, so mixed PX4/ArduPilot files do not configure both communication paths automatically. The ArduPilot startup script currently uses instance `0`; the supplied multi-vehicle setup should not be treated as an ArduPilot multi-instance launch.

### Sensor and model flags

Combine flags in `sensors`, for example `"--enable_livox --enable_ground_truth"`.

| Flag | Effect |
| --- | --- |
| `--enable_d435_front`, `--enable_d435_down` | Adds a RealSense D435 in the selected direction. |
| `--enable_d435i_front`, `--enable_d435i_down` | Adds a RealSense D435i with an IMU. |
| `--enable_livox` | Adds the Livox LiDAR model. |
| `--enable_livox_45`, `--enable_livox_down` | Selects an angled or downward Livox installation. |
| `--enable_vio` | Enables the model's VIO sensor option. |
| `--enable_garmin` | Adds the rangefinder option. |
| `--enable_ground_truth` | Publishes ground-truth state for comparison. |
| `--enable_load_pendulum` | Adds a suspended-load model. |

## Interfaces

This package starts Gazebo, model plugins and firmware processes. Sensor topics are produced by the plugins in `laser_gazebo_resources`; their names and frames come from the generated model. PX4 firmware communication uses `/<UAV_NAME>/fmu/in/...` and `fmu/out/...`, with the hardware API providing the LUS-facing topics.

The spawn launch is configured through its YAML file rather than a ROS topic or service. Flight commands, estimator outputs and planning references are documented in `laser_uav_hw_api` and `laser_uav_managers`.
