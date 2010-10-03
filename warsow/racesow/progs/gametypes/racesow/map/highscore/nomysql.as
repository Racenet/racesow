/**
 * Racesow_Map_HighScore_NoMysql
 *
 * Manage stats at runtime.
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.6
 */
class Racesow_Map_HighScore_NoMysql : Racesow_Map_HighScore_Abstract
{
    /**
     * Add a race to the highscores
     * @param Racesow_Player_Race @race
     * @return void
     */
    void addRace(Racesow_Player_Race @race)
    {
        race.getPlayer().raceCallback(0,0,0,
                race.getPlayer().bestRaceTime,
                map.getStatsHandler().getHighScore(0).getTime(),
                race.getTime());

        race.getPlayer().getClient().addAward( S_COLOR_CYAN + "Race Finished!" );
        race.getPlayer().sendMessage(S_COLOR_WHITE
                + "race finished: " + TimeToString( race.getTime() ) + "\n");
    }
}
