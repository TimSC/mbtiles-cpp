#include <iostream>
#include <sqlite3.h>
#include <string>
#include <cstring>
#include <map>
using namespace std;
#include "MBTileReader.h"

int main(int argc, char **argv)
{
	class MBTileReader mbTileReader("cairo_egypt.mbtiles");	
	
	cout << "name:" << mbTileReader.GetMetadata("name") << endl;
	cout << "type:" << mbTileReader.GetMetadata("type") << endl;
	cout << "version:" << mbTileReader.GetMetadata("version") << endl;
	cout << "description:" << mbTileReader.GetMetadata("description") << endl;
	cout << "format:" << mbTileReader.GetMetadata("format") << endl;
	cout << "bounds:" << mbTileReader.GetMetadata("bounds") << endl;

	if(0) //Get metadata fields
	{
		std::vector<std::string> fieldNames;
		mbTileReader.GetMetadataFields(fieldNames);
		for(unsigned i=0;i<fieldNames.size();i++) cout << fieldNames[i] << endl;
	}

	if(0) //Get list of tiles
	{
		TileInfoRows tileInfoRows;
		mbTileReader.ListTiles(tileInfoRows);
		for(unsigned i=0;i < tileInfoRows.size(); i++)
		{
			for(size_t j=0; j < tileInfoRows[i].size(); j++)
				cout << tileInfoRows[i][j] << ",";
			cout << endl;
		}
	}

	string blob;
	mbTileReader.GetTile(14,9618,9611,blob);
	cout << "blob size: "<< blob.size() << endl;
}

