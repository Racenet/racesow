/**
 * cVar
 */
class cVar
{
public:
	/* object properties */

	/* object behaviors */
	cVar@ f(); /* factory */ 
	cVar@ f( cString &in, cString &in, uint flags ); /* factory */ 

	/* object methods */
	void get( cString &in, cString &in, uint flags );
	void reset();
	void set( cString &in );
	void set( float value );
	void set( int value );
	void set( double value );
	void forceSet( cString &in );
	void forceSet( float value );
	void forceSet( int value );
	void forceSet( double value );
	bool modified();
	bool getBool();
	int getInteger();
	float getValue();
	cString @ getName();
	cString @ getString();
	cString @ getDefaultString();
	cString @ getLatchedString();
};

