# ROS 2 PX4 Gimbal Drone Simulation

This repository is a ROS 2 workspace for testing a quadcopter-mounted gimbal
and scanning algorithms in Gazebo. The baseline stack is:

- Ubuntu 24.04, either native Linux or Windows through WSL2
- ROS 2 Jazzy
- Gazebo Harmonic
- PX4 v1.17.0 Software-In-The-Loop (SITL)
- PX4's `x500_gimbal` quadcopter model
- Micro XRCE-DDS for PX4-to-ROS 2 communication

PX4 is used rather than ArduPilot because this workspace already targets ROS 2
Jazzy and PX4 provides a supported Gazebo gimbal vehicle and windy world for
this platform. Do not add ArduPilot packages to this workspace; it is a
different flight stack with different ROS interfaces and launch tooling.

## Project status

The repository currently provides:

- pinned Git submodules for PX4, `px4_msgs`, and Micro XRCE-DDS Agent;
- a `quadcopter_sim` ROS package that launches PX4 SITL with the Gazebo
  `x500_gimbal` model;
- selection of the Gazebo world through a launch argument, defaulting to
  `windy`;
- installation and run instructions for native Ubuntu and Windows/WSL2.

It does **not** yet provide:

- a ROS node that commands the gimbal scan trajectory;
- a ROS bridge/configuration for the gimbal camera image;
- stochastic gust or turbulence profiles beyond PX4's baseline windy world;
- scan-quality logging, ground-truth comparison, or performance metrics.

Those are the next implementation milestones after the baseline simulator is
running reliably.

## Architecture

```text
Scanning algorithm (future ROS 2 node)
       |                         |
       | gimbal commands         | image / scan measurements
       v                         v
Gazebo x500 gimbal + sensor <-> PX4 SITL
                                  |
                                  | uXRCE-DDS, UDP 8888
                                  v
                         Micro XRCE-DDS Agent
                                  |
                                  v
                         ROS 2 px4_msgs topics
```

## Repository layout

```text
ros2_dronesim_ws/
├── .gitmodules
├── README.md
├── src/
│   ├── Micro-XRCE-DDS-Agent/   # submodule, pinned to v2.4.3
│   ├── px4_msgs/               # submodule, release/1.17
│   └── quadcopter_sim/         # local ROS launch/config package
└── third_party/
    └── PX4-Autopilot/          # submodule, pinned to v1.17.0
```

PX4 is outside `src/` so colcon does not scan the firmware source tree as a ROS
package.

## Windows with WSL2

Run this section from an Administrator PowerShell terminal:

```powershell
wsl --install -d Ubuntu-24.04
wsl --update
```

Restart if prompted, then enter the Linux environment:

```powershell
wsl -d Ubuntu-24.04
```

WSL may inherit the current PowerShell directory. Always move to the Linux home
directory before cloning:

```bash
cd ~
pwd
```

`pwd` should print `/home/<username>`, not a path beginning with `/mnt/c`.
Clone the workspace and all nested dependencies:

```bash
git clone --recurse-submodules \
  https://github.com/dngo13/ros2_dronesim_ws.git
cd ~/ros2_dronesim_ws
```

Keep the active workspace under `/home/<username>`. Building under `/mnt/c` or
OneDrive can cause slow I/O, permission errors, and broken symlinks.

Open the Linux-native workspace in VS Code with:

```bash
cd ~/ros2_dronesim_ws
code .
```

VS Code should show `WSL: Ubuntu-24.04` in its lower-left corner.

## Native Ubuntu Linux

On a native Ubuntu 24.04 machine, clone directly into your Linux home:

```bash
cd ~
git clone --recurse-submodules \
  https://github.com/dngo13/ros2_dronesim_ws.git
cd ~/ros2_dronesim_ws
```

The remaining installation and run commands are identical for native Ubuntu
and WSL2.

## Initialize submodules in an existing checkout

If the repository was cloned without `--recurse-submodules`, run:

```bash
cd ~/ros2_dronesim_ws
git submodule sync --recursive
git submodule update --init --recursive
```

## Install ROS 2 Jazzy and Gazebo Harmonic

Confirm Ubuntu 24.04:

```bash
source /etc/os-release
echo "$PRETTY_NAME ($VERSION_CODENAME)"
```

