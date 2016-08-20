#include <iostream>
#include <sqlite3.h>
#include <string>
#include <cstring>
#include <map>
using namespace std;

class MBTileReader
{
public:
	MBTileReader(const char *filename);
	virtual ~MBTileReader();

	string GetMetadata(const char *metaField);

protected:
	sqlite3 *db;
	map<string, string> metadata;

	static int MetadataCallbackStatic(void *obj, int argc, char **argv, char **azColName);
	int MetadataCallback(int argc, char **argv, char **azColName);

};

MBTileReader::MBTileReader(const char *filename)
{
	this->db = NULL;

	int status = sqlite3_open(filename, &this->db);
	if(status){
		cout << "Error opening database" << endl;
		sqlite3_close(this->db);
		return;
	}

	char *zErrMsg = 0;
	status = sqlite3_exec(this->db, "SELECT * FROM metadata;", this->MetadataCallbackStatic, this, &zErrMsg);
	if( status!=SQLITE_OK )
	{
		cout << "SQL error " << zErrMsg << endl;
		sqlite3_free(zErrMsg);
		return;
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
		//cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
		if (strncmp(azColName[i], "name", 5) == 0)
			name = (argv[i] ? argv[i] : "NULL");
		if (strncmp(azColName[i], "value", 6) == 0)
			value = (argv[i] ? argv[i] : "NULL");		
	}
	if (name.size() > 0)
		this->metadata[name] = value;
	//cout << endl;
	return 0;
}

string MBTileReader::GetMetadata(const char *metaField)
{
	return metadata[metaField];
}

int main(int argc, char **argv)
{
	class MBTileReader mbTileReader("cairo_egypt.mbtiles");	
	
	cout << "name:" << mbTileReader.GetMetadata("name") << endl;
	cout << "type:" << mbTileReader.GetMetadata("type") << endl;
	cout << "version:" << mbTileReader.GetMetadata("version") << endl;
	cout << "description:" << mbTileReader.GetMetadata("description") << endl;
	cout << "format:" << mbTileReader.GetMetadata("format") << endl;
	cout << "bounds:" << mbTileReader.GetMetadata("bounds") << endl;

}

