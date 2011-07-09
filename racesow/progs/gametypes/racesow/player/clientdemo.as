/**
 * Racesow_Player_ClientDemo
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.6.2
 */

const int STOP_DELAY = 1000;

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

	/* Timestamp at which the stop command will be sent (based on levelTime)
	 * @var uint64
	 */
	uint64 sendTime;

	/**
	 * Demo control methods
	 */
	void sendStop()
	{
		this.player.getClient().execGameCommand( "dstop " + this.time );
		this.recording = false;
		this.time = 0;
	}
	void sendStart()
	{
		this.player.getClient().execGameCommand( "dstart" );
		this.recording = true;
	}
	void sendCancel()
	{
		this.player.getClient().execGameCommand( "dcancel" );
		this.recording = false;
		this.time = 0;
	}

	/**
	 * Constructor
	 */
	Racesow_Player_ClientDemo()
	{
		this.recording = false;
		this.time = 0;
	}

	/**
	 * Think
	 */
	void think()
	{
		if ( this.isStopping() && this.sendTime < levelTime )
			this.sendStop();
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
	 * Check if the stop command is sending
	 * @return bool
	 */
	bool isStopping()
	{
		if ( this.time > 0 )
			return true;
		return false;
	}

	/**
	 * Start recording
	 */
	void start()
	{
		this.sendStart();
	}
	/**
	 * Stop recording (delayed)
	 */
	void stop( uint time )
	{
		this.time = time;
		this.sendTime = levelTime + STOP_DELAY;
	}
	/**
	 * Stop recording instantly
	 * use only, if stop() was called before
	 */
	void stopNow()
	{
		if ( this.isStopping() )
			this.sendStop();
	}
	/**
	 * Stop recording and delete file
	 */
	void cancel()
	{
		this.sendCancel();
	}
}
