import socket
import tempfile
import sys, os
import threading
from time import sleep

import netsnmpagent

prgname = sys.argv[0]

class TwinSocket(object):

    def __init__(self, socket_file):
        # Create a UDS socket
        self._lock = threading.Lock()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # Connect the socket to the port where the server is listening
        print "Conectando (%s)..."%socket_file
        self.sock.connect(socket_file)
        print "Conectado!"

    def _communicate(self, method):
        striped = None
        with self._lock:
            self.sock.send("%d"%method)
            output = self.sock.recv(255)
            striped = output.strip("\0")
        return striped

    def getLife(self,):
        return int(self._communicate(0))

class LBAAgent(object):

    def __init__(self, mastersocket, persistencedir):
        self._agent = self._create_agent(mastersocket, persistencedir)

        server_address = os.path.join(tempfile.gettempdir(), 'twin_socket')
        self._twin_socket = TwinSocket(server_address)

        self._create_twsnLife()

    def _create_agent(self, mastersocket, persistencedir):
        current_path = os.path.abspath(os.path.dirname(sys.argv[0]))
        mib_file = os.path.join(current_path, "LBA-MIB.mib")

        agent = netsnmpagent.netsnmpAgent(
            AgentName      = "LBA_Agent",
            MasterSocket   = mastersocket,
            PersistenceDir = persistencedir,
            MIBFiles       = [mib_file],
        )

        return agent

    def _create_twsnLife(self, ):

        # Register the only SNMP object we server, a DisplayString
        twsnLife = self._agent.Integer32(
            oidstr  = "LBA-MIB::twsnLife",
            initval = 0
        )

        def UpdateSNMPObjs():
            """ Function that does the actual data update. """

            while True:
                sleep(1)
                twsnLife.update(self._twin_socket.getLife())

        t = threading.Thread(target=UpdateSNMPObjs, name="UpdateSNMPObjsThread")
        t.daemon = True
        t.start()

    def start(self, ):
        # Start the agent (eg. connect to the master agent).
        try:
            self._agent.start()
        except netsnmpagent.netsnmpAgentException as e:
            print "{0}: {1}".format(prgname, e)
            sys.exit(1)

        loop = True
        while loop:
            # Block until something happened (signal arrived, SNMP packets processed)
            timer_triggered = False
            res = self._agent.check_and_process()
            if res == -1 and not timer_triggered and loop:
                loop = False
                print "Error {0} in SNMP packet processing!".format(res)
            elif loop and timer_triggered:
                print "net-snmp's check_and_process() returned due to SIGALRM (res={0}), doing another loop.".format(res)
            elif loop:
                print "net-snmp's check_and_process() returned (res={0}), doing another loop.".format(res)

        print "Terminating."

