#include "../Include/VectorTile.h"
#include <string>
#include <math.h>
#include <iostream>
#include <sstream>
#include <utility>
using namespace std;

string FeatureTypeToStr(int typeIn)
{
	::vector_tile::Tile_GeomType type = (::vector_tile::Tile_GeomType)typeIn;
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

inline double CheckWindingi(LineLoop2Di pts) 
{
	double total = 0.0;
	for(size_t i=0; i < pts.size(); i++)
	{
		size_t i2 = (i+1)%pts.size();
		double val = ((double)(pts[i2].first) - (double)(pts[i].first))*((double)(pts[i2].second) + (double)(pts[i].second));
		total += val;
	}
	return -total;
}

void CheckCmdId(uint32_t cmdIdCount, int cmdId, size_t count)
{
	int cmdId2 = cmdIdCount & 0x7;
	size_t cmdCount2 = cmdIdCount >> 3;
	if (cmdId != cmdId2 || count != cmdCount2)
		throw overflow_error("Failed to encode command (probably due to number of points)");
}

// **************************************************************

DecodeVectorTile::DecodeVectorTile(int tileZoom, int tileColumn, int tileRow, class DecodeVectorTileResults &output)
{
	this->output = &output;
	this->numTiles = pow(2,tileZoom);
	this->lonMin = tilex2long(tileColumn, tileZoom);
	this->latMax = tiley2lat(tileRow, tileZoom);
	this->lonMax = tilex2long(tileColumn+1, tileZoom);
	this->latMin = tiley2lat(tileRow+1, tileZoom);
	this->dLat = this->latMax - this->latMin;
	this->dLon = this->lonMax - this->lonMin;
}

DecodeVectorTile::~DecodeVectorTile()
{

}

void DecodeVectorTile::DecodeTileData(const std::string &tileData)
{
	vector_tile::Tile tile;
	bool parsed = tile.ParseFromString(tileData);
	if(!parsed)
		throw runtime_error("Failed to parse tile data");
	
	this->output->NumLayers(tile.layers_size());	

	for(int layerNum = 0; layerNum < tile.layers_size(); layerNum++)
	{
		const ::vector_tile::Tile_Layer &layer = tile.layers(layerNum);
		this->output->LayerStart(layer.name().c_str(), layer.version(), layer.extent());

		//The spec says "Decoders SHOULD parse the version first to ensure that 
		//they are capable of decoding each layer." This has not been implemented.

		for(int featureNum = 0; featureNum < layer.features_size(); featureNum++)
		{
			const ::vector_tile::Tile_Feature &feature = layer.features(featureNum);

			map<string, string> tagMap;
			for(int tagNum = 0; tagNum < feature.tags_size(); tagNum+=2)
			{	
				const ::vector_tile::Tile_Value &value = layer.values(feature.tags(tagNum+1));
				tagMap[layer.keys(feature.tags(tagNum))] = ValueToStr(value);
			}

			vector<Point2D> points;
			vector<vector<Point2D> > lines;
			vector<Polygon2D> polygons;
			this->DecodeGeometry(feature, layer.extent(),
				points, lines, polygons);

			this->output->Feature(feature.type(), feature.has_id(), feature.id(), tagMap, 
				points, lines, polygons);
		}

		this->output->LayerEnd();
	}

	this->output->Finish();
}

void DecodeVectorTile::DecodeGeometry(const ::vector_tile::Tile_Feature &feature,
	int extent,
	vector<Point2D> &pointsOut, 
	vector<vector<Point2D> > &linesOut,
	vector<Polygon2D> &polygonsOut)	
{
	vector<Point2D> points;
	vector<Point2Di> pointsTileSpace;
	Polygon2D currentPolygon;
	bool currentPolygonSet = false;
	unsigned prevCmdId = 0;

	int cursorx = 0, cursory = 0;
	double prevx = 0.0, prevy = 0.0; //Lat lon
	int prevxTileSpace = 0, prevyTileSpace = 0;
	pointsOut.clear();
	linesOut.clear();
	polygonsOut.clear();

	for(int i=0; i < feature.geometry_size(); i ++)
	{
		unsigned g = feature.geometry(i);
		unsigned cmdId = g & 0x7;
		unsigned cmdCount = g >> 3;
		if(cmdId == 1)//MoveTo
		{
			for(unsigned j=0; j < cmdCount; j++)
			{
				unsigned v = feature.geometry(i+1);
				int value1 = ((v >> 1) ^ (-(v & 1)));
				v = feature.geometry(i+2);
				int value2 = ((v >> 1) ^ (-(v & 1)));
				cursorx += value1;
				cursory += value2;
				double px = this->dLon * double(cursorx) / double(extent) + this->lonMin;
				double py = - this->dLat * double(cursory) / double(extent) + this->latMax;
				
				if (feature.type() == vector_tile::Tile_GeomType_POINT)
					pointsOut.push_back(Point2D(px, py));
				if (feature.type() == vector_tile::Tile_GeomType_LINESTRING && points.size() > 0)
				{
					linesOut.push_back(points);
					points.clear();
					pointsTileSpace.clear();
				}
				prevx = px; 
				prevy = py;
				prevxTileSpace = cursorx;
				prevyTileSpace = cursory;
				i += 2;
				prevCmdId = cmdId;
			}
		}
		if(cmdId == 2)//LineTo
		{
			for(unsigned j=0; j < cmdCount; j++)
			{
				if(prevCmdId != 2)
				{
					points.push_back(Point2D(prevx, prevy));
					pointsTileSpace.push_back(Point2D(prevxTileSpace, prevyTileSpace));
				}
				unsigned v = feature.geometry(i+1);
				int value1 = ((v >> 1) ^ (-(v & 1)));
				v = feature.geometry(i+2);
				int value2 = ((v >> 1) ^ (-(v & 1)));
				cursorx += value1;
				cursory += value2;
				double px = this->dLon * double(cursorx) / double(extent) + this->lonMin;
				double py = - this->dLat * double(cursory) / double(extent) + this->latMax;

				points.push_back(Point2D(px, py));
				pointsTileSpace.push_back(Point2D(cursorx, cursory));
				i += 2;
				prevCmdId = cmdId;
			}
		}
		if(cmdId == 7) //ClosePath
		{
			//Closed path does not move cursor in v1 to v2.1 of spec.
			// https://github.com/mapbox/vector-tile-spec/issues/49
			for(unsigned j=0; j < cmdCount; j++)
			{
				if (feature.type() == vector_tile::Tile_GeomType_POLYGON)
				{
					double winding = CheckWindingi(pointsTileSpace);
					if(winding >= 0.0)
					{
						if(currentPolygonSet)
						{
							//We are moving on to the next polygon, so store what we have collected so far
							polygonsOut.push_back(currentPolygon);
							currentPolygon.first.clear();
							currentPolygon.second.clear();
						}
						currentPolygon.first = points; //outer shape
						currentPolygonSet = true;
					}
					else
						currentPolygon.second.push_back(points); //inter shape
					
					points.clear();
					pointsTileSpace.clear();
					prevCmdId = cmdId;
				}				
			}
		}
	}

	if (feature.type() == vector_tile::Tile_GeomType_LINESTRING)
		linesOut.push_back(points);
	if (feature.type() == vector_tile::Tile_GeomType_POLYGON)
		polygonsOut.push_back(currentPolygon);
}

// *********************************************

// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#C.2FC.2B.2B
int long2tilex(double lon, int z)
{
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int lat2tiley(double lat, int z)
{
	int y = (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
	int ymax  = 1 << z;
	y = ymax - y - 1;
	return y;
}

double tilex2long(int x, int z)
{
	return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, int z)
{
	double n = pow(2.0,z);
	int ymax  = 1 << z;
	y = ymax - y - 1;
	double latRad = atan(sinh(M_PI*(1-(2*y/n))));
	return 180.0 / M_PI * latRad;
}

// ***********************************************

DecodeVectorTileResults::DecodeVectorTileResults()
{

}

DecodeVectorTileResults::~DecodeVectorTileResults()
{

}

void DecodeVectorTileResults::NumLayers(int numLayers)
{
	cout << "Num layers: " << numLayers << endl;
}

void DecodeVectorTileResults::LayerStart(const char *name, int version, int extent)
{
	cout << "layer name: " << name << endl;
	cout << "layer version: " << version << endl;
}

void DecodeVectorTileResults::LayerEnd()
{
	cout << "layer end" << endl;
}

void DecodeVectorTileResults::Feature(int typeEnum, bool hasId, 
	unsigned long long id, const map<string, string> &tagMap,
	vector<Point2D> &points, 
	vector<vector<Point2D> > &lines,
	vector<Polygon2D> &polygons)
{
	cout << typeEnum << "," << FeatureTypeToStr((::vector_tile::Tile_GeomType)typeEnum);
	if(hasId)
		cout << ",id=" << id;
	cout << endl;

	for(map<string, string>::const_iterator it = tagMap.begin(); it != tagMap.end(); it++)
	{
		cout << it->first << "=" << it->second << endl;
	}

	for(size_t i =0; i < points.size(); i++)
		cout << "POINT("<<points[i].first<<","<<points[i].second<<") ";
	for(size_t i =0; i < lines.size(); i++)
	{
		cout << "LINESTRING(";
		vector<Point2D> &linePts = lines[i];
		for(size_t j =0; j < linePts.size(); j++)
			cout << "("<<linePts[j].first<<","<<linePts[j].second<<") ";
		cout << ") ";
	}
	for(size_t i =0; i < polygons.size(); i++)
	{
		Polygon2D &polygon = polygons[i];
		cout << "POLYGON((";
		LineLoop2D &linePts = polygon.first; //Outer shape
		for(size_t j =0; j < linePts.size(); j++)
			cout << "("<<linePts[j].first<<","<<linePts[j].second<<") ";
		cout << ")";
		if(polygon.second.size() > 0)
		{
			//Inner shape
			cout << ",(";
			for(size_t k =0; k < polygon.second.size(); k++)
			{
				cout << "(";
				LineLoop2D &linePts2 = polygon.second[k];
				for(size_t j =0; j < linePts2.size(); j++)
					cout << "("<<linePts2[j].first<<","<<linePts2[j].second<<") ";
				cout << ") ";
			}
			cout << ")";
		}

		cout << ") ";
	}
	cout << endl;
}

void DecodeVectorTileResults::Finish()
{

}
// *********************************************************************************

EncodeVectorTile::EncodeVectorTile(int tileZoom, int tileColumn, int tileRow, std::ostream &output): tileZoom(tileZoom), tileColumn(tileColumn), tileRow(tileRow), output(&output), currentLayer(NULL)
{
	this->numTiles = pow(2,tileZoom);
	this->lonMin = tilex2long(tileColumn, tileZoom);
	this->latMax = tiley2lat(tileRow, tileZoom);
	this->lonMax = tilex2long(tileColumn+1, tileZoom);
	this->latMin = tiley2lat(tileRow+1, tileZoom);
	this->dLat = this->latMax - this->latMin;
	this->dLon = this->lonMax - this->lonMin;
}

EncodeVectorTile::~EncodeVectorTile()
{

}

void EncodeVectorTile::NumLayers(int numLayers)
{

}

void EncodeVectorTile::LayerStart(const char *name, int version, int extent)
{
	if (this->currentLayer != NULL)
		throw runtime_error("Previous layer not closed");
	this->currentLayer = this->tile.add_layers();
	this->currentLayer->set_name(name);
	this->currentLayer->set_version(version);
	this->currentLayer->set_extent(extent);
}

void EncodeVectorTile::LayerEnd()
{
	if (this->currentLayer == NULL)
		throw runtime_error("Layer not started");
	this->currentLayer = NULL;
	this->keysCache.clear();
	this->valuesCache.clear();
}

void EncodeVectorTile::Feature(int typeEnum, bool hasId, unsigned long long id, 
	const std::map<std::string, std::string> &tagMap,
	std::vector<Point2D> &points, 
	std::vector<std::vector<Point2D> > &lines,
	std::vector<Polygon2D> &polygons)
{
	if (this->currentLayer == NULL)
		throw runtime_error("Cannot add feature: layer not started");
	
	vector_tile::Tile_Feature* feature = this->currentLayer->add_features();
	if(hasId)
		feature->set_id(id);

	for(std::map<std::string, std::string>::const_iterator it = tagMap.begin(); it != tagMap.end(); it++)
	{
		map<std::string, int>::iterator keyChk = this->keysCache.find(it->first);
		map<std::string, int>::iterator valueChk = this->valuesCache.find(it->second);
		int keyIndex = -1, valueIndex = -1;

		if(keyChk == this->keysCache.end())
		{
			this->currentLayer->add_keys(it->first);
			keyIndex = this->currentLayer->keys_size()-1;
			this->keysCache[it->first] = keyIndex;
		}
		else
			keyIndex = keyChk->second;

		if(valueChk == this->valuesCache.end())
		{
			vector_tile::Tile_Value *value = this->currentLayer->add_values();
			value->set_string_value(it->second);
			valueIndex = this->currentLayer->values_size()-1;
			this->valuesCache[it->second] = valueIndex;
		}
		else
			valueIndex = valueChk->second;

		feature->add_tags(keyIndex);
		feature->add_tags(valueIndex);
	}
	
	//Encode geometry
	this->EncodeGeometry((vector_tile::Tile_GeomType)typeEnum,
		this->currentLayer->extent(),
		points, 
		lines,
		polygons, 
		feature);
	
}

void EncodeVectorTile::Finish()
{
	//this->tile.SerializeToOstream(this->output);
}

void EncodeVectorTile::EncodeTileSpacePoints(const vector<Point2Di> &points, 
	int cmdId,
	bool reverseOrder,
	size_t startIndex, 
	int &cursorx, int &cursory, vector_tile::Tile_Feature *outFeature)
{
	size_t cmdCount = points.size() - startIndex;
	uint32_t cmdIdCount = (cmdId & 0x7) | (cmdCount << 3);
	CheckCmdId(cmdIdCount, cmdId, cmdCount);
	outFeature->add_geometry(cmdIdCount);

	for(size_t i = startIndex;i < points.size(); i++)
	{
		size_t i2 = i;
		if(reverseOrder)
			i2 = points.size() - 1 - i;

		int32_t value1 = points[i2].first - cursorx;
		int32_t value2 = points[i2].second - cursory;
		
		uint32_t value1enc = (value1 << 1) ^ (value1 >> 31);
		uint32_t value2enc = (value2 << 1) ^ (value2 >> 31);

		outFeature->add_geometry(value1enc);
		outFeature->add_geometry(value2enc);

		cursorx = points[i2].first;
		cursory = points[i2].second;
	}
}

void EncodeVectorTile::EncodeGeometry(vector_tile::Tile_GeomType type, 
	int extent,
	const vector<Point2D> &points, 
	const vector<vector<Point2D> > &lines,
	const vector<Polygon2D> &polygons,
	vector_tile::Tile_Feature *outFeature)
{
	if(outFeature == NULL)
		throw runtime_error("Unexpected null pointer");
	int cursorx = 0, cursory = 0;
	//double prevx = 0.0, prevy = 0.0;
	outFeature->set_type(type);
	LineLoop2Di tmpTileSpace, tmpTileSpace2;

	if(type == vector_tile::Tile_GeomType_POINT)
	{
		this->ConvertToTileCoords(points, extent, tmpTileSpace);
		EncodeTileSpacePoints(tmpTileSpace, 1, false, 0, cursorx, cursory, outFeature);
	}

	if(type == vector_tile::Tile_GeomType_LINESTRING)
	{
		for(size_t i=0;i < lines.size(); i++)
		{
			const vector<Point2D> &line = lines[i];
			this->ConvertToTileCoords(line, extent, tmpTileSpace);
			this->DeduplicatePoints(tmpTileSpace, tmpTileSpace2);
			if (tmpTileSpace2.size() < 2) continue;
			
			//Move to start
			vector<Point2Di> tmpPoints;
			tmpPoints.push_back(tmpTileSpace2[0]);
			EncodeTileSpacePoints(tmpPoints, 1, false, 0, cursorx, cursory, outFeature);

			//Draw line shape
			EncodeTileSpacePoints(tmpTileSpace2, 2, false, 1, cursorx, cursory, outFeature);
		}
	}

	if(type == vector_tile::Tile_GeomType_POLYGON)
	{
		for(size_t i=0;i < polygons.size(); i++)
		{
			const Polygon2D &polygon = polygons[i];
			this->ConvertToTileCoords(polygon.first, extent, tmpTileSpace);
			this->DeduplicatePoints(tmpTileSpace, tmpTileSpace2);
			if (tmpTileSpace2.size() < 2) continue;
			bool reverseOuter = CheckWindingi(tmpTileSpace2) < 0.0;
			
			//Move to start of outer polygon
			vector<Point2Di> tmpPoints;
			if (reverseOuter)
				tmpPoints.push_back(tmpTileSpace2[tmpTileSpace2.size()-1]);
			else
				tmpPoints.push_back(tmpTileSpace2[0]);
			EncodeTileSpacePoints(tmpPoints, 1, false, 0, cursorx, cursory, outFeature);

			//Draw line shape of outer polygon
			EncodeTileSpacePoints(tmpTileSpace2, 2, reverseOuter, 1, cursorx, cursory, outFeature);

			//Close outer contour
			uint32_t cmdId = 7;
			uint32_t cmdCount = 1;
			uint32_t cmdIdCount = (cmdId & 0x7) | (cmdCount << 3);
			outFeature->add_geometry(cmdIdCount);

			//Inner polygons
			for(size_t j=0;j < polygon.second.size(); j++)
			{
				const LineLoop2D &inner = polygon.second[j];
				this->ConvertToTileCoords(inner, extent, tmpTileSpace);
				this->DeduplicatePoints(tmpTileSpace, tmpTileSpace2);
				if(tmpTileSpace2.size() < 2) continue;
				bool reverseInner = CheckWindingi(tmpTileSpace2) >= 0.0;

				//Move to start of inner polygon
				vector<Point2Di> tmpPoints;
				if (reverseInner)
					tmpPoints.push_back(tmpTileSpace2[tmpTileSpace2.size()-1]);
				else
					tmpPoints.push_back(tmpTileSpace2[0]);
				EncodeTileSpacePoints(tmpPoints, 1, false, 0, cursorx, cursory, outFeature);

				//Draw line shape of inner polygon
				EncodeTileSpacePoints(tmpTileSpace2, 2, reverseInner, 1, cursorx, cursory, outFeature);

				//Close inner contour
				cmdId = 7;
				cmdCount = 1;
				cmdIdCount = (cmdId & 0x7) | (cmdCount << 3);
				outFeature->add_geometry(cmdIdCount);

			}			
		}
	}

}

void EncodeVectorTile::ConvertToTileCoords(const LineLoop2D &points, int extent, LineLoop2Di &out)
{
	out.clear();
	for(size_t i = 0;i < points.size(); i++)
	{
		double cx = (points[i].first - this->lonMin) * double(extent) / double(this->dLon);
		double cy = (points[i].second - this->latMax) * double(extent) / (-this->dLat);
		out.push_back(Point2Di(round(cx), round(cy)));
	}
}

void EncodeVectorTile::DeduplicatePoints(const LineLoop2Di &points, LineLoop2Di &out)
{
	out.clear();
	int px = 0, py = 0;
	for(size_t i = 0; i < points.size(); i ++)
	{
		if(i == 0 || px != points[i].first || py != points[i].second)
			out.push_back(Point2Di(points[i].first, points[i].second));
		px = points[i].first;
		py = points[i].second;
	}
}

