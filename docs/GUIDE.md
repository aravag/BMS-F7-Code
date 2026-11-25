# BMS H5 firmware: setup and test guide

Follow this top to bottom. It takes you from a fresh Windows laptop to the NUCLEO-H533RE publishing battery data
as a ROS 2 topic on the Jetson.

**What the firmware does:** the Nucleo runs a micro-ROS node called `bms_node`. Over the ST-LINK USB cable it talks to the
micro-ROS agent on the Jetson, and it publishes (fake) battery data on `/bms/battery_state`.

**What you need:**
- Windows laptop
- NUCLEO-H533RE board and a **USB-C data cable**
- SSH access to the Jetson (Ubuntu 22.04, ROS 2 Humble), currently `100.92.239.43` on Tailscale
- The W5500 Ethernet module is **not** needed yet

The work is split into one Git branch per milestone. Each branch builds on the previous one:

| Branch | What you should see |
|---|---|
| `m0-toolchain` | Nothing yet (empty project, proves the build works) |
| `m1-blinky-uart` | LED blinks, text over the serial port |
| `m2-freertos` | Same, but from two FreeRTOS tasks |
| `m3-microros-hello` | `/bms/heartbeat` topic on the Jetson |
| `m4-bms-node` | `/bms/battery_state` topic, on/off command, auto-reconnect |

---

## Part 1: Laptop setup (once)

### 1.1 Install the tools

| Tool | How | Why |
|---|---|---|
| Git for Windows | https://git-scm.com (keep the default options, which include **Git Bash**) | Code, plus Git Bash for the deploy script |
| VS Code | https://code.visualstudio.com | Editor |
| STM32CubeIDE **1.18.0** | st.com. Install to the **default folder** `C:\ST\STM32CubeIDE_1.18.0` | We only use the compiler and programmer inside it. You never open the IDE. |
| CMake + Ninja | In PowerShell: `winget install Kitware.CMake` then `winget install Ninja-build.Ninja` | Build system |
| Docker Desktop | https://www.docker.com/products/docker-desktop | Builds the micro-ROS library |
| Tailscale | https://tailscale.com/download, then sign in to the team's tailnet | Reaches the Jetson |
| STM32CubeMX (optional) | st.com | Only needed if you change pins or peripherals |

After installing, **close and reopen** PowerShell and VS Code so they pick up the new tools.

> If you installed CubeIDE somewhere else or a different version, edit the `$ide = ...` line in `scripts/env.ps1` to match.

### 1.2 Get the code
```powershell
cd $HOME\Documents
git clone https://github.com/aravag/BMS-F7-Code.git
cd BMS-F7-Code
git checkout m4-bms-node
git submodule update --init
```

### 1.3 Let PowerShell run the project scripts (once)
```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```
Answer `Y` if it asks.

### 1.4 Build the micro-ROS library (once)
1. Start **Docker Desktop** and wait until it says it's running.
2. Run this. It takes a few minutes the first time because it downloads the builder image:
   ```powershell
   .\scripts\build_microros.ps1
   ```
3. Check it worked. This file should exist and be about 10 MB:
   `bms_h5\micro_ros_stm32cubemx_utils\microros_static_library\libmicroros\libmicroros.a`

You don't need to redo this when you switch branches. It stays on disk and isn't committed to Git.

### 1.5 Build the firmware
```powershell
.\scripts\build.ps1
```
It should end with a memory table and no errors:
```
RAM:   ~149 KB / 272 KB
FLASH: ~126 KB / 512 KB
```
The output is `bms_h5\build\Debug\bms_h5.elf` and `bms_h5.bin`. In VS Code, **Ctrl+Shift+B** does the same thing.

---

## Part 2: Jetson setup (once)

From the laptop, with Tailscale connected:
```bash
tailscale ping 100.92.239.43
ssh <user>@100.92.239.43
```
Then, **on the Jetson**:
```bash
sudo apt update && sudo apt install -y screen       # serial monitor
docker --version                                    # Docker should already be installed
sudo usermod -aG docker $USER                       # lets you run docker without sudo; then log out and back in
docker pull microros/micro-ros-agent:humble         # the micro-ROS agent
source /opt/ros/humble/setup.bash && ros2 --help | head -3   # confirms ROS 2 Humble works
```

---

## Part 3: Plug in the board

1. Connect the Nucleo's **ST-LINK USB-C port** (at the top of the board, next to the ST-LINK chip) to the **Jetson**.
   This one cable carries power, flashing and the serial port.
2. On the Jetson, check it's seen:
   ```bash
   lsblk -o NAME,LABEL,MOUNTPOINT | grep -i nod    # expect a drive labelled NOD_H533RE
   ls /dev/ttyACM*                                 # expect /dev/ttyACM0
   ```
   If the label isn't `NOD_H533RE`, put the real label into `scripts/deploy_jetson.sh` (the `by-label/NOD_H533RE` part).

---

## Part 4: Flash a branch (repeat for each milestone)

On the laptop:
```powershell
git checkout <branch>          # e.g. m1-blinky-uart
.\scripts\build.ps1
```
Then in **Git Bash** (not PowerShell), from the repo folder:
```bash
JETSON=<user>@100.92.239.43 ./scripts/deploy_jetson.sh
```
This copies `bms_h5.bin` to the Jetson and onto the Nucleo's USB drive, which flashes the board.
Expect `Done.` at the end. The ST-LINK LED flickers while it programs, then the board restarts with the new code.
It may ask for the Jetson's sudo password.

