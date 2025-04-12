
from VeriFlow.Network import Network
import socket
import sys
import threading

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
		if "[CCPDN]" in message:
			## Send hello back if we receive hello
			if "Hello" in message:
				print("\nReceived hello message from CCPDN!")
				client_socket.send("[VERIFLOW] Hello".encode('utf-8'))
			## Handle logic for a flow rule added
			elif "FLOW" in message:
				## Only parse characters after the text "[CCPDN] FLOW "
				print("\nReceived FLOW Mod from CCPDN!")
				message = message[13:].strip().rstrip('\x00')
				msg = message
				pingFlag.set()

	# Create thread for server so we don't stall everything
	server_thread_instance = threading.Thread(target=server_thread)
	server_thread_instance.daemon = True
	server_thread_instance.start()

def compare_strings(manual_input, server_msg):

	print("=== Comparing Strings ===")
	print("Manual Input: '{}'".format(manual_input))
	print("Server Message: '{}'".format(server_msg))

	# Strip whitespace for a fair comparison
	manual_input = manual_input.strip()
	server_msg = server_msg.strip()

	# Direct comparison
	if manual_input == server_msg:
		print("Strings are identical.")
	else:
		print("Strings are different.")

	# Byte-level comparison
	manual_bytes = manual_input.encode('utf-8')
	server_bytes = server_msg.encode('utf-8')
	if manual_bytes == server_bytes:
		print("Strings are identical at the byte level.")
	else:
		print("Strings differ at the byte level.")
		print("Manual Input Bytes: {}".format(manual_bytes))
		print("Server Message Bytes: {}".format(server_bytes))

	# Character-by-character comparison
	print("\nCharacter-by-Character Comparison:")
	max_len = max(len(manual_input), len(server_msg))
	for i in range(max_len):
		char1 = manual_input[i] if i < len(manual_input) else None
		char2 = server_msg[i] if i < len(server_msg) else None
		if char1 != char2:
			print("Difference at position {}: '{}' != '{}'".format(i, char1, char2))

	# Length comparison
	if len(manual_input) != len(server_msg):
		print("\nStrings have different lengths: {} != {}".format(len(manual_input), len(server_msg)))

	print("=== Comparison Complete ===")

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
	filename = input("> ")
	network = Network()
	network.parseNetworkFromFile(filename)

	print("Enter the first test rule")
	testInput = input("> ")

	## Setup VeriFlow server for CCPDN to pass messages to
	print("Enter IP address to host VeriFlow on (i.e. 127.0.0.1)")
	veriflow_ip = input("> ")
	print("Enter port to host VeriFlow on (i.e. 6655)")
	veriflow_port = int(input("> "))
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
			print("\nPARSED CCPDN MSG: '{}'".format(msg))
			compare_strings(testInput, msg)
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