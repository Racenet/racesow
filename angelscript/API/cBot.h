/**
 * cBot
 */
class cBot
{
public:
	/* object properties */
	const float skill;
	const int currentNode;
	const int nextNode;
	uint moveTypesMask;
	const float reactionTime;
	const float offensiveness;
	const float campiness;
	const float firerate;

	/* object behaviors */
	cBot @ f(); /* factory */ 

	/* object methods */
	void clearGoalWeights();
	void resetGoalWeights();
	void setGoalWeight( int i, float weight );
	cEntity @ getGoalEnt( int i );
	float getItemWeight( cItem @item );
};

