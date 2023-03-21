#!/usr/bin/env python3

import socket
import numpy as np
import struct
PROTOCOL_HEADER_SIZE=48


class NSLS2TomoStreamProtocolHeader:

    def __init__(self, raw_header_data):
        print(raw_header_data)
        unpacked = struct.unpack("BBBBBBdNNNN", raw_header_data)
        print(unpacked)

        self.protocol_version = f'{unpacked[0]}.{unpacked[1]}.{unpacked[2]}'
        self.frame_type = unpacked[3]
        self.reference_type = unpacked[4]
        self._dtype = unpacked[5]
        self.reference = unpacked[6]
        self.num_bytes =unpacked[7]
        self.x_size = unpacked[8]
        self.y_size = unpacked[9]
        self.color_channels = unpacked[10]


    def __str__(self):
        
        to_str = f'NSLS2 Tomo Protocol Header {self.protocol_version}\n'
        to_str += f'{self.frame_type} frame, ref: {self.reference} w/ dims {self.x_size}x{self.y_size}'
        return to_str


    @property
    def dtype(self):

        if self._dtype == 0:
            return np.dtype('int8')
        elif self._dtype == 1:
            return np.dtype('uint8')
        elif self._dtype == 2:
            return np.dtype('int16')
        elif self._dtype == 3:
            return np.dtype('uint16')


            
       


def client():
    host = "127.0.0.1"  # as both code is running on same pc
    port = 8090  # socket server port number

    client_socket = socket.socket()  # instantiate
    client_socket.connect((host, port))  # connect to the server


    
    while True:
        data = client_socket.recv(PROTOCOL_HEADER_SIZE)

        header = NSLS2TomoStreamProtocolHeader(data)

        print(str(header.dtype))

        print(str(header))

        arr = bytearray()
        while len(arr) < header.num_bytes:
            arr += client_socket.recv(header.num_bytes)
        img_data = arr[:header.num_bytes]
        print(len(img_data))
        for i in range(10):
            start = i * 2
            print(f"Pixel value of pixel {i}: {int.from_bytes(img_data[start:start+1], 'little')}")




    client_socket.close()  # close the connection


if __name__ == '__main__':
    client()
