/**
 * Racesow_Player_ClientDemo
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.6.2
 */
class Racesow_Player_ClientDemo : Racesow_Player_Implemented
{
	/* Demo Status
	 * @var bool
	 */
    bool recording;

	/* Time at finish
	 * @var uint
	 */
    uint time;

	/**
	 * Constructor
	 *
	 */
	Racesow_Player_ClientDemo()
    {
		this.recording = false;
    }

	/**
	 * Getter/Setter methods
	 */
	void setTime( uint time )
	{
		this.time = time;
	}
	uint getTime()
	{
		return this.time;
	}

	/**
	 * Check if a demo is currently being recorded
	 * @return bool
	 */
	bool isRecording()
	{
		return this.recording;
	}

	/**
	 * Demo control methods
	 */
	void start()
	{
		this.player.getClient().execGameCommand( "dstart" );
		this.recording = true;
	}
	void stop()
	{
		this.player.getClient().execGameCommand( "dstop " + this.time );
		this.recording = false;
	}
	void cancel()
	{
		this.player.getClient().execGameCommand( "dcancel" );
		this.recording = false;
	}
}
