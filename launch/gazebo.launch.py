from launch import LaunchContext, LaunchDescription

from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration, TextSubstitution

from launch_ros.substitutions import FindPackageShare


def launch_setup(context: LaunchContext):
    # Initialize arguments
    world = LaunchConfiguration('world')
    use_sim_time = LaunchConfiguration('use_sim_time')
    verbose = LaunchConfiguration('verbose')

    world_name = world.perform(context) + '.world'

    gzserver_launcher = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([FindPackageShare('gazebo_ros'), 'launch', 'gzserver.launch.py'])),
        launch_arguments={'use_sim_time': use_sim_time,
                          'world': PathJoinSubstitution([FindPackageShare('laser_gazebo_resources'),
                                                    'worlds', TextSubstitution(text=world_name)]),
                          'verbose': verbose}.items())

    gzclient_launcher = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([FindPackageShare('gazebo_ros'), 'launch', 'gzclient.launch.py'])),
        launch_arguments={'use_sim_time': use_sim_time}.items())

    return [gzserver_launcher, gzclient_launcher]


def generate_launch_description():
    # Declare arguments
    declared_arguments = []

    declared_arguments.append(
        DeclareLaunchArgument(
            'world',
            default_value='custom_empty',
            description='Name of the world file.'))

    declared_arguments.append(
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation Gazebo clock.'))

    declared_arguments.append(
        DeclareLaunchArgument(
            'verbose',
            default_value='true',
            description='Increase messages written to terminal.'))

    return LaunchDescription(declared_arguments + [OpaqueFunction(function=launch_setup)])
