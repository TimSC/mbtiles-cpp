#ifndef _VECTOR_TILE_H
#define _VECTOR_TILE_H

#include <string>
#include <map>
#include <vector>
#include "vector_tile21/vector_tile.pb.h"

typedef std::pair<double, double> Point2D;
typedef std::vector<Point2D> LineLoop2D;
typedef std::pair<LineLoop2D, std::vector<LineLoop2D> > Polygon2D;
std::string FeatureTypeToStr(int type);
///Derive a class from this to act as storage for the results. This
///class only defines the interface.
class DecodeVectorTileResults
{
public:
	DecodeVectorTileResults();
	virtual ~DecodeVectorTileResults();

	virtual void NumLayers(int numLayers);
	virtual void LayerStart(const char *name, int version, int extent);
	virtual void LayerEnd();
	virtual void Feature(int typeEnum, bool hasId, unsigned long long id, 
		const std::map<std::string, std::string> &tagMap,
		std::vector<Point2D> &pointsOut, 
		std::vector<std::vector<Point2D> > &linesOut,
		std::vector<Polygon2D> &polygonsOut);
	virtual void Finish();
};

///Main decoding class for vector tiles in pbf format
class DecodeVectorTile
{
public:
	DecodeVectorTile(int tileZoom, int tileColumn, int tileRow, class DecodeVectorTileResults &output);
	virtual ~DecodeVectorTile();
	void DecodeTileData(const std::string &data);

protected:
	class DecodeVectorTileResults *output;
	long unsigned int numTiles;
	double lonMin, latMax, lonMax, latMin, dLat, dLon;

	void DecodeGeometry(const ::vector_tile::Tile_Feature &feature,
		int extent,
		std::vector<Point2D> &pointsOut, 
		std::vector<std::vector<Point2D> > &linesOut,
		std::vector<Polygon2D> &polygonsOut);
};

///Main encoding class for vector tiles in pbf format
class EncodeVectorTile : public DecodeVectorTileResults
{
public:
	EncodeVectorTile(int tileZoom, int tileColumn, int tileRow, std::ostream &output);
	virtual ~EncodeVectorTile();

	virtual void NumLayers(int numLayers);
	virtual void LayerStart(const char *name, int version, int extent);
	virtual void LayerEnd();
	virtual void Feature(int typeEnum, bool hasId, unsigned long long id, 
		const std::map<std::string, std::string> &tagMap,
		std::vector<Point2D> &points, 
		std::vector<std::vector<Point2D> > &lines,
		std::vector<Polygon2D> &polygons);
	virtual void Finish();

protected:
	int tileZoom, tileColumn, tileRow;
	long unsigned int numTiles;
	double lonMin, latMax, lonMax, latMin, dLat, dLon;

	std::ostream *output;
	vector_tile::Tile tile;
	vector_tile::Tile_Layer *currentLayer;
	std::map<std::string, int> keysCache;
	std::map<std::string, int> valuesCache;

	void EncodePoints(const std::vector<Point2D> &points, 
		int cmdId,
		bool reverseOrder,
		size_t startIndex, 
		int extent, int &cursorx, int &cursory, vector_tile::Tile_Feature *outFeature);
	void EncodeGeometry(vector_tile::Tile_GeomType type,
		int extent,
		const std::vector<Point2D> &points, 
		const std::vector<std::vector<Point2D> > &lines,
		const std::vector<Polygon2D> &polygons,
		vector_tile::Tile_Feature *outFeature);
	void ConvertToTileCoords(const LineLoop2D &pts, 
		int extent, LineLoop2D &out);
};

int long2tilex(double lon, int z);
int lat2tiley(double lat, int z);
double tilex2long(int x, int z);
double tiley2lat(int y, int z);

#endif //_VECTOR_TILE_H

