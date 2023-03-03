#!/usr/bin/env python3

#import cv2
import socket
#import numpy as np

PROTOCOL_HEADER_SIZE=48


class NSLS2TomoStreamProtocolHeader:

    def __init__(self, raw_header_data):
        self.protocol_version = f'{int(raw_header_data[0])}.{raw_header_data[1]}.{raw_header_data[2]}'
        self.frame_type = raw_header_data[3]
        self.reference_type = raw_header_data[4]
        self.reference = raw_header_data[5:13]
        self.num_bytes = raw_header_data[14:22]
        self.x_size = int.from_bytes(raw_header_data[23:31], "little")
        self.y_size = int.from_bytes(raw_header_data[32:39], "little")
        self.color_channels = raw_header_data[40:47]

    def __str__(self):
        to_str = f'NSLS2 Tomo Protocol Header {self.protocol_version}\n'
        to_str += f'{self.frame_type} frame, w/ dims {self.x_size}x{self.y_size}'
        return to_str

def client():
    host = "127.0.0.1"  # as both code is running on same pc
    port = 8090  # socket server port number

    client_socket = socket.socket()  # instantiate
    client_socket.connect((host, port))  # connect to the server


    
    while True:
        data = client_socket.recv(PROTOCOL_HEADER_SIZE)

        header = NSLS2TomoStreamProtocolHeader(data)

        print(str(header))

        #raw_img_data = client_socket.recv(header.num_bytes)




    client_socket.close()  # close the connection


if __name__ == '__main__':
    client()
