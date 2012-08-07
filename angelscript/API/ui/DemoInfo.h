/**
 * DemoInfo
 */
class DemoInfo
{
public:
	/* object properties */

	/* object behaviors */
	void _beh_1_();&s
	void _beh_0_();&s
	void _beh_0_(const String&in);&s
	void _beh_0_(const DemoInfo&in);&s

	/* object methods */
	DemoInfo& opAssign(const DemoInfo&in);
	const bool get_isPlaying() const;
	const bool get_isPaused() const;
	const uint get_time() const;
	void play() const;
	void stop() const;
	void pause() const;
	void jump(uint) const;
	const String@ get_name() const;
	void set_name(const String&in) const;
	String@ getMeta(const String&in) const;
};

