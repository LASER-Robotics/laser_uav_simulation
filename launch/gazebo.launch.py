import launch

from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, TextSubstitution

from launch_ros.substitutions import FindPackageShare

import os

def launch_setup(context: launch.LaunchContext, ld):

    # #{ world

    world = LaunchConfiguration('world')
    _world = world.perform(context) + '.world'

    # #}

    # #{ real_time

    real_time = LaunchConfiguration('real_time')
    os.environ['PX4_SIM_SPEED_FACTOR'] = real_time.perform(context)

    # #}

    # #{ verbose

    verbose = LaunchConfiguration('verbose')

    ld.add_action(DeclareLaunchArgument(
        'verbose',
        default_value='true',
        description='Increase messages written to terminal.'
    ))

    # #}

    # #{ gazebo_config

    gazebo_config = LaunchConfiguration('gazebo_config')

    ld.add_action(DeclareLaunchArgument(
        'gazebo_config',
        default_value=PathJoinSubstitution([
            FindPackageShare('laser_uav_simulation'),
            'config',
            'gazebo.yaml'
        ]),
        description='Path to the Gazebo configuration file.'
    ))

    # #}

    # #{ gazebo server launcher

    ld.add_action(
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                FindPackageShare('gazebo_ros'), '/launch/gzserver.launch.py'
            ]),
            launch_arguments={
                'verbose': verbose,
                'world': PathJoinSubstitution([
                    FindPackageShare('laser_gazebo_resources'),
                    'worlds',
                    TextSubstitution(text=_world)
                ]),
                'params_file': gazebo_config,
            }.items()
        )
    )

    # #}

    # #{ gazebo client launcher

    ld.add_action(
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                FindPackageShare('gazebo_ros'), '/launch/gzclient.launch.py'
            ])
        )
    )

    # #}


def generate_launch_description():
    ld = launch.LaunchDescription()

    # #{ world

    ld.add_action(DeclareLaunchArgument(
        'world',
        default_value='custom_empty',
        description='Name of the world file.'
    ))

    # #}

    # #{ real_time

    ld.add_action(DeclareLaunchArgument(
        'real_time',
        default_value='1.0',
        description='Real time param for running simulator.'
    ))

    # #}

    # #{ opaque function

    ld.add_action(
        OpaqueFunction(function=launch_setup, args=[ld])
    )

    # #}

    return ld
