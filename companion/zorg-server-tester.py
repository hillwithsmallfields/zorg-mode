#!/usr/bin/env python

"""
A simple echo client, based on http://ilab.cs.byu.edu/python/socket/echoclient.html
"""

import socket

host = 'localhost'
port = 48083
size = 1024
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host,port))
s.send('D')
data = s.recv(size)
s.close()
print 'Received:', data 
