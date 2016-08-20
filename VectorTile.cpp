#include "VectorTile.h"
#include <string>
#include "vector_tile20/vector_tile.pb.h"
#include <math.h>
#include <iostream>
#include <sstream>
#include <utility>
using namespace std;

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

typedef std::pair<double, double> Point2D;
inline double CheckWinding(std::vector<Point2D> pts) 
{
	double total = 0.0;
	for(size_t i=0; i < pts.size(); i++)
	{
		size_t i2 = (i+1)%pts.size();
		double val = (pts[i2].first - pts[i].first)*(pts[i2].second + pts[i].second);
		total += val;
	}
	return total;
}

void DecodeGeometry(const ::vector_tile::Tile_Feature &feature,
	int extent, int tileZoom, int tileColumn, int tileRow)
{
	long unsigned int numTiles = pow(2,tileZoom);
	double lonMin = tilex2long(tileColumn, tileZoom);
	double latMax = tiley2lat(numTiles-tileRow-1, tileZoom);//Why is latitude no where near expected?
	double lonMax = tilex2long(tileColumn+1, tileZoom);
	double latMin = tiley2lat(numTiles-tileRow, tileZoom);
	double dLat = latMax - latMin;
	double dLon = lonMax - lonMin;
	vector<Point2D> points;
	unsigned prevCmdId = 0;

	int cursorx = 0, cursory = 0;
	double prevx = 0.0, prevy = 0.0;

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
				prevx = px; 
				prevy = py;
				i += 2;
				prevCmdId = cmdId;
			}
		}
		if(cmdId == 2)//LineTo
		{
			for(int j=0; j < cmdCount; j++)
			{
				if(prevCmdId != 2)
					points.push_back(Point2D(prevx, prevy));
				unsigned v = feature.geometry(i+1);
				int value1 = ((v >> 1) ^ (-(v & 1)));
				v = feature.geometry(i+2);
				int value2 = ((v >> 1) ^ (-(v & 1)));
				cursorx += value1;
				cursory += value2;
				double px = dLon * double(cursorx) / double(extent) + lonMin;
				double py = - dLat * double(cursory) / double(extent) + latMax;
				cout << "LineTo " << cursorx << "," << cursory << ",(" << px << "," << py << ")" << endl;
				points.push_back(Point2D(px, py));
				i += 2;
				prevCmdId = cmdId;
			}
		}
		if(cmdId == 7) //ClosePath
		{
			for(int j=0; j < cmdCount; j++)
			{
				cout << "ClosePath" << endl;
				cout << "winding: " << CheckWinding(points) << endl;
				points.clear();
				prevCmdId = cmdId;
			}
		}
		
	}
}

DecodeVectorTile::DecodeVectorTile(class DecodeVectorTileResults &output)
{
	this->output = &output;
}

DecodeVectorTile::~DecodeVectorTile()
{

}

void DecodeVectorTile::DecodeTileData(const std::string &tileData, int tileZoom, int tileColumn, int tileRow)
{
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

			//if(feature.type() != ::vector_tile::Tile_GeomType_POLYGON) continue;
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

// *********************************************

// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#C.2FC.2B.2B
int long2tilex(double lon, int z) 
{ 
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}

int lat2tiley(double lat, int z)
{ 
	return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z))); 
}

double tilex2long(int x, int z) 
{
	return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}


