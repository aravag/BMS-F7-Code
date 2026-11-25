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
