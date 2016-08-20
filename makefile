all:
	g++ test.cpp MBTileReader.cpp ReadGzip.cpp vector_tile20/vector_tile.pb.cc VectorTile.cpp -lsqlite3 -lprotobuf-lite -lz -o test

