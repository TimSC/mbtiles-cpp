#ifndef _VECTOR_TILE_H
#define _VECTOR_TILE_H

#include <string>
#include <map>
#include <vector>

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
	virtual void LayerStart(const char *name, int version);
	virtual void LayerEnd();
	virtual void Feature(int typeEnum, bool hasId, unsigned long long id, 
		const std::map<std::string, std::string> &tagMap,
		std::vector<Point2D> &pointsOut, 
		std::vector<std::vector<Point2D> > &linesOut,
		std::vector<Polygon2D> &polygonsOut);
};

///Main decoding class for vector tiles in pbf format
class DecodeVectorTile
{
public:
	DecodeVectorTile(class DecodeVectorTileResults &output);
	virtual ~DecodeVectorTile();
	void DecodeTileData(const std::string &data, int tileZoom, int tileColumn, int tileRow);

protected:
	class DecodeVectorTileResults *output;
};

int long2tilex(double lon, int z);
int lat2tiley(double lat, int z);
double tilex2long(int x, int z);
double tiley2lat(int y, int z);

#endif //_VECTOR_TILE_H

