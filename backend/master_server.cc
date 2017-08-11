#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <arpa/inet.h>
#include <fstream>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "bigtable.grpc.pb.h"
// #include "bigtable.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "timer.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using bigtable::Position;
// using bigtable::Value;
// using bigtable::Update;
using bigtable::Empty;
using bigtable::BigTable;
using bigtable::Response;

using std::string;

#define logVerbose(a...) do { if (verbose) { printf(a); printf("\n"); } } while(0)

string serverlist_filename;

// void ParseDb(std::map<string, std::map<string, string>>& table);
void RunServer();
bool heartbeat(string ip_addr);

class BigTableImpl final : public BigTable::Service {
  public:
	explicit BigTableImpl() {
		ParseConfiguration();
    }

    void ParseConfiguration() {
    	string line;
    	std::vector<string> replica;
		std::ifstream myfile(serverlist_filename);
		if (myfile.is_open()) {
			while (getline(myfile, line)) {
				char* line_str = strdup(line.c_str());
				char * pch;
				pch = strtok (line_str, ",");
				while (pch != NULL)
				{
					// printf("Add %s to the serverlist\n", pch);
					replica.push_back(pch);
					pch = strtok (NULL, ",");
				}	
				server_list.push_back(replica);
				replica.clear();
				free(line_str);
				free(pch);
			}
			myfile.close();
		} else {
			std::cout << "Unable to open file" << std::endl;
		}	
    }

    std::vector<string> find_server(char key) {
    	// std::cout << server_list[(key - 'a') / 7] << std::endl;
    	return server_list[(key - 'a') % 4];
    }

	// Status Find(ServerContext* context, const Position* pos,
 //                    Response* addr) override {
	// 	string user = pos->row();
	// 	std::cout << user << std::endl;
	// 	// addr->set_msg(find_server(user[0]));
	// 	return Status::OK;
	// }
    Status Find(ServerContext* context, const Position* pos,
					ServerWriter<Response>* writer) override {
		string user = pos->row();
		Response addr;
		for (auto i : find_server(user[0])) {
			addr.set_msg(i);
			writer->Write(addr);
		}
		return Status::OK;
	}

	Status GetIpList(ServerContext* context, const Empty* e,
					ServerWriter<Response>* writer) override {
		Response addr;
		for (auto list : server_list) {
			for (auto ip : list) {
				if (heartbeat(ip)) {
					addr.set_msg(ip + ",1");
				} else {
					addr.set_msg(ip + ",0");
				}
				writer->Write(addr);
			}
		}
		return Status::OK;
	}	

  private:
  std::vector<std::vector<string> > server_list;
};


int main(int argc, char** argv) {
	serverlist_filename = argv[optind];
	RunServer();
	return 0;
}


void RunServer() {
	std::string server_address("0.0.0.0:5000");
  	BigTableImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;
	server->Wait();
}


bool heartbeat(string ip_addr) {
	size_t found = ip_addr.find_first_of(':');
	string port_num = ip_addr.substr(found + 1);
	int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Cannot open socket (%s)\n", strerror(errno));
        exit(1);
    }
    // connect to the server
    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(stoi(port_num));
    inet_pton(AF_INET, ip_addr.c_str(), &(dest.sin_addr));
    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
    	printf("server %s is dead\n", port_num.c_str());
    	return false;
    } 
    printf("server %s is alive\n", port_num.c_str());
    return true;
}


