/************************************************************************/
/* WARNING                                                              */
/* define this when we compile for a public release                     */
/* this will protect dangerous and untested pieces of code              */
/************************************************************************/
#define PUBLIC_BUILD

//==============================================
// wsw : jal :	these defines affect every project file. They are
//				work-in-progress stuff which is, sooner or later,
//				going to be removed by keeping or discarding it.
//==============================================

// pretty solid
#define MOREGRAVITY
#define ALT_ZLIB_COMPRESSION

// renderer config
#define CGAMEGETLIGHTORIGIN
#define HARDWARE_OUTLINES
#define CELLSHADEDMATERIAL
#define AREAPORTALS_MATRIX
//#define QUAKE2_JUNK

// collision config
#define TRACE_NOAXIAL // a hack to avoid issues with the return of traces against non axial planes

//#define ALLOWBYNNY_VOTE

//#define MATCHMAKER_SUPPORT

//==============================================
// undecided status

//#define ANTICHEAT_MODULE
#define PUTCPU2SLEEP

//#define UCMDTIMENUDGE
#ifdef MATCHMAKER_SUPPORT
# define TCP_SUPPORT
#endif
//#define TCP_ALLOW_CONNECT

#ifndef PUBLIC_BUILD
//#define WEAPONDEFS_FROM_DISK
#endif

#define DOWNSCALE_ITEMS // Ugly hack for the release. Item models are way too big
#define ELECTROBOLT_TEST

// collaborations
//==============================================

// symbol address retrieval
//==============================================
// #define SYS_SYMBOL		// adds "sys_symbol" command and symbol exports to binary release
#if defined ( SYS_SYMBOL ) && defined ( _WIN32 )
#define SYMBOL_EXPORT __declspec( dllexport )
#else
#define SYMBOL_EXPORT
#endif
