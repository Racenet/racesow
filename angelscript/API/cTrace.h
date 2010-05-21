/**
 * cTrace
 */
class cTrace
{
public:
	/* object properties */
	const bool allSolid;
	const bool startSolid;
	const float fraction;
	const int surfFlags;
	const int contents;
	const int entNum;
	const float planeDist;
	const int16 planeType;
	const int16 planeSignBits;

	/* object behaviors */
	cTrace@ f(); /* factory */ 

	/* object methods */
	bool doTrace( cVec3 &in, cVec3 &, cVec3 &, cVec3 &in, int ignore, int contentMask );
	cVec3 @ getEndPos();
	cVec3 @ getPlaneNormal();
};

