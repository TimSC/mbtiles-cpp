#ifndef _VECTOR_TILE_H
#define _VECTOR_TILE_H

#include <string>

class DecodeVectorTileResults
{
public:

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
