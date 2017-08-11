#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <pthread.h>
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "bigtable.grpc.pb.h"
#include "bigtable.h"
#include "timer.h"
		

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>


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
using bigtable::Value;
using bigtable::Update;
using bigtable::Empty;
using bigtable::BigTable;
using bigtable::Response;


using std::string;

int server_num; // current server number
string local_address; // server address
string logging_file_name;
string meta_file_name;
long long file_size = 30;
string checkpoint_file_name;
std::vector<string> replica_addr; // without itself 
std::map<string, string> primary_map; // key: user, value:server address
bool logfile_info = false; // contains recover information
std::set<string> down_lst; // down server list
int cur_seq_num; // sequence number
std::vector<Table> cache_table; // table list for different checkpoint file

void RunServer(string addr);
void *timer_worker(void *arg); // timer
std::ifstream::pos_type filesize(string filename); // get file size
bool heartbeat(string ip_addr);
int find_file(string rowkey, string colkey);
void UpdateCheckpoint();
void UpdateMetaData();
void put_value(string row, string col, string* data);
long check_exist(int num, string row, string col);


// sort according to row key and column key
struct Compare {
	bool operator()(const Table& t1, const Table& t2) {
		int rowcompare = t1.first_rowkey.compare(t2.first_rowkey);
		int colcompare = t1.first_colkey.compare(t2.first_colkey);
		if (rowcompare == 0) {
			if (colcompare <= 0) {
				return true;
			} else {
				return false;
			}
		} else {
			if (rowcompare < 0) {
				return true;
			} else {
				return false;
			}
		}
	}
} comp_key;
// sort according to sequence number
struct myCompare {
	bool operator()(const Table& t1, const Table& t2) {
		return t1.sequence_num > t2.sequence_num;
	}
} comp_seq;


class BigTableClient {
  public:
	BigTableClient(std::shared_ptr<Channel> channel)
	  : stub_(BigTable::NewStub(channel)) {
	  	// server_status = true;
	  }

	// tell replica to put value, replica simply put value
	bool PutValue (const string& row, const string& column, const string& val) {
		ClientContext context;
		Value value;
		Empty e;
		value.mutable_pos()->set_row(row);
		value.mutable_pos()->set_col(column);
		value.set_val(val);
		Status status = stub_->PutFromReplica(&context, value, &e);
		if (!status.ok()) {
			return false;
			std::cout << "Now I am primary, put value to replica failed." << std::endl;
		} else {
			return true;
			std::cout << "Now I am primary, put value to replica success." << std::endl;
		}
	}
	// report information to primary
	bool Report(const string& row, const string& column, const string& val) {
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
            return false;
			std::cout << "Now I am replica, report value to primary failed." << std::endl;
		} else {
			std::cout << "Now I am replica, report value to primary success." << std::endl;
			return true;
		}
	}
	// tell replica to update information
	bool UpdatePrimaryInfo(const string& row, const string& column) {
		ClientContext context;
		Position pos;
		Empty e;
		pos.set_row(row);
		pos.set_col(column);
		Status status = stub_->Update_Primary(&context, pos, &e);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            return false;
			std::cout << "Now I become primary, update user primary information to replica failed." << std::endl;
		} else {
			std::cout << "Now I become primary, update user primary information to replica success." << std::endl;
			return true;
		}
	}

	// tell replica server is down
	bool DeliverServerDownStatus(const string& server) {
		ClientContext context;
		Response message;
		Empty e;
		message.set_msg(server);
		Status status = stub_->Update_Sever_Down_Status(&context, message, &e);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            return false;
			std::cout << "Now I am primary, update server status information to replica failed." << std::endl;
		} else {
			std::cout << "Now I am primary, update server status information to replica success." << std::endl;
			return true;
		}
	}


	bool DeliverServerUpStatus(const string& server) {
		ClientContext context;
		Response message;
		Empty e;
		message.set_msg(server);
		Status status = stub_->Update_Sever_Up_Status(&context, message, &e);
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            return false;
			std::cout << "update my status information to others failed." << std::endl;
		} else {
			std::cout << "update my status information to others success." << std::endl;
			return true;
		}
	}

	void Recover (const string& addr) {
		string row, col;
		ClientContext context;
		Response address;
		Value message;
		address.set_msg(addr);
		std::unique_ptr<ClientReader<Value>> reader(
			stub_->ReadLog(&context, address));
		while (reader->Read(&message)) {
			row = message.pos().row();
			col = message.pos().col();
			string* val = new string(message.val());
			put_value(row, col, val);
			printf("recover: row is %s, col is %s, val is %s\n", row.c_str(), col.c_str(), val->c_str());
		}
		Status status = reader->Finish();
		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
			std::cout << "read recover message rpc failed." << std::endl;
		} else {
			std::cout << "read recover message rpc success" << std::endl;
		}
	}

	bool server_status = true;
  private:
	std::unique_ptr<BigTable::Stub> stub_;
};

