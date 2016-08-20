#include <iostream>
#include <sqlite3.h>
using namespace std;

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	for(int i=0; i<argc; i++){
		cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
	}
	cout << endl;
	return 0;
}

int main(int argc, char **argv){
	sqlite3 *db = NULL;

	int rc = sqlite3_open("cairo_egypt.mbtiles", &db);
	if(rc){
		cout << "Error opening database" << endl;
		sqlite3_close(db);
		return -1;
	}

	char *zErrMsg = 0;
	rc = sqlite3_exec(db, "SELECT * FROM metadata;", callback, 0, &zErrMsg);
	if( rc!=SQLITE_OK )
	{
		cout << "SQL error " << zErrMsg << endl;
		sqlite3_free(zErrMsg);
		return -1;
	}

	sqlite3_close(db);
	return 0;
}

