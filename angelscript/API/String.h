/**
 * String
 */
class String
{
public:
	/* object properties */

	/* object behaviors */
	int _beh_7_() const;&s
	float _beh_7_() const;&s
	double _beh_7_() const;&s
	URL _beh_7_();&s

	/* object methods */
	String& opAssign(const String&in);
	String& opAssign(int);
	String& opAssign(double);
	String& opAssign(float);
	uint8& opIndex(uint);
	const uint8& opIndex(uint) const;
	String& opAddAssign(const String&in);
	String& opAddAssign(int);
	String& opAddAssign(double);
	String& opAddAssign(float);
	String@ opAdd(const String&in) const;
	String@ opAdd(int) const;
	String@ opAdd_r(int) const;
	String@ opAdd(double) const;
	String@ opAdd_r(double) const;
	String@ opAdd(float) const;
	String@ opAdd_r(float) const;
	bool opEquals(const String&in) const;
	uint len() const;
	uint length() const;
	bool empty() const;
	String@ tolower() const;
	String@ toupper() const;
	String@ trim() const;
	String@ removeColorTokens() const;
	String@ getToken(const uint) const;
	int toInt() const;
	float toFloat() const;
	uint locate(String&inout, const uint) const;
	String@ substr(const uint, const uint) const;
	String@ subString(const uint, const uint) const;
	String@ substr(const uint) const;
	String@ subString(const uint) const;
	bool isAlpha() const;
	bool isNumerical() const;
	bool isNumeric() const;
	bool isAlphaNumerical() const;
};

