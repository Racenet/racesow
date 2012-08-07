/**
 * Cvar
 */
class Cvar
{
public:
	/* object properties */

	/* object behaviors */
	void _beh_0_(const String&in, const String&in, const uint);&s
	void _beh_0_(const Cvar&in);&s

	/* object methods */
	void reset();
	void set(const String&in);
	void set(float);
	void set(int);
	void set(double);
	void forceSet(const String&in);
	void forceSet(float);
	void forceSet(int);
	void forceSet(double);
	bool get_modified() const;
	bool get_boolean() const;
	int get_integer() const;
	float get_value() const;
	String@ get_name() const;
	String@ get_string() const;
	String@ get_defaultString() const;
	String@ get_latchedString() const;
};

