import os

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, LogInfo
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    lus_simulation_path = get_package_share_path('lus_simulation')
    scripts_path = os.path.join(lus_simulation_path, 'scripts')

    # Declare the launch argument
    model_name_arg = DeclareLaunchArgument(
        'model_name',
        description='drone model name'
    )
    namespace_arg = DeclareLaunchArgument(
        'namespace',
        description='drone namespace'
    )

    model_name = LaunchConfiguration('model_name')
    namespace = LaunchConfiguration('namespace')

    jinja_gen = ExecuteProcess(
        cmd=['python3', os.path.join(scripts_path, 'jinja_gen.py'), model_name, namespace],
        output='screen'
    )
    
    link_models = ExecuteProcess(
    	cmd=['bash', os.path.join(scripts_path, 'link_models.sh')],
    	output='screen'
    )

    return LaunchDescription([
        model_name_arg ,
        namespace_arg ,
        jinja_gen ,
        link_models ,
    ]) 
