/**
 * cString
 */
class cString
{
public:
	/* object behaviors */
	cString @ f(); /* factory */ 
	cString @ f(const cString &in); /* factory */ 
	cString & f(const cString &in); /* = */
	cString & f(int); /* = */
	cString & f(double); /* = */
	cString & f(float); /* = */
	uint8 & f(uint);
	const uint8 & f(uint);
	cString & f(cString &in); /* += */
	cString & f(int); /* += */
	cString & f(double); /* += */
	cString & f(float); /* += */

	/* global behaviors */
	cString @ f(const cString &in, const cString &in); /* + */
	cString @ f(const cString &in, int); /* + */
	cString @ f(int, const cString &in); /* + */
	cString @ f(const cString &in, double); /* + */
	cString @ f(double, const cString &in); /* + */
	cString @ f(const cString &in, float); /* + */
	cString @ f(float, const cString &in); /* + */
	bool f(const cString &in, const cString &in); /* == */
	bool f(const cString &in, const cString &in); /* != */

	/* object methods */
	int len();
	int length();
	cString @ tolower();
	cString @ toupper();
	cString @ trim();
	cString @ removeColorTokens();
	cString @ getToken(const int);
	int toInt();
	float toFloat();
	cVec3 & toVec3();
	int locate(cString &, const int);
	cString @ substr(const int, const int);
	cString @ subString(const int, const int);
};