std::map<string, BigTableClient*> replica_lst;

class BigTableImpl final : public BigTable::Service {
  public:
	explicit BigTableImpl() {
		// if not exist, create new file
    	log_file.open(logging_file_name, std::fstream::in | std::fstream::out | std::fstream::app);
    	log_file.close();
    	meta_file.open(meta_file_name, std::fstream::in | std::fstream::out | std::fstream::app);
    	meta_file.close();
    	// initialize meta file
    	if (filesize(meta_file_name) == 0) {
			meta_file.open(meta_file_name, std::fstream::out);
			string file_name = checkpoint_file_name + "_1.txt";
			meta_file << file_name << ",1,0,a,a\n";
			meta_file.close();
    	}
    	ParseMetaData();
    	ParseLogFile();
    	Synchronize();
    }

    void Synchronize() {
    	for (int i = 0; i < replica_addr.size(); i++) {
    		if (heartbeat(replica_addr[i])){
    			replica_lst[replica_addr[i]]->Recover(local_address);
    			break;
    		}
    	}
    	// broadcast i am back
    	for (int i = 0; i < replica_addr.size(); i++) {
    		if (heartbeat(replica_addr[i])){
    			replica_lst[replica_addr[i]]->DeliverServerUpStatus(local_address);
    		}
    	}
    	printf("Synchronize finish, I am back\n");
    }

    void ParseLogFile() {
    	string row, col;
    	int num;
    	FILE * pFile;
		char * buffer;
		char line[1024];
 		fpos_t position;
		size_t result;
    	pFile = fopen ( logging_file_name.c_str() , "rb" );
		if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
		// get last checkpoint position
		while(fgets(line, 1024, pFile) != NULL) {
			if (strcmp(line, "checkpoint\n") == 0){
  				fgetpos (pFile, &position);
  				continue;
  			}
		  	if (strncmp(line, "127.0.0.1:", 10) == 0) {
		  		continue;
		  	}
			char * pch;
			pch = strtok (line, ",");
			std::vector<string> parse_list;
			while (pch != NULL)
			{
				parse_list.push_back(pch);
				pch = strtok (NULL, ",");
			}
			long lSize = std::stol(parse_list.back());
			buffer = (char*) malloc (sizeof(char)*lSize);
			result = fread (buffer,1,lSize,pFile);
			buffer[result] = '\0';
    		fgetc(pFile);
			if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
			free (buffer);
	 	}
	 	fclose(pFile);	
	 	pFile = fopen ( logging_file_name.c_str() , "rb" );
  		if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
  		fsetpos(pFile, &position);
  		while(fgets(line, 1024, pFile) != NULL) {
		  	printf("read log file: %s", line);
		  	if (strncmp(line, "127.0.0.1:", 10) == 0) {
		  		continue;
		  	}
		  	char * pch;
			pch = strtok (line, ",");
			std::vector<string> parse_list;
			while (pch != NULL)
			{
				parse_list.push_back(pch);
				pch = strtok (NULL, ",");
			}
			row = parse_list[0];
			col = parse_list[1];
			num = find_file(row, col);
			long lSize = std::stol(parse_list.back());
			buffer = (char*) malloc (sizeof(char)*lSize);
		  	result = fread (buffer, 1, lSize, pFile);
		  	buffer[result] = '\0';
    		fgetc(pFile);
		  	if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
		  	string* value = new string(buffer);
			cache_table[num].table[row][col] = value;
	  		free (buffer);
  		}
  		fclose(pFile);
  		printf("parse my log file finish\n");
    }

