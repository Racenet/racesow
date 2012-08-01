/* funcdefs */

/**
 * cItem
 */
class cItem
{
public:
	/* object properties */
	const int tag;
	const uint type;
	const int flags;
	const int quantity;
	const int inventoryMax;
	const int ammoTag;
	const int weakAmmoTag;

	/* object behaviors */
	cItem@ f(); /* factory */ 

	/* object methods */
	String @ getClassname() const;
	String @ getClassName() const;
	String @ getName() const;
	String @ getShortName() const;
	String @ getModelString() const;
	String @ getModel2String() const;
	String @ getIconString() const;
	String @ getSimpleItemString() const;
	String @ getPickupSoundString() const;
	String @ getColorToken() const;
	bool isPickable() const;
	bool isUsable() const;
	bool isDropable() const;
};

