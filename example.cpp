#include <iostream>
#include <cstring>
#include <map>
#include <fstream>
#include "cppGzip/DecodeGzip.h"
#include "MBTileReader.h"
#include <math.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "VectorTile.h"
using namespace std;

// http://stackoverflow.com/a/236803/4288232
void strsplit(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

///This class would need to be expanded to store features as needed.
class ExampleDataStore : public DecodeVectorTileResults
{
public:
	ExampleDataStore() : DecodeVectorTileResults()
	{
		cout << "Create custom data store..." << endl;
	}

	void Feature(int typeEnum, bool hasId, unsigned long long id, 
		const std::map<std::string, std::string> &tagMap,
		std::vector<Point2D> &pointsOut, 
		std::vector<std::vector<Point2D> > &linesOut,
		std::vector<Polygon2D> &polygonsOut)
	{
		//In real use, delete this function call and add your own functionality.
		DecodeVectorTileResults::Feature(typeEnum, hasId, id, 
			tagMap, pointsOut, linesOut, polygonsOut);
	}
};

int main(int argc, char **argv)
{
	// Verify that the version of the library that we linked against is
	// compatible with the version of the headers we compiled against.
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	//Grab from http://osm2vectortiles.org/downloads/
	class MBTileReader mbTileReader("cairo_egypt.mbtiles");	
	
	cout << "name:" << mbTileReader.GetMetadata("name") << endl;
	cout << "type:" << mbTileReader.GetMetadata("type") << endl;
	string version = mbTileReader.GetMetadata("version");
	cout << "version:" << version << endl;
	vector<string> versionSplit;
	strsplit(version, '.', versionSplit);
	vector<int> versionInts;
	for (size_t i=0;i<versionSplit.size();i++) versionInts.push_back(atoi(versionSplit[i].c_str()));
	cout << "description:" << mbTileReader.GetMetadata("description") << endl;
	string format = mbTileReader.GetMetadata("format");
	cout << "format:" << format << endl;
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
	int tileZoom = 14;
	int tileColumn = 9613;
	int tileRow = 9626;
	mbTileReader.GetTile(tileZoom, tileColumn, tileRow, blob);

	if(format == "pbf" && versionInts[0] == 2)
	{
		//Ungzip the data
		std::stringbuf buff;
		buff.sputn(blob.c_str(), blob.size());
		DecodeGzip dec(buff);

		string tileData;

		char tmp[1024];
		while(dec.in_avail())
		{
			streamsize bytes = dec.sgetn(tmp, 1024);
			tileData.append(tmp, bytes);
		}

		//Decode vector data
		class ExampleDataStore results;
		class DecodeVectorTile vectorDec(results);
		vectorDec.DecodeTileData(tileData, tileZoom, tileColumn, tileRow);
	}
	
	if(format == "jpg" || format == "png")
	{
		//Save image to output file
		stringstream filename;
		filename << "out" << "." << format;
		ofstream outFi(filename.str().c_str());
		outFi.write(blob.c_str(), blob.size());
	}

	// Optional:  Delete all global objects allocated by libprotobuf.
	google::protobuf::ShutdownProtobufLibrary();
	
}

