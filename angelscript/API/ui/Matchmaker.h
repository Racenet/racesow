/**
 * Matchmaker
 */
class Matchmaker
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	bool login(const String&in, const String&in);
	bool logout();
	int get_state() const;
	String@ get_lastError() const;
	String@ get_user() const;
	String@ profileURL(bool) const;
	String@ baseWebURL() const;
};

