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
	cString @ getClassname();
	cString @ getClassName();
	cString @ getName();
	cString @ getShortName();
	cString @ getModelString();
	cString @ getModel2String();
	cString @ getIconString();
	cString @ getSimpleItemString();
	cString @ getPickupSoundString();
	cString @ getColorToken();
	bool isPickable();
	bool isUsable();
	bool isDropable();
};

