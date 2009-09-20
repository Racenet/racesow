/**
 * cVec3
 */
class cVec3
{
public:
	/* object properties */
	float x;
	float y;
	float z;

	/* object behaviors */
	cVec3 @ f(); /* factory */ 
	cVec3 @ f(float x, float y, float z); /* factory */ 
	cVec3 @ f(float v); /* factory */ 
	cVec3 & f(cVec3 &in); /* = */
	cVec3 & f(int); /* = */
	cVec3 & f(float); /* = */
	cVec3 & f(double); /* = */
	cVec3 & f(cVec3 &in); /* += */
	cVec3 & f(cVec3 &in); /* -= */
	cVec3 & f(cVec3 &in); /* *= */
	cVec3 & f(cVec3 &in); /* ^| */
	cVec3 & f(int); /* *= */
	cVec3 & f(float); /* *= */
	cVec3 & f(double); /* *= */

	/* global behaviors */
	cVec3 @ f(const cVec3 &in, const cVec3 &in); /* + */
	cVec3 @ f(const cVec3 &in, const cVec3 &in); /* - */
	float f(const cVec3 &in, const cVec3 &in); /* * */
	cVec3 @ f(const cVec3 &in, double); /* * */
	cVec3 @ f(double, const cVec3 &in); /* * */
	cVec3 @ f(const cVec3 &in, float); /* * */
	cVec3 @ f(float, const cVec3 &in); /* * */
	cVec3 @ f(const cVec3 &in, int); /* * */
	cVec3 @ f(int, const cVec3 &in); /* * */
	cVec3 @ f(const cVec3 &in, const cVec3 &in); /* ^ */
	bool f(const cVec3 &in, const cVec3 &in); /* == */
	bool f(const cVec3 &in, const cVec3 &in); /* != */

	/* object methods */
	void set( float x, float y, float z );
	float length();
	float normalize();
	cString @ toString();
	float distance(const cVec3 &in);
	void angleVectors(cVec3 @+, cVec3 @+, cVec3 @+);
	void toAngles(cVec3 &);
	void perpendicular(cVec3 &);
	void makeNormalVectors(cVec3 &, cVec3 &);
};