    void ParseMetaData() {
    	meta_file.open(meta_file_name, std::fstream::binary | std::fstream::in);
    	if (meta_file.is_open()) {
    		string line;
    		while (getline(meta_file, line)) {
    			cache_table.push_back(Table (line));
    		}
    	}
    	// sort(cache_table.begin(), cache_table.end(), comp_seq);
    	// cache_table.front().load_table();
    	for (int i = 0; i < cache_table.size(); i++) {
    		cache_table[i].load_table();
    		printf("first row is %s, first col is %s\n", cache_table[i].first_rowkey.c_str(), cache_table[i].first_colkey.c_str());
    	}
    	cur_seq_num = cache_table.front().sequence_num + 1;
    	// printf("sort according to key\n");
    	// std::cout << cur_seq_num << std::endl;
    	// sort according to keys
    	// for (int i = 0; i < cache_table.size(); i++) {
    	// 	printf("first row is %s, first col is %s\n", cache_table[i].first_rowkey.c_str(), cache_table[i].first_colkey.c_str());
    	// }
    	sort(cache_table.begin(), cache_table.end(), comp_key);
    	meta_file.close();
    	printf("parse meta data finish\n");
    // 	string user;
    // 	for (int i = 0; i < cache_table.size(); i++) {
    // 		printf("now table %d !!!\n", i);
    // 		for (std::map<string,std::map<string,string*> >::iterator it = cache_table[i].table.begin(); it != cache_table[i].table.end(); ++it) {
				// user = it->first;
				// printf("user is %s\n", user.c_str());
				// for (std::map<string,string*>::iterator i = it->second.begin(); i != it->second.end(); ++i) {
				// 	printf("col is: %s\n", i->first.c_str());
				// }
    // 		}
    // 	}
    }
    // record down server
	void record_replica_status (string server, BigTableClient* client) {
		logfile_info = true;
		down_lst.insert(server);
		printf("now my log file contains %s missing info\n", server.c_str());
		this->log_file.open(logging_file_name, std::fstream::app);
		this->log_file << server << "\n";
		this->log_file.close();
		client->server_status = false;
	}
	// deliver down server info to other replica
	void deliver_server_status(string server) {
		for (std::map<string, BigTableClient*>::iterator it = replica_lst.begin(); it != replica_lst.end(); ++it) {
			if (it->second->server_status) {
				it->second->DeliverServerDownStatus(server);
			}
		}
	}
	// broadcast message to all other replica
	void broadcast (string row, string col, string *data) {
		for (std::map<string, BigTableClient*>::iterator it = replica_lst.begin(); it != replica_lst.end(); ++it) {
			if (replica_lst[it->first]->server_status && heartbeat(it->first)) {
				printf("broadcast value to %s\n", it->first.c_str());
				it->second->PutValue(row, col, *data);
			} else {
				if (replica_lst[it->first]->server_status) {
					record_replica_status(it->first, it->second);
					deliver_server_status(it->first);
				}
			}
		}
		put_value(row, col, data);
	}
    // select self as primary
	void select_primary(string user) {
		primary_map[user] = local_address;
		for (std::map<string, BigTableClient*>::iterator it = replica_lst.begin(); it != replica_lst.end(); ++it) {
			if (replica_lst[it->first]->server_status && heartbeat(it->first)) {
				printf("broadcast primary info to %s\n", it->first.c_str());
				it->second->UpdatePrimaryInfo(user, local_address);
			} else {
				if (replica_lst[it->first]->server_status) {
					record_replica_status(it->first, it->second);
					deliver_server_status(it->first);
				}
			}
		}
	}

	Status PUT(ServerContext* context, const Value* value,
                    Empty* e) override {
		// printf("Put value now row is %s col is %s\n", value->pos().row().c_str(), value->pos().col().c_str());
		string user = value->pos().row();
		string col = value->pos().col();
		string* data = new string(value->val());
		if (primary_map.find(user) != primary_map.end()) {
			// connect this primary_map[user]
			if (primary_map[user].compare(local_address) == 0) {
				// broadcast
				broadcast(user, col, data);
			} else {
				// give task to primary
				printf("I am going to deliver this to the primary %s\n", primary_map[user].c_str());
				if (!replica_lst[primary_map[user]]->Report(user, col, *data)) {
					// primary down
					select_primary(user);
					broadcast(user, col, data);
				}
			}
		} else {
			// broadcast primary information
			// ask replica do this.
			printf("I am going to be the primary\n");
			select_primary(user);
			broadcast(user, col, data);
		}
		// broadcast to replica
		// for (int i = 0; i < replica_lst.size(); i++) {
		// 	// replica failed
		// 	if(!replica_lst[i].PutValue(value->pos().row(), value->pos().col(), *data)) {
		// 		replica_lst.erase(i); // remove from list
				
		// 	}
		// }
		
		return Status::OK;
	}

	

