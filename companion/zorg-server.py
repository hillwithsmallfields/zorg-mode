#!/usr/bin/env python
# server / companion for zorg mode
# uses https://github.com/bjonnh/PyOrgMode

# Companion app for zorg-mode, that will respond to requests from the watchapp

import argparse
import socket
import PyOrgMode
import os
import fnmatch

port = 48088

directory_files = []

def serve_directory_start(directory):
    """Return the first part of the listing of DIRECTORY."""
    global directory_files
    directory_files = []
    print "Starting directory listing of", directory
    for file in os.listdir(directory):
        if fnmatch.fnmatch(file, "*.org"):
            directory_files.append(file)
    print directory_files.pop(0)

def serve_directory_next():
    """Return the next part of the listing of the current directory."""
    global directory_files
    print directory_files.pop(0)

def serve_file_start(filename):
    """Serve the first part of FILENAME."""
    tree = PyOrgMode.OrgDataStructure()
    print "about to load tree from", filename
    tree.load_from_file(filename)
    print "tree is", tree
    # todo: get all keywords, tags, dates
    # todo: count up the size, and send the size line
    pass

def serve_file_next():
    """Return the next part of the current file."""
    # todo: send the keywords, tags, and date lines on the first three calls, then start sending data lines
    pass

def handle_keyword_change(change_string):
    pass

def receive_from(socket):
    chunks = []
    while True:
        chunk = socket.recv(1024)
        if chunk == '':
            return False
        chunks.append(chunk)
        if '\n' in chunk:
            return ''.join(chunks)

org_directory = "~/Dropbox/org/"

def handle_request(request):
    if request[0] == "f":
        serve_file_next()
    elif request[0] == "F":
        serve_file_start(request[1:])
    elif request[0] == "D":
        serve_directory_start(org_directory)
    elif request[0] == "d":
        serve_directory_next()
    pass

def start_zorg_server():
    global org_directory
    parser = argparse.ArgumentParser(description="Support a zorg Pebble app")
    parser.add_argument('directory', help="The directory to scan for .org files")
    args = parser.parse_args()
    print "got", args.directory, "from command line; default dir is", org_directory
    org_directory = args.directory

    print "Files in", org_directory, "are:"
    for file in os.listdir(org_directory):
        print " * ", file

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    local = True
    if local:
        hostname = 'localhost'
    else:
        hostname = socket.gethostname()
    server_socket.bind((hostname, port))
    server_socket.listen(5)
    while True:
        client_socket, address = server_socket.accept()
        # request = receive_from(client_socket)
        request = client_socket.recv(1024)
        if request:
            handle_request(request)
        client_socket.close()

if __name__ == '__main__':
    start_zorg_server()
