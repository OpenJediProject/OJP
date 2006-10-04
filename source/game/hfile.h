
//header protection
#ifndef _HFILE_H_
#define _HFILE_H_

class hfile
{
public:
	//constructor (opens file) by char *
	hfile(char *);

	//sets the file to read mode for this version of the navigation file data.
	bool open_read( int version, int checkSum );

	//loads a chunk of data from the file.
	void load(void *, size_t);

	//closes the file
	void close();

	//sets the file to write mode for this version of the navigation file data.
	bool open_write( int version, int checkSum );

	//saves data to file.
	void save( void* data, int dataSize);

};

#endif