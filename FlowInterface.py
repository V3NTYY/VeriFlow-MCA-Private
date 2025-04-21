from pox.core import core
import pox.openflow.libopenflow_01 as of
import socket
import threading
import json

log = core.getLogger()

class FlowInterface:
    def __init__(self):
        core.openflow.addListeners(self)
        self.switches = {}
        self.start_socket_server()

    def _handle_ConnectionUp(self, event):
        # This method stores the DPID and IP address of each switch for mappings w/ CCPDN
        connection = event.connection
        dpid = connection.dpid
        switch_ip, switch_port = connection.sock.getpeername()
        log.info("Switch %s connected from %s:%s", dpid, switch_ip, switch_port)
        self.switches[dpid] = connection

    def start_socket_server(self):
        # Start socket server to listen for ccpdn
        def socket_thread():
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.bind(("0.0.0.0", 6655))  # Listen on port 6655
            server_socket.listen(5)
            log.info("FlowHandler server started on port 6655")

            while True:
                client_socket, addr = server_socket.accept()
                log.info("Connection received from %s", addr)
                threading.Thread(target=self.handle_client, args=(client_socket,)).start()

        threading.Thread(target=socket_thread, daemon=True).start()

    def handle_client(self, client_socket):
        # Process client commands
        try:
            data = client_socket.recv(1024).decode("utf-8")
            log.info("Received command: %s", data)

            result = { 0, 0, 0, 0, 0 }  # Initialize

            if (data == None):
                log.error("Received malformed/empty data.")
                return

            # Ensure data consistency
            data = data.strip()
            data = data.replace(" ", "")  # Remove spaces
            data = data.replace("\n", "")  # Remove newlines
            data = data.replace("\r", "")  # Remove carriage returns
            data = data.replace("\t", "")  # Remove tabs
            
            # Parse listflows version of the command
            if (data.startswith("listflows")):
                result = data.split("-")
                result[2] = 0
                result[3] = 0
                result[4] = 0

            # Parse the command, returns a set with {command, srcDPID, dstDPID, nw_src, Wildcards}
            if (result == None):
                result = self.parse_data(data)

            if (result == None):
                log.error("Error parsing data: %s", data)
                return
            
            # print contents of result
            log.info("Parsed result: %s", result)
            
            srcDPID = int(result[1])
            dstDPID = int(result[2])

            # Create match object from our nw_src, Wildcards and dstDPID
            match = of.ofp_match()
            match.nw_src = result[3]
            match.wildcards = result[4]
            match.dl_type = 0x0800  # IPv4

            # Create action object based on srcDPID and dstDPID
            output_port = self.get_output_port(srcDPID, dstDPID)
            if (output_port == None and srcDPID != dstDPID):
                log.error("No output port found for %s to %s", srcDPID, dstDPID)
            action = of.ofp_action_output(port=output_port)

            # Apply commands via controller
            if result[0] == "add_flow":
                self.add_flow(srcDPID, match, action)
            elif result[0] == "remove_flow":
                self.remove_flow(srcDPID, match, action)
            elif (result[0] == "listflows"):
                self.list_flows(srcDPID)
        except Exception as e:
            log.error("Error handling client: %s", e)
        finally:
            client_socket.close()

    def parse_data(self, data):

        #    ---Format of received data---
        # command-A#switchDPID-rulePrefix-nextHopDPID
        # Commands: addflow, removeflow, listflows

        # Parse each individual arg using "-" as a delimiter
        args = data.split("-")

        # Remove A# or R# from the second arg
        if (args[1][0].startswith("A#") or args[1][0].startswith("R#")):
            args[1] = args[1][2:]

        # Parse rule prefix into nw_src and wildcard
        try:
            rule_prefix = args[2].split("/")
            Nw_src = rule_prefix[0]
            mask = int(rule_prefix[1])
            Wildcards = (32 - mask) & 0xFF;
        except Exception as e:
            log.error("Error parsing rule prefix: %s", e)
            return None

        # Returns a set with {command, srcDPID, dstDPID, nw_src, Wildcards}
        return { args[0], args[1], args[3], Nw_src, Wildcards }

    def add_flow(self, dpid, match, action):

        # Send a Flow Mod command to the switch
        fm = of.ofp_flow_mod()
        fm.match = match
        fm.actions.append(action)

        self.switches[dpid].send(fm)
        log.info("Flow added to switch %s", dpid)

    def remove_flow(self, dpid, match, action):

        # Send a Flow Remove command to the switch
        fm = of.ofp_flow_mod(command=of.OFPFC_DELETE)
        fm.match = match
        fm.actions.append(action)

        self.switches[dpid].send(fm)
        log.info("Flow removed from switch %s", dpid)

    def list_flows(self, dpid):

        # Send a stats request to the switch
        connection = self.switches[dpid]
        connection.send(of.ofp_stats_request(body=of.ofp_flow_stats_request()))

def launch():
    core.registerNew(FlowInterface)