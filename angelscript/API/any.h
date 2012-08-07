/**
 * any
 */
class any
{
public:
	/* object properties */

	/* object behaviors */
	int _beh_11_();&s
	void _beh_12_();&s
	bool _beh_13_();&s
	void _beh_14_(int&in);&s
	void _beh_15_(int&in);&s

	/* object methods */
	any& opAssign(any&in);
	void store(?&in);
	void store(int64&in);
	void store(double&in);
	bool retrieve(?&out);
	bool retrieve(int64&out);
	bool retrieve(double&out);
};

