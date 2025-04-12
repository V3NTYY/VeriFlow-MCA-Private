from VeriFlow.ForwardingGraph import ForwardingGraph
from VeriFlow.Interval import Interval
from VeriFlow.Trie import Trie
from VeriFlow.Switch import Switch
from VeriFlow.Host import Host
from VeriFlow.Rule import Rule
from VeriFlow.TrieNode import TrieNode
from VeriFlow.NetworkError import NetworkError


class Network(object):
	"""docstring for Network"""
	def __init__(self):
		super(Network, self).__init__()
		self.trie = Trie()
		self.switches, self.hosts, self.rulePrefixes = {}, {}, {}
		self.ecs, self.networkErrors = [], []

	def parseNetworkFromFile(self, fileName):
		f = open(fileName, 'r')
		try:
			f.readline()
			self.addSwitchesFromFileFormat(f)
			self.addHostsFromFileFormat(f)
			self.addRulesFromFileFormat(f)
			f.close()
		except Exception as e: 
			print(e)
			f.close()

	def getNumberOfECs(self):
		return self.trie.numberOfECs()

	def addRulesFromFileFormat(self, f):
		currentLine = f.readline()
		while (not currentLine.startswith("E")):
			currentLine = currentLine.strip('\n')
			ecs = self.addRuleFromString(currentLine)
			currentLine = f.readline()

	def addRuleFromString(self, ruleString):
		print("Adding Rule from string...")
		print(ruleString)
		split1 = ruleString.split("-")
		rule = Rule()
		rule.setSwitchId(split1[0])
		rule.setPrefix(split1[1])
		rule.setNextHopId(split1[2])
		return self.addRule(rule)

	def addHostsFromFileFormat(self, fh):
		currentLine = fh.readline()
		while (not currentLine.startswith("R")):
			ts = Host()
			currentLine = currentLine.strip('\n')
			split1 = currentLine.split(":")
			ts.setId(split1[0])
			ts.setSwitchId(split1[1])
			switch1 = self.switches.get(ts.getSwitchId())
			switch1.addConnectedHost(ts.getId())
			self.hosts[ts.getId()] = ts
			currentLine = fh.readline()

	def addSwitchesFromFileFormat(self, fs):
		currentLine = fs.readline()
		while (not currentLine.startswith("H")):
			ts = Switch()
			currentLine = currentLine.strip('\n')
			split1 = currentLine.split(":")
			ts.setId(split1[0])
			nhops = split1[1].split(",")
			for nh in nhops:
				ts.addNextHop(nh)
			self.switches[ts.getId()] = ts
			currentLine = fs.readline()


	def getSwitches(self):
		return self.switches

	def getHostWithId(self, id):
		return self.hosts.get(id)

	def getHosts(self):
		return self.hosts

	def addEc(self, ec):
		self.getEcs().add(ec)
	

	def checkWellformedness(self, ecs = None):
		Success = True
		print("")
		print("Checking WellFormed-Ness")
		if(ecs is None or len(ecs) == 0): #From ecs is None
			print("ECs is None")
			ecs = self.getECsFromTrie()
		print("ECs", ecs)
		print("Hosts", self.hosts.keys())
		self.getNetworkErrors().clear()
		print("Clearing Network Errors")
		for ec in ecs:
			for host in self.hosts.values():
				connectedSwitchId = host.getSwitchId()
				currentSwitch = self.switches.get(connectedSwitchId)
				forwardingGraph = ForwardingGraph()
				forwardingGraph.addToGraph(connectedSwitchId)
				while (True):
					associatedRule = currentSwitch.getAssociatedRule(ec.getLeft())
					if (associatedRule == None):
						if (len(currentSwitch.getConnectedHosts()) == 0): #Checks to see if there are hosts connected to the switch.
							break
							#should we check to see if the specific host its looking for exists?
							# print("BLACK HOLE FOUND")
							# networkError = NetworkError()
							# networkError.setErrorType(1) #ErrorType.BLACK_HOLE
							# networkError.setEc(ec)
							# networkError.setStartingPoint(self.switches.get(connectedSwitchId))
							# self.getNetworkErrors().append(networkError) #from .add to .append
					nextHopId = associatedRule.getNextHopId()
					if (nextHopId in self.hosts.keys()): #self.hosts.containsKey(nextHopId) #This is not needed, because black holes didnt work (I think?)
						break
					if (forwardingGraph.contains(nextHopId)):
						print("LOOP FOUND")
						networkError = NetworkError()
						networkError.setErrorType(0) #ErrorType.LOOP
						networkError.setEc(ec)
						networkError.setStartingPoint(self.switches.get(connectedSwitchId))
						self.getNetworkErrors().append(networkError) # .add to .append
						Success = False
						break
					currentSwitch = self.switches.get(nextHopId)
					forwardingGraph.addToGraph(nextHopId)
				return Success

	def getEcs(self):
		return self.ecs

	def setEcs(self, ecs):
		self.ecs = ecs
	
	def getNetworkErrors(self):
		return self.networkErrors
	
	def setNetworkErrors(self, networkErrors):
		self.networkErrors = networkErrors
	
	def getTrie(self):
		return self.trie
	
	def setTrie(self, trie):
		self.trie = trie
	
	def addRule(self, rule):
		switch1 = self.switches.get(rule.getSwitchId())
		switch1.addRule(rule)

		if (rule.getPrefix() not in self.rulePrefixes.keys()):
			updateEcs = self.getTrie().addRule(rule)
			self.rulePrefixes[rule.getPrefix()]=1
			return updateEcs
		else:
			self.rulePrefixes[rule.getPrefix()] = self.rulePrefixes.get(rule.getPrefix()) + 1
			return set()

	def deleteRule(self, rule):
		switchId = rule.getSwitchId()
		count = self.rulePrefixes.get(rule.getPrefix())
		try:
			self.rulePrefixes.pop(rule.getPrefix())
			affectedECs = self.trie.deleteFromTrie(rule.getPrefix() + "*")
			if (count != 1):
				self.rulePrefixes.put(rule.getPrefix(), count - 1)
			switch1 = self.switches.get(switchId)
			rules = switch1.getRules()
			for r in rules:
				if r.equals(rule):
					rules.remove(r)

			self.switches[switchId].setRules(rules)
			return affectedECs
			
		except Exception as e:
			return set()
	
	def log(self, affectedEcs):
		print("Number of ECs: " ,self.getNumberOfECs())
		print("Number of affected ECs: " ,len(affectedEcs))
		if (len(self.getNetworkErrors()) == 0):
			print("Network is well-formed (No property violations)")
		else:
			for  netErr in self.getNetworkErrors():
				if (netErr.getErrorType() == 0): #ErrorType.LOOP
					print("There is a loop starting from switch " + netErr.getStartingPoint().getId() + "!")
				if (netErr.getErrorType() == 1): #ErrorType.BLACK_HOLE
					print("There is a Black Hole At " + netErr.getStartingPoint().getId() + "!")

	def deleteRuleFromString(self, ruleString):
		split1 = ruleString.split("-")
		rule = Rule()
		rule.setSwitchId(split1[0])
		rule.setPrefix(split1[1])
		rule.setNextHopId(split1[2])
		return self.deleteRule(rule)

	def generateECs(self):
		return self.trie.generateECs()

	def getECsFromTrie(self):
		return self.trie.getECListFromTrie()

