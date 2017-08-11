#ifndef BIGTABLE_H
#define BIGTABLE_H
#include <map>
#include <string>
#include <string.h>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>


class Table {
  public:
  	std::map<std::string, std::map<std::string, std::string*> > table;
  	std::string filename;
  	int sequence_num;
  	long long size;
  	std::string first_rowkey;
  	std::string first_colkey;
    bool loaded;
  	Table ();
  	Table (std::string line);
  	Table (std::string row, std::string col);
  	void load_table();
  	void evict_table();
  	void update_file();
  	Table split_file(std::string name, int sequence);
  	void update_first_line();
    bool equals(Table t);
    void delete_val(std::string row, std::string col);
    std::vector<std::string> get_data();
};
#endif // BIGTABLE_H

