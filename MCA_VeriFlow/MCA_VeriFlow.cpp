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

#ifdef __unix__
    std::vector<double> MCA_VeriFlow::measure_tcp_connection(const std::string& host, int port, int num_pings) {
        std::vector<double> rtts;
        for (int i = 0; i < num_pings; i++) {
                int sockfd;
                struct sockaddr_in server_addr;
                struct hostent* server;
                auto start_time = std::chrono::high_resolution_clock::now();
                // create socket and text connection
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    std::cerr << "Socket creation failed.\n";
                    return {};
                }

                server = gethostbyname(host.c_str());
                if (server == nullptr) {
                    std::cerr << "Error: No such host.\n";
                    close(sockfd);
                    return {};
                }

                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
                server_addr.sin_port = htons(port);

                if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    std::cerr << "Connection failed.\n";
                    close(sockfd);
                    return {};
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> rtt = end_time - start_time;
                rtts.push_back(rtt.count());

                close(sockfd);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return rtts;
    }
#endif

#ifdef __unix__
    // Function to test TCP connection time
    double MCA_VeriFlow::test_tcp_connection_time(const std::string& host, int port, int timeout) {
            int sockfd;
            struct sockaddr_in server_addr;
            struct hostent* server;
            auto start_time = std::chrono::high_resolution_clock::now();
            // create socket and text connection
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                std::cerr << "Socket creation failed.\n";
                return -1;
            }

            server = gethostbyname(host.c_str());
            if (server == nullptr) {
                std::cerr << "Error: No such host.\n";
                close(sockfd);
                return -1;
            }

            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
            server_addr.sin_port = htons(port);

            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Connection failed.\n";
                close(sockfd);
                return -1;
            }
        
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> connection_time = end_time - start_time;
            close(sockfd);
            return connection_time.count();
    }
#endif

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
    std::cout << "This app only runs on UNIX systems due to specific socket libraries. Most things won't work." << std::endl;
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
            std::cout << "Commands:" << std::endl <<
                " * help:                                   Display all commands and their parameters." << std::endl <<
                " * run:                                    Start the CCPDN App. Controller must be linked, and topology initialized." << std::endl <<
                " * stop:                                   Stop the CCPDN App." << std::endl <<
                " * list-devices:                           List all the devices connected to the Pox Controller." << std::endl <<
                " * list-flows:                             List all the flows in the flow table." << std::endl <<
                " * link-controller [ip-address] [port]:    Link a currently running Pox Controller to this app." << std::endl <<
                " * unlink-controller:                      Free the Pox Controller from this app." << std::endl <<
                " * add-flow [flow_id] [eth_type], [protocol] [source-ip] [destination-ip] [source-port] [destination-port] [action]" << std::endl <<
                "                                           Add a flow to the flow table." << std::endl <<
                " * del-flow:                               Delete a flow from the flow table." << std::endl <<
                " * run-tcp-test                            Run's the TCP connection setup latency test." << std::endl <<
                "" << std::endl;
        }

        // link-controller command
        else if (args.at(0) == "link-controller") {
            if (args.size() < 3) {
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

        else if (args.at(0) == "unlink-controller") {
            if (!controller_linked) {
				std::cout << "Controller not linked!" << std::endl;
			}
			else {
                // TODO: Stop any services involving listening to the controller
                // TODO: Destroy references with current controller
				controller_linked = false;
                topology_initialized = false;
			}
        }

        else if (args.at(0) == "run-tcp-test") {
            std::cout << "Running TCP test..." << std::endl;
            #ifdef __unix__
                mca_veriflow->measure_tcp_connection("google.com", 80, 10);
            #endif
        }

        // Invalid response
        else {
            std::cout << "Invalid command" << std::endl;
        }
    }

    return 0;
}