/**
 * ElementTabSet
 */
class ElementTabSet
{
public:
	/* object properties */

	/* object behaviors */
	Element@ _beh_9_();&s

	/* object methods */
	void setTab(int, const String&in);
	void setTab(int, Element@);
	void setPanel(int, const String&in);
	void setPanel(int, Element@);
	void removeTab(int);
	int getNumTabs() const;
	void setActiveTab(int);
	int getActiveTab() const;
};

