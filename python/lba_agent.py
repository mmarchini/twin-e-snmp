import socket
import tempfile
import sys, os
import threading
from time import sleep

import netsnmpagent

prgname = sys.argv[0]

def deleteRow(table, row):
    netsnmpagent.libnsX.netsnmp_table_dataset_remove_and_delete_row(
        table._dataset,
        row._table_row
    )

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

    def _communicate(self, method, value=None):
        striped = None

        if not self._client:
            return None

        with self._lock:
            # Request data
            str_method = "%d"%method
            if value != None:
                str_method = "%s %s"%(str_method, value)
            if len(str_method) < 255:
                str_method += "\0"*(255-len(str_method)) # padding
            elif len(str_method) > 254:
                str_method[:254]
                str_method.append("\0")
            try:
                self._client.send(str_method)
            except socket.error:
                print "Cliente desconectado!"
                self._client = None
                return None
            except AttributeError:
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

    def getMaxLife(self,):
        returned_value = self._communicate(3)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getCurLife(self,):
        returned_value = self._communicate(4)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getMaxMP(self,):
        returned_value = self._communicate(5)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getCurMP(self,):
        returned_value = self._communicate(6)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getKeys(self,):
        returned_value = self._communicate(7)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getCash(self,):
        returned_value = self._communicate(8)
        if returned_value == None:
            return 0
        return int(returned_value)

    # -- Leaf Table -- #

    def getNumLeafs(self,):
        returned_value = self._communicate(9)
        if returned_value == None:
            return None
        return int(returned_value)

    def getNumLeafBoxes(self,):
        returned_value = self._communicate(10)
        if returned_value == None:
            return None
        return int(returned_value)

    def getCurBehaviour(self,):
        returned_value = self._communicate(11)
        if returned_value == None:
            return 0
        return int(returned_value)

    def getHasProtopack(self,):
        returned_value = self._communicate(12)
        if returned_value == None:
            return None
        return int(returned_value)

    ###########
    # SETTERS #
    ###########

    def setPaused(self, value):
        returned_value = self._communicate(100, value)
        if returned_value == None:
            return None
        return int(returned_value)

    def setPlayerName(self, value):
        returned_value = self._communicate(101, value)
        if returned_value == None:
            return ''
        return returned_value

    def setCurBehaviour(self, value):
        returned_value = self._communicate(111, value)
        if returned_value == None:
            return None
        return int(returned_value)

