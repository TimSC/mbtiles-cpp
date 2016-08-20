#ifndef _VECTOR_TILE_H
#define _VECTOR_TILE_H

#include <string>

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
};

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

