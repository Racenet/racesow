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
const uint RACESOW_CALLBACK_AUTHENTICATE = 0;
const uint RACESOW_CALLBACK_NICKPROTECT = 1;
 
 /**
 * Racesow_ThinkCallbackQueue()
 * perform a callback if there is at least one pending
 */
void Racesow_ThinkCallbackQueue()
{
	int command, arg1, arg2, arg3;
    
    if( !RS_QueryCallbackQueue( command, arg1, arg2, arg3 ) )
        return;
        
    switch( command )	
    {
        case RACESOW_CALLBACK_AUTHENTICATE:
            Racesow_GetPlayerByNumber(arg1).getAuth().authCallback( arg2, arg3 );
            break;
            
        case RACESOW_CALLBACK_NICKPROTECT:
            Racesow_GetPlayerByNumber(arg1).getAuth().nickProtectCallback( arg2 );
            break;
    }
}