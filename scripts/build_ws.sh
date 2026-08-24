#!/usr/bin/env bash
# Builds the workspace after pulling changes or editing a package.
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/.."

source ~/venv-ardupilot/bin/activate
source /opt/ros/jazzy/setup.bash

export JAVA_HOME="$HOME/.local/opt/temurin17"
export PATH="$PWD/tools/Micro-XRCE-DDS-Gen/scripts:$PATH"
export GZ_VERSION=harmonic

colcon build --symlink-install
