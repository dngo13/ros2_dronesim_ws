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
cd ~/ros2_dronesim_ws/src/ardupilot
DO_AP_STM_ENV=0 DO_PYTHON_VENV_ENV=0 Tools/environment_install/install-prereqs-ubuntu.sh -y
source ~/venv-ardupilot/bin/activate
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

## Run the simulation

Run this complete block from WSL. The resource-path exports are required for
the nested gimbal models to load.

```bash
cd ~/ros2_dronesim_ws
source ~/venv-ardupilot/bin/activate
source /opt/ros/jazzy/setup.bash
source install/setup.bash
export GZ_VERSION=harmonic
export GZ_SIM_RESOURCE_PATH=$PWD/install/ardupilot_gazebo/share:${GZ_SIM_RESOURCE_PATH:-}
export SDF_PATH=$PWD/install/ardupilot_gazebo/share:${SDF_PATH:-}

ros2 launch ardupilot_gz_bringup iris_runway.launch.py rviz:=false gz_args:=-r on_exit_shutdown:=true
```

This starts Gazebo, ArduCopter SITL, MAVProxy, Micro-ROS Agent, and the ROS 2
bridges. Stop everything with `Ctrl+C`. Remove `rviz:=false` if you also want
RViz.

If Gazebo reports `Unable to find uri[package://ardupilot_gazebo/...]`, the
resource-path exports above were not set in that terminal.

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