The codename must be `noble`.

Enable Ubuntu Universe and install the ROS repository configuration:

```bash
sudo apt update
sudo apt install -y locales software-properties-common curl
sudo add-apt-repository universe

ROS_APT_SOURCE_VERSION=$(curl -s \
  https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest \
  | grep -F '"tag_name"' \
  | awk -F '"' '{print $4}')

curl -L -o /tmp/ros2-apt-source.deb \
  "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.noble_all.deb"

sudo dpkg -i /tmp/ros2-apt-source.deb
sudo apt update
sudo apt upgrade -y
```

Install ROS, Gazebo integration, and development tools:

```bash
sudo apt install -y \
  ros-jazzy-desktop \
  ros-dev-tools \
  ros-jazzy-ros-gz \
  python3-colcon-common-extensions
```

Source ROS in new terminals:

```bash
echo 'source /opt/ros/jazzy/setup.bash' >> ~/.bashrc
source ~/.bashrc
```

## Install the PX4 simulation toolchain

The PX4 source is already pinned in `third_party/PX4-Autopilot`. Install its
Ubuntu simulation dependencies:

```bash
cd ~/ros2_dronesim_ws
bash third_party/PX4-Autopilot/Tools/setup/ubuntu.sh --no-nuttx
```

`--no-nuttx` skips the embedded Pixhawk compiler. Omit it if this computer will
also build firmware for real flight controllers. Restart WSL or log out and
back in after the script finishes.

## Build the ROS workspace

Initialize rosdep once, then build:

```bash
cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
sudo rosdep init
rosdep update
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

If `rosdep init` reports that its sources already exist, continue with
`rosdep update`.

## Run the gimbal drone simulation

Use three terminals.

### Terminal 1: PX4-to-ROS DDS agent

```bash
cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
MicroXRCEAgent udp4 -p 8888
```

### Terminal 2: PX4 x500 gimbal in wind

```bash
cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch quadcopter_sim sim_launch.py
```

The launch defaults to the `windy` Gazebo world. Select another PX4 world with:

```bash
ros2 launch quadcopter_sim sim_launch.py world:=default
```

Run without the Gazebo GUI using:

```bash
HEADLESS=1 ros2 launch quadcopter_sim sim_launch.py
```

### Terminal 3: verify PX4 ROS topics

```bash
cd ~/ros2_dronesim_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 topic list | grep /fmu/
ros2 topic echo /fmu/out/vehicle_odometry
```

If `/fmu/...` topics appear and odometry updates, Gazebo, PX4, DDS, and ROS 2
are connected.

## Development roadmap

1. **Baseline validation:** launch `x500_gimbal`, verify flight and DDS topics,
   enumerate Gazebo gimbal joint and camera topics.
2. **Gimbal scanning node:** generate repeatable yaw/pitch trajectories with
   velocity and acceleration limits, command the simulated gimbal, and record
   actual joint angles.
3. **Sensor pipeline:** bridge the gimbal camera or scanner data into ROS 2 and
   timestamp it against vehicle pose and gimbal pose.
4. **Turbulence model:** replace the baseline windy world with configurable
   mean wind, gust amplitude, direction, bandwidth, and random seed.
5. **Performance evaluation:** compute pointing error, scan coverage,
   stabilization error, settling time, and image/measurement degradation across
   repeatable turbulence profiles.

Use fixed random seeds and configuration files so every algorithm version is
tested against the same disturbances.

## References

- [ROS 2 Jazzy Ubuntu installation](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
- [Gazebo and ROS compatibility](https://gazebosim.org/docs/jetty/ros_installation/)
- [PX4 WSL2 environment](https://docs.px4.io/main/en/dev_setup/dev_env_windows_wsl)
- [PX4 Ubuntu environment](https://docs.px4.io/main/en/dev_setup/dev_env_linux_ubuntu)
- [PX4 Gazebo simulation and x500 gimbal](https://docs.px4.io/v1.17/en/sim_gazebo_gz/)
- [PX4 ROS 2 integration](https://docs.px4.io/main/en/ros2/)
- [PX4 uXRCE-DDS bridge](https://docs.px4.io/main/en/middleware/uxrce_dds)
