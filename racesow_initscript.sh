#!/bin/sh

# init.d-script to control the racesow servers
# you also may want to add a cronjrob with the check command to ensure your servers are always running
# requires: screen, start-stop-deamon, quakestat

# The user which should run the gameservers
GAMEUSER=warsow

SCREEN_NAME=warsow-0.5

# Folder to store PID files (writeable)
PATH_PIDS=/home/warsow/pids

# Warsow root directory
PATH_WARSOW=/home/warsow/warsow-0.5

# The gameserver executable
GAMESERVER=wsw_server.x86_64

# The mod directory (ie. basewsw)
MODDIR=racesow

#Path quakestat
QUAKESTAT=quakestat

#Hostname or IP for qstat queries
HOST=localhost

# The start-stop-daemon executable
DAEMON=/sbin/start-stop-daemon

# List of all handeled gameserver ports
PORTS="44400 44401 44402"

# DO NOT EDIT  BELOW THIS LINE
THISFILE=$0
SUDO="sudo -u $GAMEUSER -H"

# get the process id of the main screen
function get_main_screen_pid
{
	return `$SUDO screen -ls | pcregrep "\d+\.$SCREEN_NAME" | awk -F "." '{printf "%d",$1}'`
}
	
# Start a server loop for the given port
function gameserver_start
{
	get_main_screen_pid
	if [ "$?" == "0" ]; then
		echo "starting main screen."
		`$SUDO screen -dmS $SCREEN_NAME`
	fi

	if [ "$1" == "" ]; then
		for PORT in $PORTS; do
        	gameserver_start $PORT
	    done
    else
        PORTCHECK=$(echo $PORTS | grep $1)
		if [ "$PORTCHECK" != "" ];then
			if [ ! -f $PATH_WARSOW/$MODDIR/cfgs/port_$1.cfg ]; then
				echo "WARNING: no config found for $1"
			fi

			gameserver_check_pid $1
			if [ $? == 0 ]; then				
				PORT=$1
				CMD="$SUDO screen -S $SCREEN_NAME -X screen -t "$GAMEUSER"_"$PORT" $DAEMON --pidfile $PATH_PIDS/$PORT.pid --make-pidfile --chuid $GAMEUSER:$GAMEUSER --start --chdir $PATH_WARSOW --exec $PATH_WARSOW/$GAMESERVER +set fs_game $MODDIR +exec cfgs/port_"$PORT".cfg"
				`$CMD`
				#echo $CMD
			else
				echo "server $1 is already running"
				exit
			fi
			return 1
		else
			echo "server $1 is not configured"
			exit
		fi
	fi
}

# stop gameserver(s)
function gameserver_stop
{
	if [ "$1" == "" ]; then
        	for PORT in $PORTS; do
        		gameserver_start $PORT
	        done
        else
        	PORTCHECK=$(echo $PORTS | grep $1)
		if [ "$PORTCHECK" != "" ];then
			if [ ! -f $PATH_WARSOW/$MODDIR/cfgs/port_$1.cfg ]; then
				echo "WARNING: no config found for $1"
			fi

			gameserver_check_pid $1
			if [ $? != 0 ]; then				
				PORT=$1
				CMD="$SUDO $DAEMON --pidfile $PATH_PIDS/$PORT.pid --make-pidfile --chuid $GAMEUSER:$GAMEUSER --stop --chdir $PATH_WARSOW --exec $PATH_WARSOW/$GAMESERVER +set fs_game $MODDIR +exec cfgs/port_"$PORT".cfg"
				rm -f $PATH_PIDS/$PORT.pid
				`$CMD`
			else
				echo "server $1 is not running"
				exit
			fi
		else
			echo "server $1 is not configured"
			exit
		fi
	fi
}

# Kill server loop with the given port
function gameserver_kill
{
	if [ "$1" == "" ]; then
		echo "you need to specify a port for killing a server"
		exit
	else
		PORTCHECK=$(echo $PORTS | grep $1)
		if [ "$PORTCHECK" != "" ]; then
			gameserver_check_pid $1
			if [ $? == 1 ]; then
				echo "killing server warsow://$HOST:$1"
				kill -n 15 `cat $PATH_PIDS/$1.pid`
				rm -f  $PATH_PIDS/$1.pid
			else
				echo "server $1 is not running"
				exit
			fi
		else
			echo "server $1 is not configured"
			exit
		fi
	fi
}

# check if server on port $1 is running, also removes old pidfiles if necesary
function gameserver_check_pid
{
	if [ ! -f $PATH_PIDS/$1.pid ]; then
		return 0
	fi
	SERVERPID=$(cat $PATH_PIDS/$1.pid)
	TEST=$(echo $SERVERPID | xargs ps -fp | grep $GAMESERVER)
	if [ "$TEST" == "" ]; then
		rm -f $PATH_PIDS/$1.pid
		return 0
	else
		return 1
	fi
}

# check if the server is running, kills hanging gameserver processes
function gameserver_check_gamestate
{
	if [ "$1" == "" ]; then
		for PORT in $PORTS; do
			gameserver_check_gamestate $PORT
		done
	else
		SERVERTEST=`$QUAKESTAT -warsows $HOST:$1 | pcregrep "$HOST:$1 +(DOWN|no response)"`
		if [ "$SERVERTEST" != "" ]; then
			echo "warsow://$HOST:$1 is not reachable via qstat"
			
			echo "* trying to find the process by it's pid"
			gameserver_check_pid $1
			if [ $? == 0 ]; then
				echo "* gameserver not found, starting on port $1"
				gameserver_start $1
			else
				echo "* restarting gameserver on port $1"
				gameserver_stop $1
				gameserver_start $1
			fi
		fi
	fi
}

# Check and run the action
case $1 in
	start)  	gameserver_start $2;;
    stop)   	gameserver_stop $2;;
	check)		gameserver_check_gamestate $2;;
	*)		echo "Usage: $0 {start|stop|check}";;
esac