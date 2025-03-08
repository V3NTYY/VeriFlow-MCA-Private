#include "MCA_VeriFlow.h"

void MCA_VeriFlow::run() {

}

void MCA_VeriFlow::stop() {

}

/*
* Implement all below commands:
* 
* help: Display the all commands and their descriptions
* link-controller: Link a currently running Pox Controller to this app
* 
* The following commands should only work when a controller is linked to the app
* They should be able to be utilized while the service is running (use a thread!):
* 
* run: Start the CCPDN App.
* stop: Stop the CCPDN App. This command should only work if the CCPDN App is running.
* list-devices: List all the devices connected to the Pox Controller
* list-flows: List all the flows in the flow table
* link-controller: Link a currently running Pox Controller to this app
* unlink-controller: Free the Pox Controller from this app
* add-flow: Add a flow to the flow table
* del-flow: Delete a flow from the flow table
*/


int main(int argc, char* argv[]) {
	MCA_VeriFlow* mca_veriflow = new MCA_VeriFlow();
	// TODO: Implement command-line args handler

	// EXAMPLE: Start the CCPDN App
	// mca_veriflow->run();
	return 0;
}

// Empty Constructor and Destructor
MCA_VeriFlow::MCA_VeriFlow() {
}

MCA_VeriFlow::~MCA_VeriFlow() {
}