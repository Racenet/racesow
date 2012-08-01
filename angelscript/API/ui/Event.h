/**
 * Event
 */
class Event
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	String@ getType();
	Element@ getTarget();
	String@ getParameter(const String&in, const String&in);
	Dictionary@ getParameters();
	int getPhase();
	void stopPropagation();
};

