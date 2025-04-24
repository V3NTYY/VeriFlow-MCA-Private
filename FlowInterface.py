from pox.core import core
import pox.openflow.libopenflow_01 as of
import socket
import threading
import json

log = core.getLogger()

class FlowInterface:
    def __init__(self, flowport):
        core.openflow.addListeners(self)
        self.switches = {}
        self.start_socket_server(flowport)

    def _handle_ConnectionUp(self, event):
        # This method stores the DPID and IP address of each switch for mappings w/ CCPDN
        connection = event.connection
        dpid = connection.dpid
        switch_ip, switch_port = connection.sock.getpeername()
        log.info("Switch %s connected from %s:%s", dpid, switch_ip, switch_port)
        self.switches[dpid] = connection

    def start_socket_server(self, flowport):
        # Start socket server to listen for ccpdn
        def socket_thread():
            flowinterface_port = flowport + 1
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.bind(("0.0.0.0", flowinterface_port))
            server_socket.listen(5)
            log.info("FlowHandler server started on port %s", flowinterface_port)

            while True:
                client_socket, addr = server_socket.accept()
                log.info("Connection received from %s", addr)
                threading.Thread(target=self.handle_client, args=(client_socket,)).start()

        threading.Thread(target=socket_thread, daemon=True).start()

    def handle_client(self, client_socket):
        # Process client commands
        try:
            while True:
                data = client_socket.recv(1024).decode("utf-8")
                log.info("Received command: %s", data)

                result = [ 0, 0, 0, 0, 0, 0 ]  # Initialize

                if not data:
                    log.info("Client %s:%s disconnected.", client_socket.getpeername()[0], client_socket.getpeername()[1])
                    break

                if (data == None):
                    log.error("Received malformed/empty data.")
                    continue

                # Ensure data consistency
                data = data.strip()
                data = data.replace(" ", "")  # Remove spaces
                data = data.replace("\n", "")  # Remove newlines
                data = data.replace("\r", "")  # Remove carriage returns
                data = data.replace("\t", "")  # Remove tabs
                
                # Parse listflows version of the command
                if (data.startswith("listflows")):
                    result = data.split("-")
                    result = [ result[0], result[1], 0, 0, 0, result[2] ]

                # Parse the command, returns a set with {command, srcDPID, output_port, nw_src, Wildcards, XID}
                if (result == [0, 0, 0, 0, 0, 0]):
                    result = self.parse_data(data)

                if (result == None):
                    log.error("Error parsing data: %s", data)
                    continue
                
                srcDPID = int(result[1])
                outPort = int(result[2])
                xid = int(result[5])

                # Create match object from our nw_src, Wildcards and dstDPID
                match = of.ofp_match()
                match.nw_proto = 0x06  # TCP
                match.nw_src = result[3]
                match.wildcards = result[4]
                match.dl_type = 0x0800  # IPv4

                # Create action object based on srcDPID and dstDPID
                action = of.ofp_action_output(port=outPort)

                # Apply commands via controller
                if result[0] == "addflow":
                    self.add_flow(srcDPID, match, action, xid)
                elif result[0] == "removeflow":
                    self.remove_flow(srcDPID, match, action, xid)
                elif result[0] == "listflows":
                    self.list_flows(srcDPID, xid)
        except Exception as e:
            log.error("Error handling client: %s", e)
        finally:
            client_socket.close()

    def parse_data(self, data):

        #    ---Format of received data---
        # command-A#switchDPID-rulePrefix-nextHopDPID-XID
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
            Wildcards = (32 - mask) << of.OFPFW_NW_SRC_SHIFT
        except Exception as e:
            log.error("Error parsing rule prefix: %s", e)
            return None

        # Returns a set with {command, srcDPID, dstDPID, nw_src, Wildcards}
        return [ args[0], args[1], args[3], Nw_src, Wildcards, args[4] ]

    def add_flow(self, dpid, match, action, XID):

        # Send a Flow Mod command to the switch
        fm = of.ofp_flow_mod()
        fm.match = match
        fm.actions.append(action)
        fm.xid = XID

        try:
            self.switches[dpid].send(fm)
        except Exception as e:
            log.error("Error sending flow mod to switch %s: %s", dpid, e)
            return
        
        log.info("Flow %s added to switch %s", fm.xid, dpid)

    def remove_flow(self, dpid, match, action, XID):

        # Send a Flow Remove command to the switch
        fm = of.ofp_flow_mod(command=of.OFPFC_DELETE)
        fm.match = match
        fm.actions.append(action)
        fm.xid = XID

        try:
            self.switches[dpid].send(fm)
        except Exception as e:
            log.error("Error sending flow remove to switch %s: %s", dpid, e)
            return
        
        log.info("Flow %s removed from switch %s", fm.xid, dpid)

    def list_flows(self, dpid, XID):

        # Send a stats request to the switch
        connection = self.switches[dpid]
        req = of.ofp_stats_request(body=of.ofp_flow_stats_request())
        req.xid = XID

        try:
            connection.send(req)
        except Exception as e:
            log.error("Error sending stats request to switch %s: %s", dpid, e)
            return
        
        log.info("Flow %s requested stats from switch %s", req.xid, dpid)

def launch(flowport):
    flowport = int(flowport)
    core.registerNew(FlowInterface, flowport)