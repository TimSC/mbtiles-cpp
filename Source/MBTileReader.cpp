#include "MBTileReader.h"
#include <iostream>
#include <cstring>
#include <memory>
#include <stdlib.h>
#include <math.h>
using namespace std;

struct ListTilesContext {
	TileInfoRows *rows;
	string errorMsg;
};

MBTileReader::MBTileReader(const char *filename)
{
	this->db = NULL;

	int status = sqlite3_open(filename, &this->db);
	if(status){
		sqlite3_close(this->db);
		throw runtime_error("Error opening database");
	}

	char *zErrMsg = NULL;
	status = sqlite3_exec(this->db, "SELECT * FROM metadata;", this->MetadataCallbackStatic, this, &zErrMsg);
	if( status!=SQLITE_OK )
	{
		string err("SQL error: ");
		err += zErrMsg;
		sqlite3_free(zErrMsg);
		throw runtime_error(err);
	}
}

MBTileReader::~MBTileReader()
{
	sqlite3_close(this->db);
}

int MBTileReader::MetadataCallbackStatic(void *obj, int argc, char **argv, char **azColName)
{
	return ((MBTileReader*)obj)->MetadataCallback(argc, argv, azColName);
}

int MBTileReader::MetadataCallback(int argc, char **argv, char **azColName)
{
	string name, value;
	for(int i=0; i<argc; i++){
		if (strncmp(azColName[i], "name", 5) == 0)
			name = (argv[i] ? argv[i] : "NULL");
		if (strncmp(azColName[i], "value", 6) == 0)
			value = (argv[i] ? argv[i] : "NULL");		
	}
	if (name.size() > 0)
		this->metadata[name] = value;
	return 0;
}

string MBTileReader::GetMetadata(const char *metaField)
{
	auto it = metadata.find(metaField);
	return it != metadata.end() ? it->second : string{};
}

void MBTileReader::GetMetadataFields(vector<string> &fieldNamesOut)
{
	fieldNamesOut.clear();
	for(std::map<std::string, std::string>::iterator it=metadata.begin(); it!=metadata.end(); it++)
		fieldNamesOut.push_back(it->first);
}

int MBTileReader::ListTilesCallbackStatic(void *rawPtr, int argc, char **argv, char **azColName)
{
	ListTilesContext *ctx = static_cast<ListTilesContext *>(rawPtr);

	if(argc != 3) {
		ctx->errorMsg = "Database returned unexpected number of columns";
		return 1;
	}
	if (argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
		ctx->errorMsg = "Invalid tile descriptor";
		return 1;
	}
	std::vector<unsigned int> row;
	row.push_back((unsigned int)atoi(argv[0]));
	row.push_back((unsigned int)atoi(argv[1]));
	row.push_back((unsigned int)atoi(argv[2]));
	ctx->rows->push_back(row);
	return 0;
}

void MBTileReader::ListTiles(TileInfoRows &tileInfoRowsOut)
{
	ListTilesContext ctx{&tileInfoRowsOut, ""};
	char *zErrMsg = NULL;
	int status = sqlite3_exec(this->db, "SELECT zoom_level, tile_column, tile_row FROM tiles;",
		this->ListTilesCallbackStatic, &ctx, &zErrMsg);
	if (!ctx.errorMsg.empty())
		throw runtime_error(ctx.errorMsg);
	if( status!=SQLITE_OK )
	{
		string err("SQL error: ");
		err += zErrMsg;
		sqlite3_free(zErrMsg);
		throw runtime_error(err);
	}
}

void MBTileReader::GetTile(unsigned int zoomLevel, 
	unsigned int tileColumn, 
	unsigned int tileRow,
	string &blobOut)
{
	sqlite3_stmt *stmtRaw = NULL;
	blobOut.clear();
	int status = sqlite3_prepare(this->db,
		"SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;",
		-1,
		&stmtRaw,
		0
	);
	if(status!=SQLITE_OK)
		throw runtime_error("Could not prepare statement");

	unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(stmtRaw, sqlite3_finalize);

	status = sqlite3_bind_int(stmtRaw, 1, zoomLevel);
	if(status!=SQLITE_OK) throw runtime_error("Could not bind value to statement");
	status = sqlite3_bind_int(stmtRaw, 2, tileColumn);
	if(status!=SQLITE_OK) throw runtime_error("Could not bind value to statement");
	status = sqlite3_bind_int(stmtRaw, 3, tileRow);
	if(status!=SQLITE_OK) throw runtime_error("Could not bind value to statement");

	status = SQLITE_ROW;
	bool firstRow = true;
	while(status == SQLITE_ROW)
	{
		status = sqlite3_step(stmtRaw);
		if(firstRow && status == SQLITE_DONE)
			throw out_of_range("Tile not found");

		if(status == SQLITE_ROW)
		{
			int numBytes = sqlite3_column_bytes(stmtRaw, 0);
			const void *blobRaw = sqlite3_column_blob(stmtRaw, 0);
			blobOut.assign((const char *)blobRaw, numBytes);
			return;
		}
		firstRow = false;
	}

	throw runtime_error("Error retrieving tile from database");
}

