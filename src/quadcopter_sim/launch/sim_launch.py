"""Launch PX4 SITL with Gazebo's x500 gimbal model."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import EnvironmentVariable, LaunchConfiguration
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    px4_dir = LaunchConfiguration("px4_dir")
    world = LaunchConfiguration("world")

    default_px4_dir = PathJoinSubstitution(
        [
            EnvironmentVariable("HOME"),
            "ros2_dronesim_ws",
            "third_party",
            "PX4-Autopilot",
        ]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "px4_dir",
                default_value=default_px4_dir,
                description="Path to the PX4-Autopilot submodule.",
            ),
            DeclareLaunchArgument(
                "world",
                default_value="windy",
                description="PX4 Gazebo world name, such as windy or default.",
            ),
            ExecuteProcess(
                cmd=["make", "px4_sitl", "gz_x500_gimbal"],
                cwd=px4_dir,
                additional_env={"PX4_GZ_WORLD": world},
                output="screen",
            ),
        ]
    )
