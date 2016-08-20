#include <iostream>
#include <sqlite3.h>
#include <string>
#include <cstring>
#include <map>
#include <sstream>
#include "ReadGzip.h"
using namespace std;
#include "MBTileReader.h"
#include "vector_tile20/vector_tile.pb.h"
#include <math.h>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

// http://stackoverflow.com/a/236803/4288232
void strsplit(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

string FeatureTypeToStr(::vector_tile::Tile_GeomType type)
{
	if(type == ::vector_tile::Tile_GeomType_UNKNOWN)
		return "Unknown";
	if(type == ::vector_tile::Tile_GeomType_POINT)
		return "Point";
	if(type == ::vector_tile::Tile_GeomType_LINESTRING)
		return "LineString";
	if(type == ::vector_tile::Tile_GeomType_POLYGON)
		return "Polygon";
	return "Unknown type";
}

string ValueToStr(const ::vector_tile::Tile_Value &value)
{
	if(value.has_string_value())
		return value.string_value();
	if(value.has_float_value())
	{
		stringstream ss;
		ss << value.float_value();
		return ss.str();
	}
	if(value.has_double_value())
	{
		stringstream ss;
		ss << value.double_value();
		return ss.str();
	}
	if(value.has_int_value())
	{
		stringstream ss;
		ss << value.int_value();
		return ss.str();
	}
	if(value.has_uint_value())
	{
		stringstream ss;
		ss << value.uint_value();
		return ss.str();
	}
	if(value.has_sint_value())
	{
		stringstream ss;
		ss << value.sint_value();
		return ss.str();
	}
	if(value.has_bool_value())
	{
		stringstream ss;
		ss << value.bool_value();
		return ss.str();
	}
	return "Error: Unknown value type";
}

void DecodeGeometry(const ::vector_tile::Tile_Feature &feature,
	int extent, int tileZoom, int tileColumn, int tileRow)
{
	long unsigned int numTiles = pow(2,tileZoom);
	double lonMin = tilex2long(tileColumn, tileZoom);
	double latMin = tiley2lat(numTiles-tileRow, tileZoom);//Why is latitude no where near expected?
	double lonMax = tilex2long(tileColumn+1, tileZoom);
	double latMax = tiley2lat(numTiles-tileRow-1, tileZoom);
	double dLat = latMax - latMin;
	double dLon = lonMax - lonMin;

	int cursorx = 0, cursory = 0;
	for(int i=0; i < feature.geometry_size(); i ++)
	{
		unsigned g = feature.geometry(i);
		unsigned cmdId = g & 0x7;
		unsigned cmdCount = g >> 3;
		if(cmdId == 1)//MoveTo
		{
			for(int j=0; j < cmdCount; j++)
			{
				unsigned v = feature.geometry(i+1);
				int value1 = ((v >> 1) ^ (-(v & 1)));
				v = feature.geometry(i+2);
				int value2 = ((v >> 1) ^ (-(v & 1)));
				cursorx += value1;
				cursory += value2;
				double px = dLon * double(cursorx) / double(extent) + lonMin;
				double py = - dLat * double(cursory) / double(extent) + latMax;
				cout << "MoveTo " << cursorx << "," << cursory << ",(" << px << "," << py << ")" << endl;
				i += 2;
			}
		}
		if(cmdId == 2)//LineTo
		{
			for(int j=0; j < cmdCount; j++)
			{
				unsigned v = feature.geometry(i+1);
				int value1 = ((v >> 1) ^ (-(v & 1)));
				v = feature.geometry(i+2);
				int value2 = ((v >> 1) ^ (-(v & 1)));
				cursorx += value1;
				cursory += value2;
				double px = dLon * double(cursorx) / double(extent) + lonMin;
				double py = - dLat * double(cursory) / double(extent) + latMax;
				cout << "LineTo " << cursorx << "," << cursory << ",(" << px << "," << py << ")" << endl;
				i += 2;
			}
		}
		if(cmdId == 7) //ClosePath
		{
			for(int j=0; j < cmdCount; j++)
			{
				cout << "ClosePath" << endl;
			}
		}

	}
}

int main(int argc, char **argv)
{
	class MBTileReader mbTileReader("cairo_egypt.mbtiles");	
	
	cout << "name:" << mbTileReader.GetMetadata("name") << endl;
	cout << "type:" << mbTileReader.GetMetadata("type") << endl;
	string version = mbTileReader.GetMetadata("version");
	cout << "version:" << version << endl;
	vector<string> versionSplit;
	strsplit(version, '.', versionSplit);
	vector<int> versionInts;
	for (int i=0;i<versionSplit.size();i++) versionInts.push_back(atoi(versionSplit[i].c_str()));
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
	int tileColumn = 9618;
	int tileRow = 9611;
	mbTileReader.GetTile(tileZoom, tileColumn, tileRow, blob);

	if(format == "pbf" && versionInts[0] == 2)
	{
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

		vector_tile::Tile tile;
		cout << "ParseFromString: " << tile.ParseFromString(tileData) << endl;
		cout << "Num layers: " << tile.layers_size() << endl;
		
		for(int layerNum = 0; layerNum < tile.layers_size(); layerNum++)
		{
			const ::vector_tile::Tile_Layer &layer = tile.layers(layerNum);
			cout << "layer version: " << layer.version() << endl;
			cout << "layer name: " << layer.name() << endl;
			cout << "layer extent: " << layer.extent() << endl;
			cout << "layer keys_size(): " << layer.keys_size() << endl;
			cout << "layer values_size(): " << layer.values_size() << endl;
			cout << "layer features_size(): " << layer.features_size() << endl;

			//The spec says "Decoders SHOULD parse the version first to ensure that 
			//they are capable of decoding each layer." This has not been implemented.

			for(int featureNum = 0; featureNum < layer.features_size(); featureNum++)
			{
				const ::vector_tile::Tile_Feature &feature =layer.features(featureNum);
				cout << featureNum << "," << feature.type() << "," << FeatureTypeToStr(feature.type());
				if(feature.has_id())
					cout << ",id=" << feature.id();
				cout << endl;
				for(int tagNum = 0; tagNum < feature.tags_size(); tagNum+=2)
				{	
					cout << layer.keys(feature.tags(tagNum)) << "=";
					const ::vector_tile::Tile_Value &value = layer.values(feature.tags(tagNum+1));
					cout << ValueToStr(value) << endl;
				}
				DecodeGeometry(feature, layer.extent(), tileZoom, tileColumn, tileRow);
			}
		}
	}
}

