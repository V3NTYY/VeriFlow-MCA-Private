from pox.core import core
import pox.openflow.libopenflow_01 as of
import socket
import threading
import json

log = core.getLogger()

class FlowInterface:
    def __init__(self):
        core.openflow.addListeners(self)
        self.start_socket_server()

    def start_socket_server(self):
        """Start a socket server to handle external commands."""
        def socket_thread():
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.bind(("0.0.0.0", 6667))  # Listen on port 6667
            server_socket.listen(5)
            log.info("FlowHandler server started on port 6667")

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

            # Parse the command, returns a dictionary with {command, {flow_data}}
            result = self.parse_data(data)

            if action == "add_flow":
                self.add_flow(dpid, match, actions)
            elif action == "remove_flow":
                self.remove_flow(dpid, match)
            elif action == "list_flows":
                self.list_flows(dpid)
        except Exception as e:
            log.error("Error handling client: %s", e)
        finally:
            client_socket.close()

    def parse_data(self, data):

        #    ---Format of received data---
        # command-A#switchIP-rulePrefix-nextHopIP
        # Commands: addflow, removeflow, listflows

        # Parse each individual arg using "-" as a delimiter
        args = set(data.split("-"))

        # Remove A# or R# from the second arg
        if (args[1][0].contains("A#") or args[1][0].contains("R#")):
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

        # Returns a dictionary with {command, {flow_data}}


    def add_flow(self, dpid, match, actions):

        # Send a Flow Mod command to the switch
        fm = of.ofp_flow_mod()
        for field, value in match.items():
            setattr(fm.match, field, value)
        for action in actions:
            fm.actions.append(of.ofp_action_output(port=action["port"]))

        self.switches[dpid].send(fm)
        log.info("Flow added to switch %s", dpid)

    def remove_flow(self, dpid, match):

        # Send a Flow Remove command to the switch
        fm = of.ofp_flow_mod(command=of.OFPFC_DELETE)
        for field, value in match.items():
            setattr(fm.match, field, value)

        self.switches[dpid].send(fm)
        log.info("Flow removed from switch %s", dpid)

    def list_flows(self, dpid):

        # Send a stats request to the switch
        connection = self.switches[dpid]
        connection.send(of.ofp_stats_request(body=of.ofp_flow_stats_request()))

def launch():
    core.registerNew(FlowInterface)