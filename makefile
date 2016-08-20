all:
	g++ test.cpp MBTileReader.cpp -lsqlite3 -o test
