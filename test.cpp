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

	mbTileReader.ListTiles();
}

