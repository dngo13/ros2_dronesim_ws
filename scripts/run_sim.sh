#!/usr/bin/env bash
# Launches the ArduCopter SITL + Gazebo simulation.
# Extra args are forwarded to `ros2 launch`, e.g.:
#   scripts/run_sim.sh out:=172.20.64.1:14550
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/.."

source ~/venv-ardupilot/bin/activate
source /opt/ros/jazzy/setup.bash
source install/setup.bash

export GZ_VERSION=harmonic
export GZ_SIM_RESOURCE_PATH="$PWD/install/ardupilot_gazebo/share:${GZ_SIM_RESOURCE_PATH:-}"
export SDF_PATH="$PWD/install/ardupilot_gazebo/share:${SDF_PATH:-}"

exec ros2 launch ardupilot_gz_bringup iris_runway.launch.py rviz:=false gz_args:=-r on_exit_shutdown:=true "$@"
