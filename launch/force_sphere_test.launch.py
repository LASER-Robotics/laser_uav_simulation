#!/usr/bin/env python3

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    TimerAction,
)
from launch.conditions import IfCondition
from launch.launch_description_sources import (
    PythonLaunchDescriptionSource,
)
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import (
    get_package_share_directory,
)


def generate_launch_description():
    simulation_share = get_package_share_directory(
        "laser_uav_simulation"
    )

    gazebo_share = get_package_share_directory(
        "gazebo_ros"
    )

    uav1_sdf = (
        simulation_share
        + "/models/force_sphere_uav1/model.sdf"
    )

    uav2_sdf = (
        simulation_share
        + "/models/force_sphere_uav2/model.sdf"
    )

    payload_sdf = (
        simulation_share
        + "/models/point_payload/model.sdf"
    )

    start_gazebo = LaunchConfiguration(
        "start_gazebo"
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            gazebo_share
            + "/launch/gazebo.launch.py"
        ),
        condition=IfCondition(
            start_gazebo
        ),
        launch_arguments={
            "pause": "true",
            "verbose": "false",
        }.items(),
    )

    spawn_uav1 = Node(
        package="gazebo_ros",
        executable="spawn_entity.py",
        name="spawn_uav1_force_sphere",
        output="screen",
        arguments=[
            "-entity",
            "uav1_force_sphere",
            "-file",
            uav1_sdf,
            "-x",
            "-0.25",
            "-y",
            "0.0",
            "-z",
            "4.968245837",
        ],
    )

    spawn_uav2 = Node(
        package="gazebo_ros",
        executable="spawn_entity.py",
        name="spawn_uav2_force_sphere",
        output="screen",
        arguments=[
            "-entity",
            "uav2_force_sphere",
            "-file",
            uav2_sdf,
            "-x",
            "0.25",
            "-y",
            "0.0",
            "-z",
            "4.968245837",
        ],
    )

    spawn_payload = Node(
        package="gazebo_ros",
        executable="spawn_entity.py",
        name="spawn_point_payload",
        output="screen",
        arguments=[
            "-entity",
            "point_payload",
            "-file",
            payload_sdf,
            "-x",
            "0.0",
            "-y",
            "0.0",
            "-z",
            "4.0",
        ],
    )

    spawn_models = TimerAction(
        period=2.0,
        actions=[
            spawn_uav1,
            spawn_uav2,
            spawn_payload,
        ],
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "start_gazebo",
                default_value="true",
                description=(
                    "Start Gazebo paused. Set false when "
                    "Gazebo is already running."
                ),
            ),
            gazebo,
            spawn_models,
        ]
    )
