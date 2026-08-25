# ArduPilot Gazebo Drone Simulation

This ROS 2 workspace runs ArduCopter Software-In-The-Loop (SITL) with Gazebo
Harmonic. The included launch file uses an Iris drone with a three-axis gimbal
and camera. It is a starting point for a custom hexacopter, turbulence testing,
and a gimbal scanning controller.

The simulation includes:

- ROS 2 Jazzy
- Gazebo Harmonic
- ArduPilot SITL and MAVProxy
- ArduPilot's Gazebo plugin and ROS 2 packages
- Micro-ROS Agent and Micro XRCE-DDS Generator

## Workspace layout

```text
ros2_dronesim_ws/
├── src/
│   ├── ardupilot/
│   ├── ardupilot_gazebo/
│   ├── ardupilot_gz/
│   ├── ardupilot_sitl_models/
│   └── micro_ros_agent/
└── tools/Micro-XRCE-DDS-Gen/
```

These dependencies are pinned Git submodules. Put custom drone models and ROS
nodes in a separate workspace package instead of editing the submodules.

## Windows with WSL2

Run ROS, Gazebo, ArduPilot, and builds inside Ubuntu 24.04. Mission Planner can
run on Windows.

From PowerShell:

```powershell
wsl -d Ubuntu-24.04
```

Keep the repository under `/home`, not `/mnt/c` or OneDrive:

```bash
cd ~/ros2_dronesim_ws
code .
```

VS Code should show `WSL: Ubuntu-24.04` in the lower-left corner. The same
commands also work on native Ubuntu 24.04.

## Clone

```bash
cd ~
git clone --recurse-submodules https://github.com/dngo13/ros2_dronesim_ws.git
cd ~/ros2_dronesim_ws
```

For an existing checkout:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## Install dependencies

Install ROS 2 Jazzy first, then run:

```bash
sudo apt update
sudo apt install -y ros-jazzy-desktop ros-jazzy-ros-gz ros-jazzy-sdformat-urdf gz-harmonic \
  python3-colcon-common-extensions python3-vcstool python3-venv rapidjson-dev libgz-sim8-dev libopencv-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl
```

Install ArduPilot's Ubuntu prerequisites:

```bash
cd ~/ros2_dronesim_ws
git submodule update --init --recursive

cd ~/ros2_dronesim_ws/src/ardupilot
python3 -m venv ~/venv-ardupilot
source ~/venv-ardupilot/bin/activate
DO_AP_STM_ENV=0 DO_PYTHON_VENV_ENV=0 Tools/environment_install/install-prereqs-ubuntu.sh -y
python -m pip install 'setuptools<80'
```

## Build the DDS generator

Install Java 17 locally because the pinned Gradle wrapper is incompatible with
Ubuntu 24.04's default Java 21:

```bash
mkdir -p ~/.local/opt/temurin17
wget 'https://api.adoptium.net/v3/binary/latest/17/ga/linux/x64/jdk/hotspot/normal/eclipse' -O /tmp/temurin17.tar.gz
tar -xzf /tmp/temurin17.tar.gz --strip-components=1 -C ~/.local/opt/temurin17
rm /tmp/temurin17.tar.gz

cd ~/ros2_dronesim_ws/tools/Micro-XRCE-DDS-Gen
JAVA_HOME=$HOME/.local/opt/temurin17 ./gradlew assemble
```

## Install ROS dependencies

Initialize `rosdep` once. If it says the default sources already exist,
continue with the remaining commands.

```bash
sudo rosdep init
sudo wget https://raw.githubusercontent.com/osrf/osrf-rosdep/master/gz/00-gazebo.list -O /etc/ros/rosdep/sources.list.d/00-gazebo.list
rosdep update

cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
export GZ_VERSION=harmonic
rosdep install --from-paths src --ignore-src -r -y
```

## Build the workspace

```bash
cd ~/ros2_dronesim_ws
source ~/venv-ardupilot/bin/activate
source /opt/ros/jazzy/setup.bash
export JAVA_HOME=$HOME/.local/opt/temurin17
export PATH=$PWD/tools/Micro-XRCE-DDS-Gen/scripts:$PATH
export GZ_VERSION=harmonic
colcon build --symlink-install
```

## WSL GPU rendering workaround

If Gazebo is rendering through `llvmpipe` in WSL, add the following to your shell startup so Mesa uses the D3D12 driver path:

```bash
echo 'unset LIBGL_ALWAYS_SOFTWARE' >> ~/.bashrc
echo 'export MESA_LOADER_DRIVER_OVERRIDE=d3d12' >> ~/.bashrc
echo 'export GALLIUM_DRIVER=d3d12' >> ~/.bashrc
echo 'export QT_QPA_PLATFORM=xcb' >> ~/.bashrc
source ~/.bashrc
```

Verify that the renderer changed before launching Gazebo:

```bash
export MESA_LOADER_DRIVER_OVERRIDE=d3d12
export GALLIUM_DRIVER=d3d12
glxinfo -B | grep "OpenGL renderer"
```

The output should no longer show `llvmpipe`.

## Auto-source the simulation environment

Add this once to `~/.bashrc` so every new terminal has the venv, ROS 2,
and the workspace overlay ready without retyping anything:

```bash
cat >> ~/.bashrc <<'EOF'

# ros2_dronesim_ws: source SITL/Gazebo env so `ros2 launch ...` works directly
if [ -f "$HOME/venv-ardupilot/bin/activate" ]; then
    source "$HOME/venv-ardupilot/bin/activate"
fi
if [ -f "$HOME/ros2_dronesim_ws/install/setup.bash" ]; then
    source "$HOME/ros2_dronesim_ws/install/setup.bash"
fi
export GZ_VERSION=harmonic
export GZ_SIM_RESOURCE_PATH="$HOME/ros2_dronesim_ws/install/ardupilot_gazebo/share:${GZ_SIM_RESOURCE_PATH:-}"
export SDF_PATH="$HOME/ros2_dronesim_ws/install/ardupilot_gazebo/share:${SDF_PATH:-}"
EOF
source ~/.bashrc
```

The `if` guards mean a fresh checkout without the venv or a build yet won't
break other terminals — they just skip sourcing until those exist. Re-run
`source ~/.bashrc` (or open a new terminal) after your first `colcon build`
so the workspace overlay picks up.

## Run the simulation

With the environment auto-sourced, starting the simulation is just:

```bash
cd ~/ros2_dronesim_ws
ros2 launch ardupilot_gz_bringup iris_runway.launch.py rviz:=false gz_args:=-r on_exit_shutdown:=true
```

This starts Gazebo, ArduCopter SITL, MAVProxy, Micro-ROS Agent, and the ROS 2
bridges. Stop everything with `Ctrl+C`. Remove `rviz:=false` if you also want
RViz.

If Gazebo reports `Unable to find uri[package://ardupilot_gazebo/...]`, the
`~/.bashrc` block above hasn't been sourced in that terminal — open a new one
or run `source ~/.bashrc`.

## Verify ROS 2

While the simulation is running, open a second WSL terminal:

```bash
cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 topic list
```

Expected topics include `/ap/clock`, `/ap/navsat/navsat0`, `/iris/odometry`,
`/joint_states`, and `/tf`.

## Mission Planner

The launch sends MAVLink UDP traffic to `127.0.0.1:14550`. In Mission Planner
on Windows, select `UDP` and port `14550`.

If Windows does not receive it, run `ip route show default` in WSL and pass the
reported Windows gateway to the launch command, for example:

```bash
ros2 launch ardupilot_gz_bringup iris_runway.launch.py rviz:=false gz_args:=-r out:=172.20.64.1:14550
```

Replace the example IP address with your gateway. Keep the real flight
controller disconnected while using SITL.

## Custom package: drone_autonomy

A C++ package that will eventually hold custom autonomy nodes (starting with a
gimbal controller). For now it just has a launch file that re-launches the
sim, as a first step before adding real nodes.

Created with:

```bash
cd ~/ros2_dronesim_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 --dependencies rclcpp --node-name gimbal_controller_node drone_autonomy
```

- `--build-type ament_cmake` — C++ package (`ament_python` would be for Python).
- `--dependencies rclcpp` — the C++ client library any ROS 2 C++ node needs.
- `--node-name gimbal_controller_node` — also scaffolds a starter `.cpp` file
  and wires it into `CMakeLists.txt` automatically. Not used yet.

### Launch file

`ros2 pkg create` doesn't generate a `launch/` folder or add a launch-file
flag, so it's added by hand: `src/drone_autonomy/launch/drone_autonomy_launch.py`,
which re-uses the existing `iris_runway.launch.py` instead of duplicating it:

```python
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare('ardupilot_gz_bringup'),
                'launch',
                'iris_runway.launch.py',
            ])
        )
    )

    return LaunchDescription([sim])
```

Building blocks used, simplified:

| Line | What it does |
|---|---|
| `LaunchDescription([...])` | The list of everything this launch file starts. |
| `IncludeLaunchDescription(...)` | "Also run this other launch file" — how launch files nest. |
| `PythonLaunchDescriptionSource(...)` | Says the included file is a `.py` launch file. |
| `FindPackageShare('ardupilot_gz_bringup')` | Finds where that package's installed files live, so this file can reach into it. |
| `PathJoinSubstitution([...])` | Joins path pieces (package share dir + `launch/` + filename) into one path. |

Also needed — `colcon build` only installs files it's told to, so
`CMakeLists.txt` needs this added before `ament_package()`, or the launch file
won't exist in `install/` and `ros2 launch` can't find it:

```cmake
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

Build just this package and run it:

```bash
colcon build --packages-select drone_autonomy --symlink-install
source ~/.bashrc
ros2 launch drone_autonomy drone_autonomy_launch.py
```

## Gimbal and future work

The current Iris model includes a camera and maps gimbal roll, pitch, and yaw
to RC channels 6, 7, and 8. Suggested next steps are:

1. Fly the supplied drone using SITL and Mission Planner.
2. Test the gimbal axes and camera topics.
3. Create a separate hexacopter model package.
4. Add configurable wind and turbulence.
5. Implement the scanning controller and record performance metrics.

## References

- [ArduPilot source](https://github.com/ArduPilot/ardupilot)
- [ArduPilot ROS 2 and Gazebo](https://ardupilot.org/dev/docs/ros2-gazebo.html)
- [ArduPilot Gazebo plugin](https://github.com/ArduPilot/ardupilot_gazebo)
- [ArduPilot SITL models](https://github.com/ArduPilot/SITL_Models)
- [Gazebo Harmonic](https://gazebosim.org/docs/harmonic/getstarted/)

--------
## Creating new package for drone autonomy
```bash
cd ~/ros2_dronesim_ws/src
ros2 pkg create --build-type ament_cmake --license Apache-2.0 --dependencies rclcpp --node-name gimbal_controller_node drone_autonomy
```
- `ament_cmake` - tells that is a C++ package 
- `rclcpp` - the C++ client library that allows communication between the program and ROS2 functions
- `node-name gimbal_controller_node` - creates the C++ starter file for the node 