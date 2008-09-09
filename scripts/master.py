#!/usr/bin/python
#
################################################################################
# Plutocracy - Copyright (C) 2008 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
################################################################################
#
# This is the master server script that runs on the web host.

import time
import cgi
import cgitb
import ConfigParser
import re

# Seconds after which a server entry expires
SERVER_TTL = 360.0

# Server list filename
SERVERS_FILE = 'servers.ini'

# Output HTML protocol header
print "Content-type: text/plain\r\n\r\n"

cgitb.enable()

# Open the configuration file
config = ConfigParser.ConfigParser()
config.read(SERVERS_FILE)

# Get the current time
time = time.time()

# See if there is input to the script
try:
        # Invalid input string regex
        invalid_re = re.compile('[=\[\];]')

        # Function to properly escape a string
        def escape(s):
                s = s.replace("'", "\\'")
                return s.replace('"', '\\"')

        # Function to test for invalid input
        def invalid(s, l):
                return invalid_re.match(s) != None or len(s) > l

        form = cgi.FieldStorage()
        address = (cgi.os.environ['REMOTE_ADDR'] + ':' +
                   escape(form['port'].value))

        # No name indicates a server is done hosting
        if (not form.has_key('name')):
                config.remove_section(address)
                raise 'Removed'

        # Make sure this input is not retarded
        if (invalid(form['name'].value, 16) or
            invalid(form['info'].value, 16) or
            form['port'].value.isdigit() == False or
            form['protocol'].value.isdigit() == False):
                raise 'Invalid'

        # Add the section if it is not already there
        if (not config.has_section(address)):
                config.add_section(address)

        config.set(address, 'time', str(time))
        config.set(address, 'name', escape(form['name'].value))
        config.set(address, 'info', escape(form['info'].value))
        config.set(address, 'protocol', escape(form['protocol'].value))
except:
        pass

# Start off with a preamble token
print 'plutocracy_master'

# List the servers we already have
servers = config.sections()
for s in servers:
        try:
                if (float(config.get(s, 'time')) + SERVER_TTL < time):
                        raise 'Expired'
                print ('server ' + config.get(s, 'protocol') + ' "' +
                       s + '" "' + config.get(s, 'name') + '" "' +
                       config.get(s, 'info') + '"')
        except:
                config.remove_section(s)

# Save the INI file
ini_file = open(SERVERS_FILE, 'w')
config.write(ini_file)
ini_file.close()

