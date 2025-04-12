
from VeriFlow.Network import Network
import socket
import sys
import threading

ROUTE_VIEW = 1
BIT_BUCKET = 2

import threading

def start_veriflow_server(host, port):
	def handle_client(client_socket):
		try:
			while True:
				message = client_socket.recv(1024).decode('utf-8')
				if not message:
					break
				parse_message(message)

		except Exception as e:
			print(f"Error handling client: {e}")
		finally:
			client_socket.close()

	def server_thread():
		server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		server_socket.bind((host, port))
		server_socket.listen(2)
		print(f"\nVeriFlow server started on {host}:{port}")
		try:
			while True:
				client_socket, addr = server_socket.accept()
				print(f"Connection accepted from {addr}")
				client_handler = threading.Thread(target=handle_client, args=(client_socket,))
				client_handler.start()
		except Exception as e:
			print(f"Server error: {e}")
		finally:
			server_socket.close()

	def parse_message(msg):
		print(f"Received message: {msg}")

	# Create thread for server so we don't stall everything
	server_thread_instance = threading.Thread(target=server_thread)
	server_thread_instance.daemon = True
	server_thread_instance.start()

def main():
	print("Enter network configuration file name (eg.: file.txt):")
	filename = input("> ")
	network = Network()
	network.parseNetworkFromFile(filename)

	## Setup VeriFlow server for CCPDN to pass messages to
	print("Enter IP address to host VeriFlow on (i.e. 127.0.0.1)")
	veriflow_ip = input("> ")
	print("Enter port to host VeriFlow on (i.e. 6655)")
	veriflow_port = int(input("> "))
	start_veriflow_server(veriflow_ip, veriflow_port)

	generatedECs = network.getECsFromTrie()
	network.checkWellformedness()
	network.log(generatedECs)

	while True:
		print(" ")
		print("Add rule by entering A#switchIP-rulePrefix-nextHopIP (eg.A#127.0.0.1-128.0.0.0/2-127.0.0.2)")
		print("Remove rule by entering R#switchIP-rulePrefix-nextHopIP (eg.R#127.0.0.1-128.0.0.0/2-127.0.0.2)")
		print("To exit type exit")

		#inputline = socket.recv().decode()
		#print("Received: ", inputline)
		inputline = input('> ')
		if(inputline is not None):
			affectedEcs = set()
			if (inputline.startswith("A")):
				affectedEcs = network.addRuleFromString(inputline[2:])
				network.checkWellformedness(affectedEcs)
			elif (inputline.startswith("R")):
				affectedEcs = network.deleteRuleFromString(inputline[2:])
				network.checkWellformedness(affectedEcs)
			elif ("exit" in inputline):
				break
			else:
				print("Wrong input format!")
				continue

			print("")
			network.log(affectedEcs)
			inputline = None
		else:
			print("waiting for data...")
			print(" ")

if __name__ == '__main__':
	main()