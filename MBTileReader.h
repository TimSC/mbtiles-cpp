#ifndef MBTILE_READER_H
#define MBTILE_READER_H

#include <sqlite3.h>
#include <string>
#include <map>

class MBTileReader
{
public:
	MBTileReader(const char *filename);
	virtual ~MBTileReader();

	std::string GetMetadata(const char *metaField);
	void ListTiles();

protected:
	sqlite3 *db;
	std::map<std::string, std::string> metadata;

	static int MetadataCallbackStatic(void *obj, int argc, char **argv, char **azColName);
	int MetadataCallback(int argc, char **argv, char **azColName);

	int ListTilesCallbackStatic(void *obj, int argc, char **argv, char **azColName);
	int ListTilesCallback(int argc, char **argv, char **azColName);

};

#endif //MBTILE_READER_H

