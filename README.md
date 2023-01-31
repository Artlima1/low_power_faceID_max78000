# MAX78000 Risc-V Mini Closed-Circuit Television

## Introduction

Our project aims to use the image capture capabilities of the MAX78000_FTHR to detect if there's an unintended person enters the monitered area.

The board will be fixed at the location you want to monitor, the cable is connected directly to the security room, when someone enters the monitoring area, the security room will receive the image through UART connection.

## Requirements

### Hardware Requirements

The projrct uses MAX78000 series Dual-Core Ultra-Low-Power Microcontroller board. Our program is performed on [MAX78000_FTHR board](https://www.analog.com/media/en/technical-documentation/data-sheets/max78000fthr.pdf).

![Board pic](https://www.mouser.it/images/maxim/lrg/MAX78000FTHR_DSL.jpg)

To upload the firmware for the first time use, you can follow the instruction [here](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#first-time-firmware-updates).

### Software Requirements

No special software requirements. For detailed operational steps please refer to ["How to build, Flash and Debug the example"](#how-to-build-flash-and-debug-the-example)

## Project Layout  

    ├─ include
    │  ├─ img_capture.h
    │  ├─ linked_list.h
    │  ├─ MAXCAM_Debug.h
    │  └─ utils.h
    ├─ main.c           #ARM core main code
    ├─ main_riscv.c     #Risc_v core main code
    ├─ Makefile         
    ├─ Makefile.ARM     #ARM core makefile
    ├─ Makefile.RISCV   #Risc_v core makefile
    ├─ pc_utility       #Python console on PC
    │  ├─ comManager.py
    │  ├─ grab_image.py
    │  ├─ imgConverter.py
    │  ├─ README.md
    │  └─ requirements.txt
    ├─ README.md
    ├─ risc_v_img_capture.launch
    └─ src              #Source code of project
    ├─ img_capture.c
    ├─ linked_list.c
    └─ utils.c

## Getting Started  

### Required Connections  

• Connect a USB cable between the PC and the CN1 (USB/PWR) connector of the Feather Board and another USB cable between PC and ESP8266.  
• Connect the UART2 pins of the Feather Board with the UART0 pins of the ESP8266 Board.  
• Connect the grounds of two boards.  
• Open a terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.

### How to Build, Flash and Debug the Example  

Please follow the instructions in [Getting Started with the MAX78000FTHR](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#getting-started-with-the-max78000fthr)

If you are using Visual Studio Code as your platform, please follow the instructions  [here](.vscode/readme.md).

### How to Burn the Application  

To burn the program:

1. Connect the USB cable connected to the MAX78000FTHR board.
2. Create an empty text file named 'erase.act' and Drag-and-drop it onto the DAPLINK drive.
3. This should mass erase the flash of the target device.

### How to Visualize the Picture on PC  

Please follow the instructions [here](./pc_utility/README.md).

## Links to Powerpoint and Youtube Video

[PowerPoint](https://docs.google.com/presentation/d/1iDG8Hwt4incC3QIWbK0QsuGcLuzB3y-jmzu0u0kR7mA/edit?usp=sharing)  
[Video]