	Status Update_Sever_Down_Status(ServerContext* context, const Response* message,
                    Empty* e) override {
		record_replica_status(message->msg(), replica_lst[message->msg()]);
		return Status::OK;
	}

	Status Update_Sever_Up_Status(ServerContext* context, const Response* message,
                    Empty* e) override {
		string server = message->msg();
		logfile_info = false;
		down_lst.erase(server);
		replica_lst[server]->server_status = true;
		return Status::OK;
	}

	Status Update_Primary(ServerContext* context, const Position* pos,
                    Empty* e) override {
		primary_map[pos->row()] = pos->col();
		return Status::OK;
	}


	Status PutFromReplica(ServerContext* context, const Value* value,
                    Empty* e) override {
		string* data = new string(value->val());
		put_value(value->pos().row(),value->pos().col(), data);
		return Status::OK;
	}

	Status GET(ServerContext* context, const Position* pos,
                    Value* value) override {
		auto row_key = pos->row();
		auto col_key = pos->col();
		int num = find_file(row_key, col_key);		
		printf("Get row key: %s and col key: %s from No.%d start\n", row_key.c_str(), col_key.c_str(), num);

		if (cache_table[num].table.find(row_key) != cache_table[num].table.end()) {
			if (cache_table[num].table[row_key].find(col_key) != cache_table[num].table[row_key].end()) {
				value->mutable_pos()->CopyFrom(*pos);
				value->set_val(*cache_table[num].table[row_key][col_key]);
				printf("Get row key: %s and col key: %s from %s succuess\n", row_key.c_str(), col_key.c_str(), cache_table[num].filename.c_str());
				return Status::OK;
			}
			return Status(grpc::NOT_FOUND, "data not available");
		}
		printf("Get row key: %s and col key: %s from No.%d failed\n", row_key.c_str(), col_key.c_str(), num);
		return Status(grpc::NOT_FOUND, "data not available");
		
	}

	Status CPUT(ServerContext* context, const Update* update,
                    Empty* e) override {
		auto row = update->new_val().pos().row();
		auto column = update->new_val().pos().col();
		int num = find_file(row, column);
		if (*cache_table[num].table[row][column] == update->old_val()) {
			string* data = new string (update->new_val().val());
			put_value(row, column, data);
			return Status::OK;
		}
		return Status(grpc::NOT_FOUND, "cannot put this data");
	}

	Status DELETE(ServerContext* context, const Position* pos,
                    Empty* e) override {
        string row = pos->row();
        string col = pos->col();
        int num = find_file(row, col);
        cache_table[num].delete_val(row, col);
		return Status::OK;
	}

	Status Terminate(ServerContext* context, const Empty* e1,
                    Empty* e2) override {
		exit(1);
		return Status::OK;
	}

	Status GetDataList(ServerContext* context, const Empty* e,
					ServerWriter<Response>* writer) override {
		Response file;
		for (int i = 0; i < cache_table.size(); i++) {
			for (string paths : cache_table[i].get_data()) {
				std::cout << "path is " << paths << std::endl;
				file.set_msg(paths);
				writer->Write(file);
			}	
		}
		std::cout << "now read file list finish" << std::endl;
		return Status::OK;
	}	

