#include "q_arch.h"
#include "q_math.h"
#include "q_shared.h"
#include "q_comref.h"

// Prejump/noprejump checking code

/**
 * Store the prejump state for each player
 */

int pj_jumps[MAX_CLIENTS]={0};
int pj_dashes[MAX_CLIENTS]={0};
int pj_walljumps[MAX_CLIENTS]={0};

void RS_ResetPjState(int playerNum)
{
    pj_jumps[playerNum]=0;
    pj_dashes[playerNum]=0;
    pj_walljumps[playerNum]=0;
}

/**
 increment jump/dash/wj count
*/

void RS_IncrementWallJumps(int playerNum)
{
    pj_walljumps[playerNum]++;
}

void RS_IncrementDashes(int playerNum)
{
    pj_dashes[playerNum]++;
}

void RS_IncrementJumps(int playerNum)
{
    pj_jumps[playerNum]++;
}

/**
 decide whether the player has prejumped or not
*/
qboolean RS_QueryPjState(int playerNum)
{
    // that printf needs to be commented as G_Printf is not supposed to be linked properly (but practically it is)
    G_Printf("Pj query for player %d: %d jumps, %d walljumps, %d dashes\n", playerNum, pj_jumps[playerNum], pj_walljumps[playerNum], pj_dashes[playerNum]);
    return  ( pj_jumps[playerNum] > 1 || pj_dashes[playerNum] > 1 || pj_walljumps[playerNum] > 1 );
}

