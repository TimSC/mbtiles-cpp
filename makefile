all:
	g++ example.cpp MBTileReader.cpp cppGzip/DecodeGzip.cpp vector_tile20/vector_tile.pb.cc VectorTile.cpp -lsqlite3 -lprotobuf -lz -Wall -o example
	g++ example2.cpp cppGzip/DecodeGzip.cpp vector_tile20/vector_tile.pb.cc VectorTile.cpp -lprotobuf -lz -Wall -o example2
