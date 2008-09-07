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
        # Function to properly escape a string
        def escape(s):
                return s.replace('"', '\\"')

        form = cgi.FieldStorage()
        address = (cgi.os.environ['REMOTE_ADDR'] + ':' +
                   escape(form['port'].value))

        # No name indicates a server is done hosting
        if (not form.has_key('name')):
                config.remove_section(address)
                raise 'Removed'

        # Add the section if it is not already there
        if (not config.has_section(address)):
                config.add_section(address)

        config.set(address, 'time', str(time))
        config.set(address, 'name', escape(form['name'].value))
        config.set(address, 'info', escape(form['info'].value))
except:
        pass

# List the servers we already have
servers = config.sections()
for s in servers:
        try:
                if (float(config.get(s, 'time')) + SERVER_TTL < time):
                        raise 'Expired'
                print ('"' + s + '" "' + config.get(s, 'name') + '" "' +
                       config.get(s, 'info') + '"')
        except:
                config.remove_section(s)

# Save the INI file
ini_file = open(SERVERS_FILE, 'w')
config.write(ini_file)
ini_file.close()

