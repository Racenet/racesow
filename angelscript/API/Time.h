/**
 * Time
 */
class Time
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
	void _beh_0_();&s
	void _beh_0_(uint64);&s
	void _beh_0_(const Time&in);&s

	/* object methods */
	Time& opAssign(const Time&in);
	bool opEquals(const Time&in, const Time&in);
};

