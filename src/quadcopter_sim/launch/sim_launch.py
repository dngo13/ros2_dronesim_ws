import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

def generate_launch_description():
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')
    pkg_my_sim = get_package_share_directory('quadcopter_sim')

    # Path to our custom SDF world
    world_file = os.path.join(pkg_my_sim, 'worlds', 'drone_gimbal.sdf')

    # Include the Gazebo launch file
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        # The '-r' flag runs the simulation immediately on startup
        launch_arguments={'gz_args': f'-r {world_file}'}.items(),
    )

    return LaunchDescription([
        gazebo
    ])