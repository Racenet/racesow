/**
 * Racesow_Player_ClientDemo
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.6.2
 */

const uint CLIENTDEMO_STOP_DELAY = 1000;
const uint CLIENTDEMO_COMMAND_DELAY = 200;

class Racesow_Player_ClientDemo : Racesow_Player_Implemented
{

	/* Demo Status
	 * @var bool
	 */
	bool recording;

	/* Indicates if the start command is in queue
	 * @var bool
	 */
	bool starting;

	/* Time at finish
	 * @var uint
	 */
	uint time;

	/* Timestamp at which the stop command will be sent (based on realTime)
	 * @var uint
	 */
	uint stopTime;

	/* Timestamp at which the last command was sent (based on realTime)
	 * @var uint
	 */
	uint lastCommandTime;

	/**
	 * Demo control methods
	 */
	void sendStop()
	{
		this.player.getClient().execGameCommand( "dstop " + this.time );
		this.lastCommandTime = realTime;
		this.recording = false;
		this.time = 0;
	}
	void sendStart()
	{
		this.player.getClient().execGameCommand( "dstart" );
		this.lastCommandTime = realTime;
		this.recording = true;
		this.starting = false;
	}
	void sendCancel()
	{
		this.player.getClient().execGameCommand( "dcancel" );
		this.lastCommandTime = realTime;
		this.recording = false;
		this.time = 0;
	}

	/**
	 * Constructor
	 */
	Racesow_Player_ClientDemo()
	{
		this.recording = false;
		this.starting = false;
		this.time = 0;
		this.stopTime = 0;
		this.lastCommandTime = 0;
	}

	/**
	 * Think
	 */
	void think()
	{
		if( this.lastCommandTime + CLIENTDEMO_COMMAND_DELAY < realTime )
		{
			if( this.isStopping() && this.stopTime < realTime )
				this.sendStop();
			if( this.isStarting() )
				this.sendStart();
		}
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
	 * Check if the start command is in queue
	 * @return bool
	 */
	bool isStarting()
	{
		return this.starting;
	}

	/**
	 * Check if the stop command is in queue
	 * @return bool
	 */
	bool isStopping()
	{
		return ( this.time > 0 );
	}

	/**
	 * Start recording
	 * @return bool
	 */
	bool start()
	{
		if( this.isRecording() || this.isStarting() )
			return false;
		if( this.isStopping() )
			this.stopNow();
		this.starting = true;
		this.think();
		return true;
	}
	/**
	 * Stop recording (delayed)
	 */
	void stop( uint time )
	{
		this.time = time;
		this.stopTime = realTime + CLIENTDEMO_STOP_DELAY;
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