	Status ReadLog(ServerContext* context, const Response* address,
					ServerWriter<Value>* writer) override {
		string server_addr = address->msg();
		// if (down_lst.find(server_addr) == down_lst.end()) {
		// 	printf("no information for %s recover\n", server_addr.c_str());
		// 	return Status::OK;
		// }
		printf("read missing message from log file\n");
		Value message;
		if (find(replica_addr.begin(), replica_addr.end(), server_addr) != replica_addr.end()) {
			// find this message
			FILE * pFile;
		  	char * buffer;
		  	char line[1024];
		  	size_t result;
		  	string row, col;
		  	fpos_t position;
		  	pFile = fopen ( logging_file_name.c_str(), "rb" );
		  	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

		  	while(fgets(line, 1024, pFile) != NULL) {
				if (strcmp(line, "checkpoint\n") == 0){
					continue;
				}
				if (strncmp(line, "127.0.0.1:", 10) == 0) {
		  			if (strncmp(line, server_addr.c_str(), 14) == 0) {
		  				fgetpos(pFile, &position);
		  			}
		  			continue;
		  		}
				char * pch;
				pch = strtok (line, ",");
				std::vector<string> parse_list;
				while (pch != NULL)
				{
					parse_list.push_back(pch);
					pch = strtok (NULL, ",");
				}
				long lSize = std::stol(parse_list.back());
    			// fgetc(pFile);
				buffer = (char*) malloc (sizeof(char)*(lSize + 1));
				result = fread (buffer,1,lSize,pFile);
				// strcat(buffer, "\0");
				buffer[result] = '\0';
				fgetc(pFile);
				if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
				free (buffer);
			}
			fclose(pFile);


		  	pFile = fopen ( logging_file_name.c_str(), "rb" );
		  	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}
		  	fsetpos(pFile, &position);
			while(fgets(line, 1024, pFile) != NULL) {
				if (strcmp(line, "checkpoint\n") == 0){
					continue;
				}
				if (strncmp(line, "127.0.0.1:", 10) == 0) {
		  			continue;
		  		}
				char * pch;
				pch = strtok (line, ",");
				std::vector<string> parse_list;
				while (pch != NULL)
				{
					parse_list.push_back(pch);
					pch = strtok (NULL, ",");
				}
				row = parse_list[0];
				col = parse_list[1];
				long lSize = std::stol(parse_list.back());
    			// fgetc(pFile);
				buffer = (char*) malloc (sizeof(char)*(lSize + 1));
				// memset(buffer, '\0', sizeof(char)*lSize);
				result = fread (buffer,1,lSize,pFile);
				buffer[result] = '\0';
				fgetc(pFile);
				printf("the message is %s\n", buffer);
				// std::cout << "result" << result << std::endl;
				// std::cout << "lSize is: " << lSize << " buffer len is " << strlen(buffer) << std::endl;
				if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
				// string* value = new string(buffer);
				// printf("send message to the restart server\n");
				Position pos;
				pos.set_row(row);
				pos.set_col(col);
				Value value;
				value.mutable_pos()->CopyFrom(pos);
				string* content = new string(buffer);
				value.set_val(*content);
				writer->Write(value);
				free (buffer);
			}
			fclose(pFile);
		}
		printf("read log file succuess\n");
		return Status::OK;
	}

	Status Check(ServerContext* context, const Empty* ping,
					Empty* pong) override {
		return Status::OK;	
	}

  private:
  std::fstream log_file;
  std::fstream meta_file;

};

int main(int argc, char** argv) {
	std::vector<std::vector<string> >server_replica; // group list
	std::vector<string> server_list; // all servers
	string line;
	std::ifstream myfile(argv[optind]);
	if (myfile.is_open()) {
		std::vector<string> replica;
		while (getline(myfile, line)) {
			char* line_str = strdup(line.c_str());
			char * pch;
			pch = strtok (line_str, ",");
			while (pch != NULL)
			{
				// printf("Add %s to the serverlist\n", pch);
				server_list.push_back(pch);
				replica.push_back(pch);
				pch = strtok (NULL, ",");
			}
			server_replica.push_back(replica);
			replica.clear();	
			free(line_str);
			free(pch);
		}
		myfile.close();
	} else {
		std::cout << "Unable to open serverlist file" <<std::endl;
	}
	optind ++;
	if (optind == argc) {
		exit(1);
	}
	// get address
	server_num = std::stoi(argv[optind]) - 1;
	local_address = server_list[server_num];
	checkpoint_file_name = "server" + std::string(argv[optind]) + "_checkingpoint";
	logging_file_name = "server" + std::string(argv[optind]) + "_logging.txt";
	meta_file_name = "server" + std::string(argv[optind]) + "_metadata.txt";
	// create channel
	for (string r: server_replica[server_num / 3]) {
		if (r.compare(local_address) == 0) {
			continue;
		}
		replica_addr.push_back(r);
		// printf("replica: %s\n", r.c_str());
		replica_lst[r] = new BigTableClient ( grpc::CreateChannel(r,
  				grpc::InsecureChannelCredentials()));
	}
	RunServer(local_address);
	return 0;
}


void RunServer(string addr) {
	std::string server_address(addr);
  	BigTableImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	// pthread_t timer_thread;
	// pthread_create(&timer_thread, NULL, timer_worker, &service);
	std::cout << "Server listening on " << server_address << std::endl;
	server->Wait();
}

