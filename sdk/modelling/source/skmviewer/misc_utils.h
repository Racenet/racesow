
//====================================================
// simple and portable swap
//====================================================

short (*BigShort) (short l);
short (*LittleShort) (short l);
int (*BigLong) (int l);
int (*LittleLong) (int l);
float (*BigFloat) (float l);
float (*LittleFloat) (float l);

short ShortSwap (short l);
short ShortNoSwap (short l);
int LongSwap (int l);
int LongNoSwap (int l);
float FloatSwap (float f);
float FloatNoSwap (float f);
void InitSwapFunctions (void);


//====================================================
// write file to disk
//====================================================
void putwritefile (char *filename, int size);

void putstring (char *in, int length);
void putnulls (int num);
void putlong (int num);
void putfloat (double num);
void putinit (int max_filesize);
int putgetposition (void);
void putsetposition (int n);

//====================================================
// read file from disk
//====================================================
void *loadfile(const char *filename, int *size);

