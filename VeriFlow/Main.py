
from VeriFlow.Network import Network
import socket
import sys

ROUTE_VIEW = 1
BIT_BUCKET = 2


def main():
	print("Enter network configuration file name (eg.: file.txt):")
	#filename = input("> ")
	filename = "Topo/Small-Topo.txt"
	network = Network()
	network.parseNetworkFromFile(filename)
	#print("Enter IP address of the Controller")
	#ip = input("> ")
	#print("Enter Port for controller")
	#port = input("> ")

	#socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	#socket.connect(ip, port)

	generatedECs = network.getECsFromTrie()
	network.checkWellformedness()
	network.log(generatedECs)

	while True:
		print(" ")
		print("Add rule by entering A#switchIP-rulePrefix-nextHopIP (eg.A#127.0.0.1-128.0.0.0/2-127.0.0.2)")
		print("Remove rule by entering R#switchIP-rulePrefix-nextHopIP (eg.R#127.0.0.1-128.0.0.0/2-127.0.0.2)")
		print("To exit type exit")

		#inputline = socket.recv().decode()
		#print("Recieved: ", inputline)
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