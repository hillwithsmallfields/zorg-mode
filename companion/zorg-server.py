#!/usr/bin/python
# server / companion for zorg mode
# uses https://github.com/bjonnh/PyOrgMode

# Companion app for zorg-mode, that will respond to requests from the watchapp

import argparse
import socket
import PyOrgMode

def serve_directory_start(directory):
    """Return the first part of the listing of DIRECTORY."""
    pass

def serve_directory_next():
    """Return the next part of the listing of the current directory."""
    pass

def serve_file_start(filename):
    """Serve the first part of FILENAME."""
    tree = PyOrgMode.OrgDataStructure()
    tree.load_from_file(filename)
    # todo: get all keywords, tags, dates
    # todo: count up the size, and send the size line
    pass

def serve_file_next():
    """Return the next part of the current file."""
    # todo: send the keywords, tags, and date lines on the first three calls, then start sending data lines
    pass

def handle_keyword_change(change_string):
    pass

def receive_from(client_socket):
    chunks = []
    while true:
        chunk = client_socket.recv(1024)
        if chunk == '':
            return false
        chunks.append(chunk)
        if '\n' in chunk:
            return ''.join(chunks)

def start_zorg_server():
    parser = argparse.ArgumentParser(description="Support a zorg Pebble app")
    parser.add_argument('directory', help="The directory to scan for .org files")
    args = parser.parse_args()
    # todo: open a socket, listen on it
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # hostname = socket.gethostname()
    hostname = 'localhost'
    server_socket.bind((hostname, 8080))
    server_socket.listen(5)
    # normally will start with being asked to list the files in args.directory
    while true:
        (client_socket, address) = server_socket.accept()
        # todo: put this into a thread of its own, and loop reading requests in it:
        request = receive_from(client_socket)

if __name__ == '__main__':
    startZorgServer()
