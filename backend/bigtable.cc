#include "bigtable.h"

using std::string;

Table::Table() {
}

Table::Table (string line) {
	char * pch;
	char* line_cstr = strdup(line.c_str());
	pch = strtok (line_cstr, ",");
	std::vector<string> parse_list;
	while (pch != NULL)
	{
		parse_list.push_back(pch);
		pch = strtok (NULL, ",");
	}
	this->filename = parse_list[0];
	this->sequence_num = stoi(parse_list[1]);
	this->size = stoll(parse_list[2]);
	this->first_rowkey = parse_list[3];
	this->first_colkey = parse_list[4];
	this->loaded = false;
  	std::fstream checkpoint_file;
	checkpoint_file.open(this->filename, std::fstream::in | std::fstream::out | std::fstream::app);
    checkpoint_file.close();
}

Table::Table (string row, string col) {
	this->first_rowkey = row;
	this->first_colkey = col;
}

void Table::load_table() {
	// string line, row, col;
 //  	std::fstream checkpoint_file;
	// checkpoint_file.open(filename, std::fstream::binary | std::fstream::in);
	// std::vector<char>* buffer = new std::vector<char>((std::istreambuf_iterator<char>(checkpoint_file)), 
	// 		(std::istreambuf_iterator<char>()));
	// checkpoint_file.close();
	// checkpoint_file.open(filename, std::fstream::binary | std::fstream::in);
	// if (checkpoint_file.is_open()) {
	// 	std::vector<char>::iterator start, end;
	// 	while (checkpoint_file.peek() != std::fstream::traits_type::eof()){
	// 		char* line = new char [1024];
	// 		checkpoint_file.getline(line, 1024);
	// 		printf("loading this line: %s\n", line);
	// 		std::cout << strlen(line) << std::endl;
	// 		start = buffer->begin() + strlen(line) + 1;
	// 		char * pch;
	// 		pch = strtok (line, ",");
	// 		std::vector<string> parse_list;
	// 		while (pch != NULL)
	// 		{
	// 			parse_list.push_back(pch);
	// 			pch = strtok (NULL, ",");
	// 		}
	// 		row = parse_list[0];
	// 		col = parse_list[1];
	// 		int size = std::stoi(parse_list.back());
	// 		end = start + size;
	// 		// char* value = new char [size+1];
	// 		// std::filebuf* pbuf = checkpoint_file.rdbuf();
	// 		// pbuf->sgetn(value,size);
	// 		// for (int i = 0; i < size; i++) {
	// 		// 	*(value + i) = checkpoint_file.get();
	// 		// }
	// 		// checkpoint_file.get(value, size+1);
	// 		// checkpoint_file.read(value, std::streamsize(size));
	// 		// std::stringstream myStream(value);
	// 		string* value_str = new string(start, end);
	// 		start = end + 1;
	// 		table[row][col] = value_str;
	// 		checkpoint_file.seekg(1+size, checkpoint_file.cur);
	// 		delete[] line;
	// 		// delete[] value;
	// 	}
	// }
	// checkpoint_file.close();
	// delete buffer;
	string row, col;
	FILE * pFile;
  	char * buffer;
  	char line[1024];
  	size_t result;

  	pFile = fopen ( filename.c_str(), "rb" );
  	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

	while(fgets(line, 1024, pFile) != NULL) {
		// printf("%s\n", line);
		if (strcmp(line, "checkpoint") == 0) continue;
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
		buffer = (char*) malloc (sizeof(char)*lSize);
		result = fread (buffer,1,lSize,pFile);
		buffer[result] = '\0';
		fgetc(pFile);
		if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
		string* value = new string(buffer);
		table[row][col] = value;
		free (buffer);
	}
	printf("load finish\n");
	loaded = true;
}

void Table::evict_table() {
	std::string user;
	for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
		user = it->first;
		for (std::map<string,string*>::iterator i = it->second.begin(); i != it->second.end();) {
			delete i->second;
			table[user].erase(i++);
		}
		table.erase(user);
	}
	loaded = false;
}