void *timer_worker(void *arg) {
	// BigTableImpl* service = (BigTableImpl*) arg;
	unsigned long seconds = 60;
	Timer t;
	t.start();
	std::cout << "timer started . . ." << std::endl;
	while(true) {
		if(t.elapsedTime() >= seconds) {
			UpdateCheckpoint();
			t.start();
		}
	}
}


std::ifstream::pos_type filesize(string filename) {
	std::streampos begin, end;
	std::ifstream myfile (filename, std::ios::binary);
  	begin = myfile.tellg();
  	myfile.seekg (0, std::ios::end);
  	end = myfile.tellg();
  	myfile.close();
  	std::cout << "size is: " << (end-begin) << " bytes.\n";
  	return (end-begin);
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


int find_file(string rowkey, string colkey) {
	// int num;
	if (cache_table.size() == 1) {
		return 0;
	}
	Table temp_table (rowkey, colkey);
	int start = 0, end = cache_table.size() - 1;
	while (start + 1 < end) {
		int mid = (start + end) / 2;
		if (comp_key(cache_table[mid], temp_table)) {
			start = mid;
		} else {
			end = mid;
		}
	}
	if (cache_table[start].equals(temp_table)){return start;}
	if (cache_table[end].equals(temp_table)) {return end;}
	if (comp_key(cache_table[start], temp_table)) {
		return start;
	} else {
		return end;
	}
	// if table is not loaded.
	// if (!cache_table[num].loaded) {
	// 	cache_table[num].load_table();
	// }
	// return num;
}



void UpdateCheckpoint(){
	std::fstream log_file;
	for (int i = 0; i < cache_table.size(); i++) {
		printf("now update checkpoint_file\n");
		cache_table[i].update_file();
	}
	// if (logfile_info) {
	if (down_lst.size() > 0) {
		log_file.open(logging_file_name, std::fstream::app);
	} else {
		log_file.open(logging_file_name, std::fstream::out);
	}
	log_file << "checkpoint\n";
	log_file.close();
}

void UpdateMetaData() {
	std::fstream meta_file;
	meta_file.open(meta_file_name, std::fstream::out);
	for (int i = 0; i < cache_table.size(); i++) {
		// cache_table[i].update_first_line();
		meta_file << cache_table[i].filename << "," << std::to_string(cache_table[i].sequence_num)\
		 << "," << std::to_string(cache_table[i].size) << "," << cache_table[i].first_rowkey\
		 << "," << cache_table[i].first_colkey << "\n";
		 // printf("name is %s\n", cache_table[i].filename.c_str());
	}
	meta_file.close();
	printf("Update meta data file succuess\n");
}

void put_value(string row, string col, string* data) {
	std::fstream log_file;
	int num = find_file(row, col);
	long old_size = check_exist(num, row, col); 
	cache_table[num].table[row][col] = data;
	// printf("write log file\n");
	log_file.open(logging_file_name, std::fstream::app);
	log_file << row << "," << col << "," << data->size() << "\n";
	log_file << *data << "\n";
	log_file.close();
	// printf("split file start\n");
	if (old_size == -1) {
		std::cout << "server number is " << num << " size is " << cache_table[num].size << std::endl;
		if (cache_table[num].size > file_size) {
			string file_name = checkpoint_file_name + "_" + std::to_string(cache_table.size() + 1) + ".txt";
			cache_table.push_back(cache_table[num].split_file(file_name, cur_seq_num));
			// printf("splif file finish\n");
    		sort(cache_table.begin(), cache_table.end(), comp_key);
    		for (int i = 0; i < cache_table.size(); i++) {
    			std::cout << "table name is !!::" << cache_table[i].filename << "first_rowkey " << cache_table[i].first_rowkey << std::endl;
    		}
    		// std::cout << "compare result is :" << comp_key(cache_table[0], cache_table[1]) << std::endl;
    		// printf("sort according to key finish\n");
    		UpdateMetaData();
    		UpdateCheckpoint();
		}
		cache_table[num].size += data->size();
	} else {
		cache_table[num].size += (data->size() - old_size);
	}
	printf("Put row key: %s and col key: %s in %s succuess\n", row.c_str(), col.c_str(), cache_table[num].filename.c_str());
}

long check_exist(int num, string row, string col) {
	if (cache_table[num].table.find(row) != cache_table[num].table.end()) {
		if (cache_table[num].table[row].find(col) != cache_table[num].table[row].end()) {
			return cache_table[num].table[row][col]->size();
		}
	}
	return -1;
}
