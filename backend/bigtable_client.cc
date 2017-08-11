#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "bigtable.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using bigtable::Position;
using bigtable::Value;
using bigtable::Update;
using bigtable::Empty;
using bigtable::Response;
using bigtable::BigTable;


using std::string;


#define logVerbose(a...) do { if (verbose) { printf(a); printf("\n"); } } while(0)

class BigTableClient {
  public:
	BigTableClient(std::shared_ptr<Channel> channel)
	  : stub_(BigTable::NewStub(channel)) {}

	// string FindIp(const string& row, const string& column) {
	// 	ClientContext context;
	// 	Position pos;
	// 	Response addr;
	// 	pos.set_row(row);
	// 	pos.set_col(column);
	// 	Status status = stub_->Find(&context, pos, &addr);
	// 	if (!status.ok()) {
	// 		std::cout << "Getvalue rpc failed." << std::endl;
	// 	} else {
	// 		std::cout << "value is:" << addr.msg() << std::endl;
	// 		return addr.msg();
	// 	}
	// }

	void PutValue (const string& row, const string& column, const string& val) {
		ClientContext context;
		Value value;
		Empty e;
		value.mutable_pos()->set_row(row);
		value.mutable_pos()->set_col(column);
		value.set_val(val);
		Status status = stub_->PUT(&context, value, &e);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "put value rpc failed." << std::endl;
		} else {
			std::cout << "put value rpc success." << std::endl;
		}
	}

	void GetValue (const string& row, const string& column) {
		ClientContext context;
		Position pos;
		Value value;
		pos.set_row(row);
		pos.set_col(column);
		Status status = stub_->GET(&context, pos, &value);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "Getvalue rpc failed." << std::endl;
		} else {
			std::cout << "value is:" << value.val() << std::endl;
		}
	}

	void terminate() {
		ClientContext context;
		Empty e1, e2;
		Status status = stub_->Terminate(&context, e1, &e2);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "rpc failed." << std::endl;
		} else {
			std::cout << "Terminate server success" << std::endl;
		}
	}
	// void ReadLog () {
	// 	ClientContext context;
	// 	Empty e;
	// 	Response message;
	// 	std::unique_ptr<ClientReader<Response>> reader(
	// 		stub_->ReadLog(&context, e));
	// 	while (reader->Read(&message)) {
	// 		std::cout << message.msg() << std::endl;
	// 	}
	// 	Status status = reader->Finish();
	// 	if (!status.ok()) {
	// 		std::cout << status.error_code() << ": " << status.error_message()
 //                << std::endl;
	// 		std::cout << "readlog rpc failed." << std::endl;
	// 	} else {
	// 		std::cout << "success" << std::endl;
	// 	}
	// }

	void FindIpList() {
		std::vector<string> replica;
		ClientContext context;
		Position pos;
		Response message;
		std::unique_ptr<ClientReader<Response>> reader(
			stub_->Find(&context, pos));
		while (reader->Read(&message)) {
			replica.push_back(message.msg());
		}
		Status status = reader->Finish();
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "readlog rpc failed." << std::endl;
		} else {
			std::cout << "success" << std::endl;
		}
	} 



	std::vector<string> getAllIpAddress() {
      	std::vector<string> ip_list;
        ClientContext context;
        Empty e;
        Response message;
        std::unique_ptr<ClientReader<Response>> reader(
            stub_->GetIpList(&context, e));
        // ServletManager* manager = ServletManager::getInstance();
        // std::vector<std::vector<string> >* ip_list = manager->getIpList();
        while (reader->Read(&message)) {
        	std::cout << message.msg() << std::endl;
            ip_list.push_back(message.msg());
        }
        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            std::cout << "get ip list rpc failed." << std::endl;
        } else {
            std::cout << "get ip list rpc success" << std::endl;
        }
        return ip_list;
    }

    void deleteValue(const string& row, const string& column) {
		ClientContext context;
		Value value;
		Empty e;
		value.mutable_pos()->set_row(row);
		value.mutable_pos()->set_col(column);
		value.set_val("$deleted$");
		Status status = stub_->PUT(&context, value, &e);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "delete value rpc failed." << std::endl;
		} else {
			std::cout << "delete value rpc success." << std::endl;
		}
	}

	std::vector<string> getAllData() {
      	std::vector<string> file_list;
        ClientContext context;
        Empty e;
        Response message;
        std::unique_ptr<ClientReader<Response>> reader(
            stub_->GetDataList(&context, e));
        while (reader->Read(&message)) {
        	std::cout << message.msg() << std::endl;
            file_list.push_back(message.msg());
        }
        Status status = reader->Finish();
        std::cout << "here" << std::endl;
        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            std::cout << "get ip list rpc failed." << std::endl;
        } else {
            std::cout << "get ip list rpc success" << std::endl;
        }
        return file_list;
    }


  private:
	std::unique_ptr<BigTable::Stub> stub_;
};



int main(int argc, char** argv) {
	string address = argv[optind]; 
  BigTableClient bigtable(
      grpc::CreateChannel(address,
                          grpc::InsecureChannelCredentials()));
  std::vector<string> ip_list = bigtable.getAllIpAddress();
  

  	// bigtable.PutValue("abc","file1","asdfghjkl");
  	// bigtable.PutValue("abc","file2","dkjfnajkdf");
  	// bigtable.PutValue("abc","file3","asddjhfjdjfjdfffdfdfddkjhasjkvdaksjfhndaskljfblasdfghjkl");
  	// bigtable.PutValue("abc","file4","dhfjkadhfdkasjhdjkf");
  	//  while (true) {
  	// 	std::cout << "-------------- PutValue --------------" << std::endl;
  	// 	string row, col, data;
  	// 	std:: cin >> row;
  	// 	std:: cin >> col;
  	// 	std:: cin >> data;
  	// 	bigtable.PutValue(row, col, data);
  	// }
  	// bigtable.GetValue("jiawei", "password");
  	// bigtable.GetValue("xuyimeng", "password");
  	// bigtable.GetValue("jiawei", "email:receive");
  	// sleep(10*1000);
  	// bigtable.terminate();
  	
  	// bigtable.GetValue("abc","file7");
  	// bigtable.GetValue("abc","file2");
  	// bigtable.GetValue("abc","file3");
  	// bigtable.GetValue("abc","file4");
  	bigtable.getAllData();

  	exit(1);
  	
  	// std::map<string, BigTableClient*> server_map;
  	// while (true) {
  	// 	std::cout << "-------------- PutValue --------------" << std::endl;
  	// 	string row, col, data;
  	// 	std:: cin >> row;
  	// 	std:: cin >> col;
  	// 	std:: cin >> data;
  	// 	string ip_addr = bigtable.FindIp(row, col);
  	// 	std::cout << "ip address is :" << ip_addr << std::endl;
  	// 	if (server_map.find(ip_addr) == server_map.end()) {
  	// 		server_map[ip_addr] = new BigTableClient ( grpc::CreateChannel(ip_addr,
  	// 			grpc::InsecureChannelCredentials()));
  	// 	}
  	// 	std::cout << "-------------- Getvalue --------------" << std::endl;
  	// 	server_map[ip_addr]->PutValue(row, col, data);
  	// 	std:: cin >> row;
  	// 	std:: cin >> col;
  	// 	server_map[ip_addr]->GetValue(row, col);
  	// }

  return 0;
}


