#ifndef _READ_GZIP_H
#define _READ_GZIP_H

#include <zlib.h>
#include <streambuf>

class DecodeGzip : public std::streambuf
{
protected:
	char *readBuff;
	char *decodeBuff;
	std::streambuf &inStream;
	z_stream d_stream;
	bool decodeDone;
	char *decodeBuffCursor;
	std::streamsize readBuffSize, decodeBuffSize;

	void Decode();

	//Override streambuf virtual methods
	std::streamsize xsgetn (char* s, std::streamsize n);
	int uflow();
	std::streamsize showmanyc();
	std::streampos seekpos (std::streampos pos, std::ios_base::openmode which);

public:
	DecodeGzip(std::streambuf &inStream, std::streamsize readBuffSize = 1024*128, std::streamsize decodeBuffSize = 1024*128);
	virtual ~DecodeGzip();
};

#endif //_READ_GZIP_H

