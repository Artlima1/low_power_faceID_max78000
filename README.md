# MAX78000 Risc-V Environment change detector

## Introduction

Our project aims to use the image capture capabilities of the MAX78000_FTHR to detect if there's significant change in the monitored area.

The board will be fixed at the location you want to monitor, when something happens in the  area, the picture will be sent to a PC through UART connection and then uploaded to a cloud storage. The user can get the image from the cloud.

## Requirements

### Hardware Requirements

The project uses MAX78000 series Dual-Core Ultra-Low-Power Microcontroller board. Our program is performed on [MAX78000_FTHR board](https://www.analog.com/media/en/technical-documentation/data-sheets/max78000fthr.pdf).

![Board pic](./images/max78000fthr.jpg)

To upload the firmware for the first time use, you can follow the instructions [here](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#first-time-firmware-updates).

### Software Requirements

The SDK for MAX78000 that can be found in this section: ["How to build, Flash and Debug the example"](#2-how-to-build-flash-and-debug-the-example).  

Python3 installed in PC to support the image transfer and upload tool.  
For the Python terminal, use `pip install -r requirements.txt` to install all dependencies.

You may need to install png and serial and opencv python library on your machine.


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

### 1.  Required Connections  

Connect a USB cable between the PC and the CN1 (USB/PWR) connector of the Feather Board.  

### 2.  How to Build, Flash and Debug the Example  

Please follow the instructions in [Getting Started with the MAX78000FTHR](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#getting-started-with-the-max78000fthr)

If you are using Visual Studio Code as your platform, please follow the instructions  [here](.vscode/readme.md).

### 3. How to Burn the Application  

To burn the program:

1. Connect the USB cable connected to the MAX78000FTHR board.
2. Create an empty text file named 'erase.act' and Drag-and-drop it onto the DAPLINK drive.
3. This should mass erase the flash of the target device.

### 4. How to Visualize the Picture on PC and send to the cloud

To upload the picture to the cloud, first create a Firebase application, get it's SDK file (JSON) and put it in this folder. After that, just change the name of the file and the firebase application link in upload.py

Run:  python grab_image.py <comport>  

Ex:sudo python3 grab_image.py /dev/ttyACM0


## Links to Powerpoint and Youtube Video

[PowerPoint](https://docs.google.com/presentation/d/1iDG8Hwt4incC3QIWbK0QsuGcLuzB3y-jmzu0u0kR7mA/edit?usp=sharing)  
[Video](https://youtu.be/qPhET3jG1A0)
