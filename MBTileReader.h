#ifndef MBTILE_READER_H
#define MBTILE_READER_H

#include <sqlite3.h>
#include <string>
#include <map>
#include <vector>

typedef std::vector<std::vector<unsigned int> > TileInfoRows;

class MBTileReader
{
public:
	MBTileReader(const char *filename);
	virtual ~MBTileReader();

	std::string GetMetadata(const char *metaField);
	void GetMetadataFields(std::vector<std::string> &fieldNamesOut);
	void ListTiles(TileInfoRows &tileInfoRowsOut);
	void GetTile(unsigned int zoomLevel, 
		unsigned int tileColumn, 
		unsigned int tileRow,
		std::string &blobOut);

protected:
	sqlite3 *db;
	std::map<std::string, std::string> metadata;

	static int MetadataCallbackStatic(void *obj, int argc, char **argv, char **azColName);
	int MetadataCallback(int argc, char **argv, char **azColName);

	static int ListTilesCallbackStatic(void *obj, int argc, char **argv, char **azColName);
};

int long2tilex(double lon, int z);
int lat2tiley(double lat, int z);
double tilex2long(int x, int z);
double tiley2lat(int y, int z);

#endif //MBTILE_READER_H

