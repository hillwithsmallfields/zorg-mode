#!/usr/bin/env python

"""
A simple echo client, based on http://ilab.cs.byu.edu/python/socket/echoclient.html
"""

import socket

host = 'localhost'
port = 48088
size = 1024
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host,port))
s.send('F/home/jcgs/Dropbox/org/general.org')
data = s.recv(size)
s.close()
print 'Received:', data 
