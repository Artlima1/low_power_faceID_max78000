# MAX78000 Risc-V Home Security System
## Introduction
Our project aims to use the image capture capabilities of the MAX78000_FTHR board and the wireless transmission capabilities of the ESP8266 board to detect if there's an unintended person enters the room when the user is not at home.

The whole product will be placed in the user's room, when there are changes in the scene detected by the camera, the MAX78000 will take a picture and transmit it to the ESP8266 via a UART connection, after which the image will be sent to a server using a HTTP request from the ESP8266. So that the user can be alerted.
## Requirements
### Hardware Requirements
The projrct uses MAX78000 series Dual-Core Ultra-Low-Power Microcontroller board. Our program is performed on [MAX78000_FTHR board](https://www.analog.com/media/en/technical-documentation/data-sheets/max78000fthr.pdf).

![Board pic](https://www.analog.com/-/media/analog/en/evaluation-board-maxim-images/max78000fthr.jpg?imgver=1&h=270&hash=290709D8D68B29A194FE60CCF35E6F35)

To upload the firmware for the first time use, you can follow the instruction [here](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#first-time-firmware-updates).

Also a ESP Wi-Fi Development Board is required,here we use the [ESP8266 board](https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf). 
![ESP Board Pic](https://cdn.ecommercedns.uk/files/3/206443/1/7026261/nodemcu-r-1024.jpg)

### Software Requirements

## Project Layout  

## Getting Started

### 1. Required Connections  
• Connect a USB cable between the PC and the CN1 (USB/PWR) connector of the Feather Board and another USB cable between PC and ESP8266.  
• Connect the UART2 pins of the Feather Board with the UART0 pins of the ESP8266 Board.  
• Connect the grounds of two boards.  
• Open a terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.

### 2. How to Build, Flash and Debug the Example  
Please follow the instructions in [Getting Started with the MAX78000FTHR](https://github.com/MaximIntegratedAI/MaximAI_Documentation/blob/master/MAX78000_Feather/README.md#getting-started-with-the-max78000fthr)

If you are using Visual Studio Code as your platform, please follow the instructions  [here](.vscode/readme.md).
### 3. How to Burn the Application  
To burn the program:

1. Connect the USB cable connected to the MAX78000FTHR board.
2. Create an empty text file named 'erase.act' and Drag-and-drop it onto the DAPLINK drive.
3. This should mass erase the flash of the target device.

### 4. How to Visualize the Picture on PC  
Please follow the instructions [here](.utils/__pycache__/README.md)

## Links to Powerpoint and Youtube Video







