import socket
import tempfile
import sys, os
import threading
from time import sleep

import netsnmpagent

prgname = sys.argv[0]

class TwinSocket(object):

    def __init__(self, socket_file):
        self._lock = threading.Lock()

        try:
            os.unlink(socket_file)
        except OSError:
            if os.path.exists(socket_file):
                raise

        # Create a UDS socket
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # Connect the socket to the port where the server is listening
        print "Criando o socket em '%s'..."%socket_file
        self.sock.bind(socket_file)
        self.sock.listen(0)
        print "Socket criado!"

        self._client = None
        self._connect()

    def _connect(self):
        def acceptClient():
            while True:
                while self._client:
                    pass
                print "Esperando alguem conectar..."
                self._client, addr = self.sock.accept()
                print "Novo cliente conectado!"

        t = threading.Thread(target=acceptClient, name="UpdateSNMPObjsThread")
        t.daemon = True
        t.start()

    def _communicate(self, method):
        striped = None

        if not self._client:
            return None

        with self._lock:
            # Request data
            try:
                str_method = "%d"%method
                str_method += "\0"*(255-len(str_method)) # padding
                self._client.send(str_method)
            except socket.error:
                print "Cliente desconectado!"
                self._client = None
                return None

            # Returned data
            output = self._client.recv(255)
            striped = output.strip("\0")

        return striped

    ###################
    # Game Statistics #
    ###################

    def isRunning(self):
        self._communicate(-1)
        return self._client and 1 or 0

    def getPaused(self,):
        returned_value = self._communicate(0)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getPlayerName(self,):
        returned_value = self._communicate(1)
        if returned_value == None:
            return ""
        return returned_value

    ###################
    # Hero Statistics #
    ###################

    def getLife(self,):
        returned_value = self._communicate(3)
        if returned_value == None:
            return 0
        return int(returned_value)

class LBAAgent(object):

    def __init__(self, mastersocket, persistencedir):
        self._agent = self._create_agent(mastersocket, persistencedir)

        server_address = os.path.join(tempfile.gettempdir(), 'twin_socket')
        self._twin_socket = TwinSocket(server_address)

        self._lbaGameRunning()
        self._lbaGamePaused()
        self._lbaGamePlayerName()

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

    #######################
    # LBA Game Statistics #
    #######################

    def _lbaGameRunning(self, ):

        running = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaGameRunning",
            initval = 0
        )

        def running_updater():

            while True:
                sleep(1)
                running.update(self._twin_socket.isRunning())

        t = threading.Thread(target=running_updater)
        t.daemon = True
        t.start()

    def _lbaGamePaused(self, ):

        # Register the only SNMP object we server, a DisplayString
        paused = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaGamePaused",
            initval = 0
        )

        def paused_updater():

            while True:
                sleep(1)
                paused.update(self._twin_socket.getPaused())

        t = threading.Thread(target=paused_updater)
        t.daemon = True
        t.start()

    def _lbaGamePlayerName(self, ):

        # Register the only SNMP object we server, a DisplayString
        player_name = self._agent.DisplayString(
            oidstr  = "LBA-MIB::lbaGamePlayerName",
            initval = ""
        )

        def player_name_updater():

            while True:
                sleep(1)
                player_name.update(self._twin_socket.getPlayerName())

        t = threading.Thread(target=player_name_updater)
        t.daemon = True
        t.start()

    #######################
    # LBA Hero Statistics #
    #######################

    def _getLife(self, ):

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

