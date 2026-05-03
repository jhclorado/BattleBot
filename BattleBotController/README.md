# BattleBot Controller

ESP32 controller for the BattleBot quadruped robot using ESP-NOW wireless communication.

## Features

- ESP-NOW broadcasting to control the robot wirelessly
- Serial command interface
- Direct angle control for all 8 servos
- Preset positions (stand, sit)
- Basic walking sequence

## Hardware Requirements

- ESP32 microcontroller
- Serial connection (USB or UART)

## Commands

### Direct Angle Control
```
FL:90,45;FR:90,45;BL:90,45;BR:90,45
```
Sets femur and fibula angles for each leg:
- FL: Front Left (femur, fibula)
- FR: Front Right
- BL: Back Left
- BR: Back Right

### Preset Commands
- `stand` - Stand up position
- `sit` - Sit down position
- `walk` - Start walking sequence
- `stop` - Stop current sequence

## Usage

1. Upload this code to a separate ESP32
2. Open Serial Monitor at 115200 baud
3. Send commands to control the robot

## Communication

- Uses ESP-NOW on channel 6
- Broadcasts to all devices on the same channel
- Robot must be running the BattleBot firmware

## Example Commands

```
stand
sit
FL:90,45;FR:90,45;BL:90,45;BR:90,45
walk
stop
```