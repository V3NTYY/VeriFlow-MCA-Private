
from VeriFlow.Network import Network
import socket
import sys
import threading
import time

ROUTE_VIEW = 1
BIT_BUCKET = 2

client_socket = None
pingFlag = threading.Event()
msg = None

def start_veriflow_server(host, port):
	global msg
	global client_socket

	def handle_client(client_socket):
		try:
			while True:
				msg = client_socket.recv(1024).decode('utf-8')
				if not msg:
					break
				parse_message(msg, client_socket)

		except Exception as e:
			print("\nError handling client: {}".format(e))
		finally:
			client_socket.close()

	def server_thread():
		global client_socket
		server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		server_socket.bind((host, port))
		server_socket.listen(2)
		print("\nVeriFlow server started on {}:{}".format(host, port))
		try:
			while True:
				client_socket, addr = server_socket.accept()
				print("\nConnection accepted from {}".format(addr))
				client_handler = threading.Thread(target=handle_client, args=(client_socket,))
				client_handler.start()
		except Exception as e:
			print("\nServer error: {}".format(e))
		finally:
			server_socket.close()

	def parse_message(message, client_socket):
		global msg
		# FORMAT: [CCPDN] FLOW A#192.168.0.0-0.0.0.0/0-192.168.0.1
		# If the message contains [CCPDN], then we can acknowledge it

		# Split message using null char as delimiter
		packets = message.split('\x00')

		# Iterate through each packet and process it
		for packet in packets:

			# Make sure we don't modify msg until it is none, sleep for 100ms
			while msg is not None:
				time.sleep(0.1)

			packet = packet.strip()
			if not packet:
				continue
			if "[CCPDN]" in packet:
				## Send hello back if we receive hello
				if "Hello" in packet:
					print("\nReceived hello message from CCPDN!")
					client_socket.send("[VERIFLOW] Hello".encode('utf-8'))
				## Handle logic for a flow rule added
				elif "FLOW" in packet:
					## Only parse characters after the text "[CCPDN] FLOW "
					print("\nReceived FLOW Mod from CCPDN!")
					packet = packet[13:].strip().rstrip('\x00')
					msg = packet
					pingFlag.set()

	# Create thread for server so we don't stall everything
	server_thread_instance = threading.Thread(target=server_thread)
	server_thread_instance.daemon = True
	server_thread_instance.start()

def checkPythonVersion():
	# Check if Python version is 3.x
	if sys.version_info[0] != 3:
		print("This script requires Python 3.x")
		sys.exit(1)

def main():
	global msg
	global client_socket
	checkPythonVersion()
	print("Enter network configuration file name (eg.: file.txt):")
	# filename = input("> ")
	filename = sys.argv[1]
	network = Network()
	network.parseNetworkFromFile(filename)

	## Setup VeriFlow server for CCPDN to pass messages to
	print("Enter IP address to host VeriFlow on (i.e. 127.0.0.1)")
	# veriflow_ip = input("> ")
	veriflow_ip = sys.argv[2]
	print("Enter port to host VeriFlow on (i.e. 6657)")
	# veriflow_port = int(input("> "))
	veriflow_port = sys.argv[3]

	veriflow_port = int(veriflow_port)
	start_veriflow_server(veriflow_ip, veriflow_port)

	generatedECs = network.getECsFromTrie()
	network.checkWellformedness()
	network.log(generatedECs)

	print("")
	print("Use ctrl+c to exit the program")
	print("")

	while True:
		## Wait for thread signal to handle message
		pingFlag.wait()
		pingFlag.clear()

		if msg is not None:
			affectedEcs = set()
			if (msg.startswith("A")):
				affectedEcs = network.addRuleFromString(msg[2:])
				if network.checkWellformedness(affectedEcs) is True:
					print("Rule added successfully!")
					client_socket.send("[VERIFLOW] Success".encode('utf-8'))
				else:
					print("Rule addition failed!")
					client_socket.send("[VERIFLOW] Fail".encode('utf-8'))
			elif (msg.startswith("R")):
				affectedEcs = network.deleteRuleFromString(msg[2:])
				if network.checkWellformedness(affectedEcs) is True:
					print("Rule deleted successfully!")
					client_socket.send("[VERIFLOW] Success".encode('utf-8'))
				else:
					print("Rule deletion failed!")
					client_socket.send("[VERIFLOW] Fail".encode('utf-8'))
			else:
				print("Wrong input on packet!")
				continue

			print("")
			network.log(affectedEcs)
			msg = None

if __name__ == '__main__':
	main()