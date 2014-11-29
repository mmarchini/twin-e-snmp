#!/usr/bin/env python
#
# python-netsnmpagent example agent with threading
#
# Copyright (c) 2013 Pieter Hollants <pieter@hollants.com>
# Licensed under the GNU Public License (GPL) version 3
#

#
# simple_agent.py demonstrates registering the various SNMP object types quite
# nicely but uses an inferior control flow logic: the main loop blocks in
# net-snmp's check_and_process() call until some event happens (eg. SNMP
# requests need processing). Only then will data be updated, not inbetween. And
# on the other hand, SNMP requests can not be handled while data is being
# updated, which might take longer periods of time.
#
# This example agent uses a more real life-suitable approach by outsourcing the
# data update process into a separate thread that gets woken up through an
# SIGALRM handler at an configurable interval. This does only ensure periodic
# data updates, it also makes sure that SNMP requests will always be replied to
# in time.
#
# Note that this implementation does not address possible locking issues: if
# a SNMP client's requests are processed while the data update thread is in the
# midst of refreshing the SNMP objects, the client might receive partially
# inconsistent data.
#
# Use the included script run_threading_agent.sh to test this example.
#
# Alternatively, see the comment block in the head of simple_agent.py for
# adaptable instructions how to run this example against a system-wide snmpd
# instance.
#

import tempfile
import sys, os, signal
import optparse, threading

# Make sure we use the local copy, not a system-wide one
sys.path.insert(0, os.path.dirname(os.getcwd()))
import netsnmpagent

prgname = sys.argv[0]

# Process command line arguments
parser = optparse.OptionParser()
parser.add_option(
    "-i",
    "--interval",
    dest="interval",
    help="Set interval in seconds between data updates",
    default=1
)
parser.add_option(
    "-m",
    "--mastersocket",
    dest="mastersocket",
    help="Sets the transport specification for the master agent's AgentX socket",
    default="/var/run/agentx/master"
)
parser.add_option(
    "-p",
    "--persistencedir",
    dest="persistencedir",
    help="Sets the path to the persistence directory",
    default="/var/lib/net-snmp"
)
(options, args) = parser.parse_args()

headerlogged = 0

# Create an instance of the netsnmpAgent class
try:
    agent = netsnmpagent.netsnmpAgent(
        AgentName      = "LBA_Agent",
        MasterSocket   = options.mastersocket,
        PersistenceDir = options.persistencedir,
        MIBFiles       = [ os.path.abspath(os.path.dirname(sys.argv[0])) +
                           "/LBA-MIB.mib" ],
    )
except netsnmpagent.netsnmpAgentException as e:
    print "{0}: {1}".format(prgname, e)
    sys.exit(1)

# Register the only SNMP object we server, a DisplayString
twsnLife = agent.Integer32(
    oidstr  = "LBA-MIB::twsnLife",
    initval = 0
)

import socket

# Create a UDS socket
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = os.path.join(tempfile.gettempdir(), 'socket')
print "Conectando (%s)..."%server_address
sock.connect(server_address)
print "Conectado!"

def UpdateSNMPObjs():
    """ Function that does the actual data update. """

    sock.send("0")
    output = sock.recv(255)
    striped = output.strip("\0")
    if not striped:
        print "Nao veio dessa vez..."
        return
    print output
    twsnLife.update(int(striped))

def UpdateSNMPObjsAsync():
    """ Starts UpdateSNMPObjs() in a separate thread. """

    # UpdateSNMPObjs() will be executed in a separate thread so that the main
    # thread can continue looping and processing SNMP requests while the data
    # update is still in progress. However we'll make sure only one update
    # thread is run at any time, even if the data update interval has been set
    # too low.
    if threading.active_count() == 1:
        t = threading.Thread(target=UpdateSNMPObjs, name="UpdateSNMPObjsThread")
        t.daemon = True
        t.start()
    else:
        pass

# Start the agent (eg. connect to the master agent).
try:
    agent.start()
except netsnmpagent.netsnmpAgentException as e:
    print "{0}: {1}".format(prgname, e)
    sys.exit(1)

# Trigger initial data update.
UpdateSNMPObjsAsync()

# Install a signal handler that terminates our threading agent when CTRL-C is
# pressed or a KILL signal is received
def TermHandler(signum, frame):
    global loop
    loop = False
signal.signal(signal.SIGINT, TermHandler)
signal.signal(signal.SIGTERM, TermHandler)

# Define a signal handler that takes care of updating the data periodically
def AlarmHandler(signum, frame):
    global loop, timer_triggered


    if loop:
        timer_triggered = True

        UpdateSNMPObjsAsync()

        signal.signal(signal.SIGALRM, AlarmHandler)
        signal.setitimer(signal.ITIMER_REAL, float(options.interval))
msg = "Installing SIGALRM handler triggered every {0} seconds."
msg = msg.format(options.interval)
signal.signal(signal.SIGALRM, AlarmHandler)
signal.setitimer(signal.ITIMER_REAL, float(options.interval))

loop = True
while loop:
    # Block until something happened (signal arrived, SNMP packets processed)
    timer_triggered = False
    res = agent.check_and_process()
    if res == -1 and not timer_triggered and loop:
        loop = False
        print "Error {0} in SNMP packet processing!".format(res)
    elif loop and timer_triggered:
        print "net-snmp's check_and_process() returned due to SIGALRM (res={0}), doing another loop.".format(res)
    elif loop:
        print "net-snmp's check_and_process() returned (res={0}), doing another loop.".format(res)

print "Terminating."
#del agent
