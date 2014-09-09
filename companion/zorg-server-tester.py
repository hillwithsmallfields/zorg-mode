#!/usr/bin/env python

"""
A simple echo client, based on http://ilab.cs.byu.edu/python/socket/echoclient.html
"""

import socket

commands = ['D\n',
            'd\n',
            'd\n',
            'F/home/jcgs/Dropbox/org/general.org\n',
            'Q\n']

host = 'localhost'
port = 48080
size = 1024
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
for command in commands:
    s.connect((host,port))
    print 'Sending:', command
    s.send(command)
    data = s.recv(size)
    print 'Received:', data 
    s.close()
