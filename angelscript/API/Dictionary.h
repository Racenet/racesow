/**
 * Dictionary
 */
class Dictionary
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
	Dictionary& opAssign(const Dictionary&in);
	void set(const String&in, ?&in);
	bool get(const String&in, ?&out) const;
	void set(const String&in, int64&in);
	bool get(const String&in, int64&out) const;
	void set(const String&in, double&in);
	bool get(const String&in, double&out) const;
	bool exists(const String&in) const;
	void delete(const String&in);
	void clear();
	bool empty() const;
	String@[]@ getKeys() const;
};

