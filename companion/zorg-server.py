#!/usr/bin/python
# server / companion for zorg mode
# uses https://github.com/bjonnh/PyOrgMode

# Companion app for zorg-mode, that will respond to requests from the watchapp

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

def start_zorg_server():
    # todo: open a socket, listen on it
    pass

if __name__ == '__main__':
    startZorgServer()