class LBAAgent(object):

    def __init__(self, mastersocket, persistencedir):
        self._agent = self._create_agent(mastersocket, persistencedir)

        server_address = os.path.join(tempfile.gettempdir(), 'twin_socket')
        self._twin_socket = TwinSocket(server_address)

        #
        self._lbaGameRunning()
        self._lbaGamePaused()
        self._lbaGamePlayerName()

        #
        self._lbaHeroMaxLife()
        self._lbaHeroCurLife()
        self._lbaHeroMaxMP()
        self._lbaHeroCurMP()
        self._lbaHeroCash()
        self._lbaHeroKeys()
        ##
        self._lbaHeroLeafTable()

        #
        self._lbaBehaviourCurrent()
        self._lbaBehaviourTable()


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

        paused = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaGamePaused",
            initval = 0
        )

        def paused_updater():

            last_value = 0

            while True:
                sleep(1)
                if last_value == paused.value():
                    last_value = self._twin_socket.getPaused()
                else:
                    last_value = self._twin_socket.setPaused(paused.value())
                paused.update(last_value)

        t = threading.Thread(target=paused_updater)
        t.daemon = True
        t.start()

    def _lbaGamePlayerName(self, ):

        player_name = self._agent.DisplayString(
            oidstr  = "LBA-MIB::lbaGamePlayerName",
            initval = ""
        )

        def player_name_updater():

            last_value = ''

            while True:
                sleep(1)
                if last_value == player_name.value():
                    last_value = self._twin_socket.getPlayerName()
                else:
                    last_value = self._twin_socket.setPlayerName(player_name.value())
                player_name.update(last_value)

        t = threading.Thread(target=player_name_updater)
        t.daemon = True
        t.start()

    #######################
    # LBA Hero Statistics #
    #######################

    def _lbaHeroMaxLife(self, ):

        max_life = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroMaxLife",
            initval = 0
        )

        def max_life_updater():

            while True:
                sleep(1)
                max_life.update(self._twin_socket.getMaxLife())

        t = threading.Thread(target=max_life_updater)
        t.daemon = True
        t.start()

    def _lbaHeroCurLife(self, ):

        cur_life = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroCurLife",
            initval = 0
        )

        def cur_life_updater():

            while True:
                sleep(1)
                cur_life.update(self._twin_socket.getCurLife())

        t = threading.Thread(target=cur_life_updater)
        t.daemon = True
        t.start()

    def _lbaHeroMaxMP(self, ):

        max_mp = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroMaxMP",
            initval = 0
        )

        def max_mp_updater():

            while True:
                sleep(1)
                max_mp.update(self._twin_socket.getMaxMP())

        t = threading.Thread(target=max_mp_updater)
        t.daemon = True
        t.start()

    def _lbaHeroCurMP(self, ):

        cur_mp = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroCurMP",
            initval = 0
        )

        def cur_mp_updater():

            while True:
                sleep(1)
                cur_mp.update(self._twin_socket.getCurMP())

        t = threading.Thread(target=cur_mp_updater)
        t.daemon = True
        t.start()

    def _lbaHeroCash(self, ):

        cash = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroCash",
            initval = 0
        )

        def cash_updater():

            while True:
                sleep(1)
                cash.update(self._twin_socket.getCash())

        t = threading.Thread(target=cash_updater)
        t.daemon = True
        t.start()

    def _lbaHeroKeys(self, ):

        keys = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaHeroKeys",
            initval = 0
        )

        def keys_updater():

            while True:
                sleep(1)
                keys.update(self._twin_socket.getKeys())

        t = threading.Thread(target=keys_updater)
        t.daemon = True
        t.start()

    def _lbaHeroLeafTable(self, ):

        table = self._agent.Table(
            oidstr  = "LBA-MIB::lbaHeroLeafTable",
            indexes = [
                self._agent.Unsigned32()
            ],
            columns = [
                (2, self._agent.Integer32())
            ]
        )

        table_rows = []

        def append(value):
            row = table.addRow([self._agent.Unsigned32(len(table_rows)+1)])
            row.setRowCell(2, self._agent.Integer32(value))
            table_rows.append(row)

        def update(index, value):
            row = table_rows[index]
            row.setRowCell(2, self._agent.Integer32(value))

        def delete(index):
            row = table_rows.pop(index)
            deleteRow(table, row)

        def clean_up():
            while len(table_rows):
                row = table_rows.pop()
                deleteRow(table, row)

        is_filled = lambda i, leafs: i<leafs and 1 or 0

        def keys_updater():

            while True:
                sleep(1)
                leafs = self._twin_socket.getNumLeafs()
                boxes = self._twin_socket.getNumLeafBoxes()

                if leafs == None or boxes == None:
                    clean_up()
                else:
                    for i in range(0, len(table_rows)):
                        update(i, (is_filled(i, leafs)))

                    if boxes > len(table_rows):
                        for i in range(len(table_rows), boxes):
                            append(is_filled(i, leafs))
                    elif boxes < len(table_rows):
                        for i in range(boxes, len(table_rows)):
                            delete(i)

        t = threading.Thread(target=keys_updater)
        t.daemon = True
        t.start()

    ######################
    # LBA Hero Behaviour #
    ######################

    def _lbaBehaviourCurrent(self, ):

        cur_behaviour = self._agent.Unsigned32(
            oidstr  = "LBA-MIB::lbaBehaviourCurrent",
            initval = 0
        )

        def cur_behaviour_updater():

            last_value = 0

            while True:
                sleep(1)
                if last_value == cur_behaviour.value():
                    last_value = self._twin_socket.getCurBehaviour()
                else:
                    last_value = self._twin_socket.setCurBehaviour(cur_behaviour.value())
                cur_behaviour.update(last_value)

        t = threading.Thread(target=cur_behaviour_updater)
        t.daemon = True
        t.start()

    def _lbaBehaviourTable(self, ):
        table = self._agent.Table(
            oidstr  = "LBA-MIB::lbaBehaviourTable",
            indexes = [
                self._agent.Unsigned32()
            ],
            columns = [
                (2, self._agent.DisplayString())
            ]
        )

        table.addRow([self._agent.Unsigned32(0)])\
            .setRowCell(2, self._agent.DisplayString("Normal"))

        table.addRow([self._agent.Unsigned32(1)])\
            .setRowCell(2, self._agent.DisplayString("Esportivo"))

        table.addRow([self._agent.Unsigned32(2)])\
            .setRowCell(2, self._agent.DisplayString("Agressivo"))

        table.addRow([self._agent.Unsigned32(3)])\
            .setRowCell(2, self._agent.DisplayString("Discreto"))


        def behaviours_updater():

            proto_row = None

            while True:
                sleep(1)
                has_protopack = self._twin_socket.getHasProtopack()

                if has_protopack and not proto_row:
                    proto_row = table.addRow([self._agent.Unsigned32(4)])
                    proto_row.setRowCell(2, self._agent.DisplayString("ProtoPack"))
                elif proto_row:
                    del proto_row

        t = threading.Thread(target=behaviours_updater)
        t.daemon = True
        t.start()

