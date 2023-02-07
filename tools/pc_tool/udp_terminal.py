import socket
from datetime import datetime
import imgConverter
import upload

localIP     = "192.168.223.206"
localPort   = 60996
bufferSize  = 1024

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))
print("UDP server up and listening")

img_len = 0
w = 0
h = 0
pixelformat = 0

# Listen for incoming datagrams
while(True):

    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)

    message = bytesAddressPair[0]

    address = bytesAddressPair[1]

    if (img_len==0) and (message[0]==1):
        img_len = message[4]*256*256*256 + message[3]*256*256 + message[2]*256 + message[1]
        w = message[6]*256 + message[5]
        h = message[8]*256 + message[7]
        pixelformat = message[9]
        print("Img command", img_len, w, h, pixelformat)
        image = bytearray()
    elif (img_len>0):
        image += message
        if (img_len == len(image)):
            img_name = datetime.now().strftime("%d.%m.%Y-%H:%M:%S")
            img_name += ".png"
            imgConverter.convert(image, img_name, w, h, "RGB565")
            print("Finished getting img")
            img_len = 0
            upload.upload_to_firebase(img_name)


