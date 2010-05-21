
//====================================================
// simple and portable swap
//====================================================

#include "misc_utils.h"

short ShortSwap (short l)
{
	unsigned char b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short ShortNoSwap (short l)
{
	return l;
}

int LongSwap (int l)
{
	unsigned char b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float f;
		unsigned char b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

void InitSwapFunctions (void)
{
	unsigned char swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1)
	{
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
}


//====================================================
//  Write binary file to disk
//====================================================

#include <stdlib.h>
#include <stdio.h>

unsigned char *output;
unsigned char *outputbuffer;

//=============================================================
//=============================================================

void writefile (char *filename, void *buffer, int size)
{
	int size1;
	FILE *file;

	file = fopen (filename, "wb");
	if (!file)
	{
		printf ("unable to open file \"%s\" for writing\n", filename);
		return;
	}

	size1 = fwrite (buffer, 1, size, file);
	fclose (file);

	if (size1 < size)
	{
		printf ("unable to write file \"%s\"\n", filename);
		return;
	}
}


void putwritefile (char *filename, int size)
{
	if (!outputbuffer) return;

	writefile (filename, outputbuffer, size);
	
	free(outputbuffer);
	outputbuffer = NULL;
}



void putstring (char *in, int length)
{
	if( !outputbuffer )
		return;

	while (*in && length)
	{
		*output++ = *in++;
		length--;
	}

	// pad with nulls
	while (length--)
		*output++ = 0;
}

void putnulls (int num)
{
	if( !outputbuffer )
		return;

	while (num--)
		*output++ = 0;
}

void putlong (int num)
{
	if( !outputbuffer )
		return;

	*output++ = ((num >>  0) & 0xFF);
	*output++ = ((num >>  8) & 0xFF);
	*output++ = ((num >> 16) & 0xFF);
	*output++ = ((num >> 24) & 0xFF);
}

void putfloat (double num)
{
	union
	{
		float f;
		int i;
	}
	n;

	if( !outputbuffer )
		return;

	n.f = (float)num;

	// this matches for both positive and negative 0, thus setting it to positive 0
	if (n.f == 0)
		n.f = 0;

	putlong (n.i);
}

void putinit (int max_filesize)
{
	if( !max_filesize )
		return;

	outputbuffer = malloc( sizeof(unsigned char) * max_filesize );
	output = outputbuffer;
}

int putgetposition (void)
{
	if( !outputbuffer )
		return 0;

	return (int) output - (int) outputbuffer;
}

void putsetposition (int n)
{
	if( !outputbuffer )
		return;

	output = (unsigned char *)(n + (int) outputbuffer);
}


void *readfilefromdisk(const char *filename, int *size)
{
	FILE *file;
	void *filedata;
	size_t filesize;
	*size = 0;
	file = fopen(filename, "rb");
	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	filedata = malloc(filesize);
	if (fread(filedata, 1, filesize, file) < filesize)
	{
		free(filedata);
		fclose(file);
		return NULL;
	}
	fclose(file);
	*size = filesize;
	return filedata;
}


//====================================================
//  Read file from disk
//====================================================

void *loadfile(const char *filename, int *size)
{
	FILE *file;
	void *filedata;
	size_t filesize;
	*size = 0;
	file = fopen(filename, "rb");
	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	filedata = malloc(filesize);
	if (fread(filedata, 1, filesize, file) < filesize)
	{
		free(filedata);
		fclose(file);
		return NULL;
	}
	fclose(file);
	*size = filesize;
	return filedata;
}

