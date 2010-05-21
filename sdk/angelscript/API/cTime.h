/**
 * cTime
 */
class cTime
{
public:
	/* object properties */
	const uint64 time;
	const int sec;
	const int min;
	const int hour;
	const int mday;
	const int mon;
	const int year;
	const int wday;
	const int yday;
	const int isdst;

	/* object behaviors */
	cTime @ f(); /* factory */ 
	cTime @ f(uint64 t); /* factory */ 
	cTime & f(const cTime &in); /* = */

	/* global behaviors */
	bool f(const cTime &in, const cTime &in); /* == */
	bool f(const cTime &in, const cTime &in); /* != */
};