**Alternative:** plug the Nucleo into the **laptop** instead and run `.\scripts\flash_local.ps1`.

---

## Part 5: Test each milestone, in order

### m1-blinky-uart
- The green LED (LD2) toggles every 0.5 s.
- On the Jetson: `screen /dev/ttyACM0 115200` shows `BMS H5 alive 0, 1, 2...` once per second.
  To quit screen: **Ctrl+A**, then **K**, then **Y**.

### m2-freertos
- LD2 toggles faster, every 0.25 s.
- `screen` shows `BMS H5 alive N (FreeRTOS)` once per second.

### m3-microros-hello
From now on the serial port belongs to micro-ROS, so **don't open `screen`**.

Jetson terminal 1, the agent (leave it running):
```bash
docker run -it --rm --net=host --device=/dev/ttyACM0 microros/micro-ros-agent:humble serial --dev /dev/ttyACM0 -b 115200 -v4
```
You should see `session established` within a few seconds.

Jetson terminal 2:
```bash
source /opt/ros/humble/setup.bash
ros2 node list                    # /bms_node
ros2 topic echo /bms/heartbeat    # data: 1, 2, 3 ... once per second
```
LD2: **solid** means waiting for the agent. **Toggling every second** means it's publishing.
If nothing appears, press the Nucleo's black **RESET** button once with the agent running.

### m4-bms-node (the deliverable)
Start the agent as in m3. Then in terminal 2:
```bash
source /opt/ros/humble/setup.bash
ros2 node list                                  # /bms_node
ros2 topic list                                 # /bms/battery_state and /bms/cmd_output_enable
ros2 topic echo /bms/battery_state              # voltage 16.8 V falling to 14.0 V over ~1 min, 4 cell voltages
ros2 topic hz /bms/battery_state                # about 1.0 Hz
ros2 topic pub --once /bms/cmd_output_enable std_msgs/msg/Bool "{data: false}"   # LD2 turns off
ros2 topic pub --once /bms/cmd_output_enable std_msgs/msg/Bool "{data: true}"    # LD2 turns on
```
**Reconnect test:** stop the agent with Ctrl+C. Within about a second LD2 starts slow-blinking. Start the agent again,
and LD2 goes steady and the topic comes back **without** pressing reset.

LD2 in m4:

| LED | Meaning |
|---|---|
| Slow blink (0.5 s) | Looking for the agent |
| Steady on or off | Connected. On/off shows the last `cmd_output_enable` value (starts on) |
| Fast blink (0.1 s) | micro-ROS setup failed and is retrying |

The battery data is simulated. The real MAX17205 driver will later replace `bms_h5/App/battery_source_fake.c`.

---

## Part 6: Troubleshooting

| Problem | Fix |
|---|---|
| `build.ps1` "cannot be loaded because running scripts is disabled" | Do step 1.3 |
| `build.ps1` can't find `arm-none-eabi-gcc` | CubeIDE isn't at `C:\ST\STM32CubeIDE_1.18.0`. Fix the path in `scripts/env.ps1` |
| `build.ps1` can't find `cmake` or `ninja` | Reopen PowerShell after installing them (step 1.1) |
| Build error: `libmicroros.a` or `rcl/rcl.h` not found | Do step 1.4 (and `git submodule update --init`) |
| `build_microros.ps1` "failed to connect to the docker API" | Start Docker Desktop and wait for it |
| Deploy: "Nucleo drive NOD_H533RE not found" | Wrong USB port (use the ST-LINK one), a charge-only cable, or a different drive label (Part 3) |
| `FAIL.TXT` appears on the Nucleo drive | `cat` it: it says why programming failed. Unplug and replug the board, then deploy again |
| Agent: "Device or resource busy" | Something else has the port open. Close `screen` |
| `docker: permission denied` on the Jetson | Log out and back in after `usermod`, or put `sudo` in front |
| m3/m4: `ros2 node list` is empty | Make sure the agent shows `session established`. Run `source /opt/ros/humble/setup.bash` in the second terminal. Press RESET once. |
| m4: LD2 fast-blinks | micro-ROS couldn't create its entities, probably not enough FreeRTOS heap. Tell Arav |

---

## Part 7: Where things live

| Path | What |
|---|---|
| `bms_h5/bms_h5.ioc` | CubeMX project (pins, clocks, peripherals). Open it in CubeMX to change hardware settings, then click GENERATE CODE |
| `bms_h5/App/` | Our code: `app.c` (tasks), `ros_node.c` (micro-ROS), `bms_data.c` (shared battery data), `battery_source_fake.c` (fake battery) |
| `bms_h5/Core/`, `Drivers/`, `Middlewares/` | Generated by CubeMX. Only edit inside `USER CODE BEGIN/END` blocks |
| `bms_h5/micro_ros_stm32cubemx_utils/` | Official micro-ROS helper repo (Git submodule) |
| `scripts/` | `build.ps1`, `flash_local.ps1`, `deploy_jetson.sh`, `build_microros.ps1` |
| `docs/reference/h5-latest.ioc` | The custom BMS board's CubeMX file (STM32H523). Reference only, not built |
