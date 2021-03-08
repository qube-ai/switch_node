# Baby Qube

## TODO

  1. Changing Mesh name and password. Otherwise homes that are side by side will share the data automatically. Which is wrong. We need to create unique mesh names and passwords
  2. Ability to change the priority of physical switch or cloud message.
  3. Add ability to perform an OTA update from master.
  4. Change GPIO0 to TX and GPIO2 to RX
  5. Implement software serial on GPIO0 and GPIO2 with preprocessor definitions to remove serial statements from release code. (Create a release build and a debug build).

## Getting Started

  - Clone the project
  - Open the project in VS Code (with Platform IO extension installed)
  - To run the project select `release_esp01_1m` environment
  - Build and upload the code to ESP01 device

## Hardware pin out
![Image of ESP01s pin description](media/esp01s-pinout.jpg)

![Image of FTDI USB to TTL pinout](media/ftdi-pinout.jpeg)

## Programming ESP01 using FTDI

  - Connect `CH_PD` from ESP01 to VCC
  - Connect VCC and GND to FTDI VCC and GND
  - Connect `TX` of ESP01 to `RX` of FTDI
  - Connect `RX` of ESP01 to `TX` of FTDI
  - Connect `GPIO0` on ESP01 to GND (GPIO0 low puts ESP8266 into bootloader mode for downloading code)
![Programming ESP01](media/programming-esp01.png)

To reset the ESP, connect `RST` to gronud for a brief moment. The chip will automatically restart.

## Message Types

  - **Type 1 (Node -> Master)** : Whenever the state of node changes. The device generates the message and sends it to the master. Example message:
    ```
    {
      "t": 1,   -> 't' stands for type of message
      "rs": 1,  -> 'rs' stands for relay state (1 - ON, 0 - Off)
      "ss": 0   -> 'ss' starnds for switch state (1 - ON, 0 - Off)
    }
    ```

  - **Type 2 (Master -> Node)** : When master instructs to change the state of relay in a baby qube. Example message
    ```
    {
      "t": 2,   -> 't' stands for type of message
      "rs": 0,  -> Relay would be set to this state (1 - ON, 0 - Off)
    }
    ```

  - **TODO : Type 3 (Master -> Node)** : Change priority between cloud and physical switch. The priority is as follows:
    - `0` : Follow physical swtich (ignore cloud)
    - `1` : Follow physical switch byut also accept cloud commands
    - `2` : Ignore physical switch but accept cloud commands
    - `3` : Ignore physical switch and cloud commands
   Example message
    ```
    {
      "t": 3,   -> 't' stands for type of message
      "p": 0,  -> The priority in which the device should exist
    }
    ```

## Debugging

  - **If the board keeps restarting without any reason.** -> Power ESP01 using a dedicated power supply. Use FTDI power supply **ONLY** for programming the board and for nothing else. If you use FTDI's power supply it would keep restarting with some error random error every now and then.