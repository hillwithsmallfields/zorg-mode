#!/usr/bin/env python
# server / companion for zorg mode
# uses https://github.com/bjonnh/PyOrgMode

# Companion app for zorg-mode, that will respond to requests from the watchapp
# using bits from https://docs.python.org/2/library/socketserver.html to get started

# todo: stop using PyOrgMode and do it all myself, I think it'll be simpler for this application

import argparse
import SocketServer
import os
import os.path
import fnmatch

port = 48084
org_directory = os.path.expanduser("~/Dropbox/org/")
session_running = True
directory_files = []
file_lines = []

def serve_directory_start(directory):
    """Return the first part of the listing of DIRECTORY."""
    global directory_files
    directory_files = []
    print "Starting directory listing of", directory
    for file in os.listdir(directory):
        if fnmatch.fnmatch(file, "*.org"):
            directory_files.append(file)
    return directory_files.pop(0)

def serve_directory_next():
    """Return the next part of the listing of the current directory."""
    global directory_files
    return directory_files.pop(0)

def serve_file_start(filename):
    """Serve the first part of FILENAME."""
    global file_lines
    filename = os.path.join(org_directory, filename)
    file = open(filename)
    file_lines = [ line.rstrip('\n') for line in file ]
    file.close()
    total_size = 0
    for line in file_lines:
        total_size += len(line) + 1
    return "bytes=%d;lines=%d" % (total_size,len(file_lines))

def serve_file_next():
    """Return the next part of the current file."""
    # todo: send the keywords, tags, and date lines on the first three calls, then start sending data lines
    global file_lines
    return "file line to go here"

def handle_keyword_change(change_string):
    pass

def handle_request(request):
    global session_running
    command = request[0]
    if command == "f":
        return serve_file_next()
    elif command == "F":
        return serve_file_start(request[1:])
    elif command == "D":
        return serve_directory_start(org_directory)
    elif command == "d":
        return serve_directory_next()
    elif command == "Q":
        session_running = False
        return "quitting"
    pass

class MyTCPHandler(SocketServer.StreamRequestHandler):

    def handle(self):
        self.data = self.rfile.readline().strip()
        print "{} wrote:".format(self.client_address[0])
        print self.data
        result = handle_request(self.data) + "\n"
        print "Writing back result", result
        self.wfile.write(result)

def start_zorg_server():
    global org_directory
    global session_running
    local = True
    if local:
        hostname = 'localhost'
    else:
        hostname = socket.gethostname()
    parser = argparse.ArgumentParser(description="Support a zorg Pebble app")
    parser.add_argument('directory', help="The directory to scan for .org files")
    args = parser.parse_args()
    print "got", args.directory, "from command line; default dir is", org_directory
    org_directory = args.directory

    print "Files in", org_directory, "are:"
    for file in os.listdir(org_directory):
        print " * ", file

    print "Opening on port", port

    server = SocketServer.TCPServer((hostname, port), MyTCPHandler)

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    server.serve_forever()

if __name__ == '__main__':
    start_zorg_server()
