/**
 * Window
 */
class Window
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	void open(const URL&in);
	void close(int arg0 = 0);
	void modal(const String&inout, int arg1 = - 1);
	int getModalValue() const;
	ElementDocument@ get_document() const;
	URL get_location() const;
	void set_location(const URL&in);
	uint get_time() const;
	bool get_drawBackground() const;
	int get_width() const;
	int get_height() const;
	uint history_size() const;
	void history_back() const;
	void startLocalSound(const String&in) const;
	void startBackgroundTrack(String&in, String&in, bool arg2 = true) const;
	void stopBackgroundTrack() const;
	int setTimeout(TimerCallback@, uint);
	int setInterval(TimerCallback@, uint);
	int setTimeout(TimerCallback2@, uint, any&in);
	int setInterval(TimerCallback2@, uint, any&in);
	void clearTimeout(int);
	void clearInterval(int);
	void flash(uint);
};

