#include "MCA_VeriFlow.h"
#include "Controller.h"

// Function to split a string into a vector of words
std::vector<std::string> splitInput(std::string input) {
    std::vector<std::string> words;
    std::istringstream stream(input);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

void MCA_VeriFlow::run() {

}

void MCA_VeriFlow::stop() {

}

/*
* Implement all below commands:
* 
* help: Display the all commands and their descriptions
* link-controller: Link a currently running Pox Controller to this app
* init-top: Identifys all local domain nodes, have them transmit a handshake to neighboring controllers.
* 
* The following commands should only work when:
* A controller is linked to the app, and the topology is initialized with init-top.
* 
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


int main() {
    MCA_VeriFlow* mca_veriflow = new MCA_VeriFlow();
    Controller* c = new Controller();

    #ifdef _WIN32
    std::cout << "This app only runs on UNIX systems due to specific socket libraries. Exiting..." << std::endl;
    return 0;
    #endif

    bool controller_linked = false;
    bool topology_initialized = false;
   
    while (true) {

        std::string input;
        std::cout << ">>> ";
        std::getline(std::cin, input);
        std::cout << std::endl;
        if (input == "exit") {
			break;
		}

        // Parse args
        std::vector<std::string> args = splitInput(input);
        if (args.size() == 0) {
			std::cout << "Invalid command" << std::endl;
		}

        // Help command
        else if (args.at(0) == "help") {
            std::cout << "Help command -- needs more info" << std::endl;
        }

        // link-controller command
        else if (args.at(0) == "link-controller") {
            if (args.size() < 2) {
				std::cout << "Not enough arguments. Usage: link-controller [ip-address] [port]" << std::endl;
			}
            else if (controller_linked) {
                std::cout << "Controller already linked. Try unlink-controller first" << std::endl;
            }
            else {
				c->setControllerIP(args.at(1), args.at(2));
				controller_linked = c->start();
			}
        }

        // init-top command
        else if (args.at(0) == "init-top") {
            if (!controller_linked) {
                std::cout << "Controller not linked. Try link-controller first" << std::endl;
            }
            else {
				std::cout << "Initializing topology..." << std::endl;
				topology_initialized = true;
			}
        }

        // Invalid response
        else {
            std::cout << "Invalid command" << std::endl;
        }
    }

    return 0;
}