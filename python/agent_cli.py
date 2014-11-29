#!/usr/bin/env python

import optparse

import lba_agent

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

agent = lba_agent.LBAAgent(options.mastersocket, options.persistencedir)
agent.start()