void Table::update_file() {
  	std::fstream checkpoint_file;
	checkpoint_file.open(filename, std::fstream::out);
	string user;
	for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
		user = it->first;
		for (std::map<string,string*>::iterator i = it->second.begin(); i != it->second.end(); ++i) {
			if (i->second->compare("$deleted$") == 0) {
				continue;
			}
			checkpoint_file << user << "," << i->first << "," << i->second->size() << "\n";
			std::cout << "update checkpoint file :" << *i->second << std::endl;
			checkpoint_file << *i->second << "\n";
		}
	}
    checkpoint_file.close();
    this->sequence_num ++;
}

Table Table::split_file(string name, int sequence) {
	std::string user;
	// for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
	// 	user = it->first;
	// 	printf("user is %s\n", user.c_str());
	// 	for (std::map<string,string*>::iterator i = it->second.begin(); i != it->second.end(); ++i) {
	// 		printf("col is: %s\n", i->first.c_str());
	// 	}
 //    }
  	std::map<std::string, std::map<std::string, std::string*> > new_table;
  	user = table.rbegin()->first;
  	// printf("get last user%s\n", user.c_str());
  	std::string col = table[user].rbegin()->first;
  	// printf("add value to new table \n");
  	new_table[user][col] = table[user][col];
  	// printf("get value size\n");
	long long new_size = table[user][col]->size();
	// printf("erase value in old table\n");
  	table[user].erase(col);
  	if (table[user].empty()) {
  		table.erase(user);
  	}
  // 	for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
		// user = it->first;
		// for (std::map<string,string*>::iterator i = it->second.begin(); i != it->second.end();) {
		// 	if (count > size) {
		// 		printf("copy table %s\n", i->first.c_str());
		// 		new_count += i->second->size();
		// 		new_table[user][i->first] = i->second;
		// 		table[user].erase(i++);
		// 	} else {
		// 		count += i->second->size();
		// 		++i;
		// 	}
		// }
  //   }
  	// printf("create new table object\n");
    this->size -= new_size; 
    Table new_t;
    new_t.table = new_table;
    new_t.filename = name;
    new_t.sequence_num = sequence;
    new_t.size = new_size;
    new_t.loaded = true;
    new_t.update_first_line();
  	std::fstream checkpoint_file;
    checkpoint_file.open(name, std::fstream::in | std::fstream::out | std::fstream::app);
    checkpoint_file.close();
    update_first_line();
    return new_t;
}


void Table::update_first_line() {
	this->first_rowkey = this->table.begin()->first;
    this->first_colkey = this->table[this->first_rowkey].begin()->first;
}

bool Table::equals(Table t){
	return (t.first_rowkey.compare(this->first_rowkey) == 0) && (t.first_colkey.compare(this->first_colkey) == 0);
}

void Table::delete_val(string row, string col) {
	delete table[row][col];
	table[row].erase(col);
	if (table[row].empty()) {
  		table.erase(row);
  	}
}

// std::vector<string> Table::get_data() {
// 	std::vector<string> file_lst;
// 	std::string user;
// 	for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
// 		user = it->first;
// 		if (*(it->second).find("email:receive") != *(it->second).end()) {
// 			file_lst.push_back(*(it->second)["email:receive"]);
// 		}
// 		if (*(it->second).find("email:send") != *(it->second).end()) {
// 			file_lst.push_back(*(it->second)["email:send"]);
// 		}
// 		if (*(it->second).find("files:paths") != *(it->second).end()) {
// 			file_lst.push_back(*(it->second)["files:paths"]);
// 		}
// 	}
// 	// std::cout << "one table finish" << std::endl;
// 	return file_lst;
// }
std::vector<string> Table::get_data() {
 std::vector<string> file_lst;
 std::string user;
 for (std::map<string,std::map<string,string*> >::iterator it = table.begin(); it != table.end(); ++it) {
  user = it->first;
  if (*(it->second).find("email:receive") != *(it->second).end()) {
   file_lst.push_back(user + ",email:receive," + *(it->second)["email:receive"]);
  }
  if (*(it->second).find("email:send") != *(it->second).end()) {
   file_lst.push_back(user + ",email:send," + *(it->second)["email:send"]);
  }
  if (*(it->second).find("files:paths") != *(it->second).end()) {
   file_lst.push_back(user + ",files:paths," + *(it->second)["files:paths"]);
  }
 }
 // std::cout << "one table finish" << std::endl;
 return file_lst;
}

