/**
 * Irc
 */
class Irc
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	bool get_connected();
	void connect();
	void connect(const String&inout, const int arg1 = 0);
	void disconnect();
	void join(const String&in);
	void join(const String&in, const String&in);
	void part(const String&in);
	void privateMessage(const String&in, const String&in);
	void mode(const String&in, const String&in);
	void mode(const String&in, const String&in, const String&in);
	void who(const String&in);
	void whois(const String&in);
	void whowas(const String&in);
	void quote(const String&in);
	void action(const String&in);
	void names(const String&in);
	void channelMessage(const String&in);
	void topic(const String&in);
	void topic(const String&in, const String&in);
	void kick(const String&in, const String&in);
	void kick(const String&in, const String&in, const String&in);
	void joinOnEndOfMotd(const String&in);
};

