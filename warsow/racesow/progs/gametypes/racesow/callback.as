/**
* Racesow callbacks 
*
* @package Racesow
* @version 0.5.2 (initially)
* @date 01.06.2010 (initially)
* @author r2
*/

/**
 * callback codes
 */
const uint RACESOW_CALLBACK_AUTHENTICATE=0;
 
 /**
 * Racesow_ThinkCallbackQueue()
 * perform a callback if there is at least one pending
 */
void Racesow_ThinkCallbackQueue()
{
	int command;
	int arg1;
	int arg2;
	int arg3;
	bool do_action;
	do_action=RS_QueryCallbackQueue(command,arg1,arg2,arg3);
	if (do_action)
		if (command==RACESOW_CALLBACK_AUTHENTICATE)	
			RS_MysqlAuthenticate_Callback(arg1,arg2,arg3);

}