# Test day checklist (NUCLEO-H533RE plugged into the Jetson)

Plug the Nucleo's **ST-LINK USB-C port** (the one at the top, next to the ST-LINK chip) into the Jetson.
That one cable does three things: power, flashing (the board appears as a USB drive), and the serial port `/dev/ttyACM0`.

## Before you go (tonight)
```bash
tailscale ping 100.92.239.43
ssh <user>@100.92.239.43
which screen  || sudo apt install -y screen      # serial monitor
docker --version && docker pull microros/micro-ros-agent:humble
source /opt/ros/humble/setup.bash && ros2 --help | head -3
```

## Step 0: check the board is seen
On the Jetson, with the Nucleo plugged in:
```bash
lsblk -o NAME,LABEL,MOUNTPOINT | grep -i nod    # expect NOD_H533RE
ls /dev/ttyACM*                                 # usually /dev/ttyACM0
```
If the drive label is different (e.g. not `NOD_H533RE`), edit the label in `scripts/deploy_jetson.sh`.

## Flashing any branch (from the laptop)
```bash
git checkout <branch>
.\scripts\build.ps1                                        # PowerShell
JETSON=<user>@100.92.239.43 ./scripts/deploy_jetson.sh     # Git Bash
```
The ST-LINK LED flickers while it programs, then the board resets and runs the new code.
If the drive shows a `FAIL.TXT` afterwards, read it (`cat /mnt/nucleo/FAIL.TXT` or the mount point shown).

## m1-blinky-uart
- The green LED (LD2) toggles every 0.5 s.
- On the Jetson: `screen /dev/ttyACM0 115200` shows `BMS H5 alive 0`, `1`, `2`, ... once per second.
  (Quit screen: Ctrl+A then K then Y.)
- [ ] Pass

## m2-freertos
- LD2 toggles every 0.25 s (faster than m1), driven by one FreeRTOS task.
- `screen /dev/ttyACM0 115200` shows `BMS H5 alive N (FreeRTOS)` once per second, from a second task.
- [ ] Pass

## m3-microros-hello
From here on, USART2 carries micro-ROS traffic, so **do not open `screen`** on `/dev/ttyACM0` (it would steal the port from the agent).
LD2: **solid on** = waiting for the agent, **toggling every 1 s** = publishing, **fast blink** = micro-ROS setup failed.

Terminal 1 on the Jetson (agent):
```bash
docker run -it --rm --net=host --device=/dev/ttyACM0 microros/micro-ros-agent:humble serial --dev /dev/ttyACM0 -b 115200 -v4
```
You should see `session established` within a few seconds, and LD2 starts toggling.

Terminal 2 on the Jetson:
```bash
source /opt/ros/humble/setup.bash
ros2 node list                    # shows /bms_node
ros2 topic echo /bms/heartbeat    # data: 1, 2, 3 ... once per second
```
If nothing shows, press the Nucleo's black RESET button once with the agent already running
(m3 has no reconnect logic; m4 does).
- [ ] Pass

## m4-bms-node (the deliverable)
LD2: **slow blink** (0.5 s) = looking for the agent; **fast blink** (0.1 s) = micro-ROS setup failed, retrying;
**connected** = LED shows the output-enable command (starts **on**).

Start the agent as in m3. Then in a second terminal:
```bash
source /opt/ros/humble/setup.bash
ros2 node list                                  # /bms_node
ros2 topic list                                 # /bms/battery_state, /bms/cmd_output_enable
ros2 topic echo /bms/battery_state              # voltage ~16.8 -> 14.0 V over ~1 min, 4 cell_voltage values, percentage falling
ros2 topic hz /bms/battery_state                # ~1.0 Hz
ros2 topic pub --once /bms/cmd_output_enable std_msgs/msg/Bool "{data: false}"   # LD2 off
ros2 topic pub --once /bms/cmd_output_enable std_msgs/msg/Bool "{data: true}"    # LD2 on
```
Reconnect test: Ctrl+C the agent → within ~1 s LD2 starts slow-blinking. Start the agent again → LD2 goes steady
and `ros2 topic echo` resumes, **without** pressing reset.

The battery data is **fake** (simulated 4S pack, 5 A discharge, sped up 60x). The real MAX17205 replaces
`bms_h5/App/battery_source_fake.c` later; nothing else changes.
- [ ] Pass
