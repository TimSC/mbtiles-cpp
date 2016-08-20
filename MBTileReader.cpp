#include "MBTileReader.h"
#include <iostream>
#include <cstring>
using namespace std;

MBTileReader::MBTileReader(const char *filename)
{
	this->db = NULL;

	int status = sqlite3_open(filename, &this->db);
	if(status){
		sqlite3_close(this->db);
		throw runtime_error("Error opening database");
	}

	char *zErrMsg = 0;
	status = sqlite3_exec(this->db, "SELECT * FROM metadata;", this->MetadataCallbackStatic, this, &zErrMsg);
	if( status!=SQLITE_OK )
	{
		string err("SQL error: ");
		err += zErrMsg;
		sqlite3_free(zErrMsg);
		throw runtime_error(err);
	}
}

MBTileReader::~MBTileReader()
{
	sqlite3_close(this->db);
}

int MBTileReader::MetadataCallbackStatic(void *obj, int argc, char **argv, char **azColName)
{
	return ((MBTileReader*)obj)->MetadataCallback(argc, argv, azColName);
}

int MBTileReader::MetadataCallback(int argc, char **argv, char **azColName)
{
	string name, value;
	for(int i=0; i<argc; i++){
		if (strncmp(azColName[i], "name", 5) == 0)
			name = (argv[i] ? argv[i] : "NULL");
		if (strncmp(azColName[i], "value", 6) == 0)
			value = (argv[i] ? argv[i] : "NULL");		
	}
	if (name.size() > 0)
		this->metadata[name] = value;
	return 0;
}

string MBTileReader::GetMetadata(const char *metaField)
{
	return metadata[metaField];
}

