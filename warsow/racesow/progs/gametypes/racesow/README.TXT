/**
 * Racesow README file
 *
 * @package Racesow
 * @version 0.5.1c
 * @date 23.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
 */

    Project goals:

        - no changes to game module which are required to run the gametype to allow easy setup
        - no use of external services, instead effectively (!) based on files
        - bring the race community together <3 (!!!!)
        - coding for fun while learning a lot
        - contribute to warsow

		
		
    Sources:

        - development trunk: http://svn.warsow-race.net/racesow0.5/t … gametypes/
        - latest stable version: http://svn.warsow-race.net/racesow0.5/t … gametypes/

		
		
    Game cvars:

        g_logRaces 1|0, default 0, log all races to a file
        g_secureAuth 1|0, default 0, use md5 hashing for auth passwords, §
        g_freestyle 1|0, default 0, enables freestyle mode (not implemented yet)

		
		
    Game commands:
       
        racerestart, go back to the start-area whenever you want
        register <name> <email> <password> <password confirmation>
        auth <name> <password>
        admin <command params>
            admin map <map name> (not implemented yet)
        help, shows the help

        you can add this to your autoexec.cfg to auto-login when connecting to a racesow server
            setu auth_login "name"
            setu auth_pass "password"

			
			
    Racenet/Racesow team:

        core team
            FoV: development (racesow), takes gameserver costs <3
            soh-zolex: project founder, management, public relations, lead coder, server costs

        inactive:
            weqo: development (racenet)

        ppl currently getting into
            R2: development (racesow)
            valliko: development (racesow)
            Zaran: development (racenet)

        ppl who contributed before 0.5
            imhotep
            Jezus
            joko

        are you missing in this list? tell me...

        we're still searching for people interested in helping with racesow or contributing their own race modifications.
        join us in IRC #racenet@quakenet

		
		
    Changelog: (items with a § require the modded game module)

        2009-09-23 / racesow 0.5.1c
            - extracted auth plugin from player model
            - added very nice nickname protection!
            - removed own md5 sources as they are in qcommon
			- removed g_secureAuth as it won't solve our problem
            - changed makefile and vcproject for md5 stuff
            - changed console command workflows

        2009-09-22 / racesow 0.5.1b
            - added fix for memory management by vic, thanks to jal §
            - added md5 encryption for passwords §
            - added g_secureAuth 0 to disable md5 encryption
            - added help command
            - changed authorization to create some subfolders
            - changed authentication to create subfolders
            - changed commands racesow_register and racesow_auth to register and auth
            - changed all files to lowercase for linux
            - fixed possible authentication exploit

        2009-09-21 / racesow 0.5.1b
            - added race-overtime feature
            - added idle detection when in overtime
            - added an abstract interface for handling race highscores
            - added basic authentication
            - added authorization
            - added admin command "map"
            - added logging of races

        2009-09-19 / racesow 0.5.1a
            - refactored whole race gameytpe (better OOP)
            - changed racerestart command so it can be used at any time
            - changed awards for checkpoints and races
            - fixed autorespawn bug when finishing a race
			
			
			
	Roadmap: (items with a § require the modded game module, or wait for stuff be added to basewsw game module)
   
        - create shadow files for all nicknames of registered players
        - movement types like b1shop's mod (planned: vq3-like, cpm-like, basewsw, racesow)
        - optionally force movement type serverside
        - weapondefs, depending on movement type §
        - let player choose to shoot weak or strong ammo
        - defrag like race-ghost for own, best and server best §
        - protect admins from beeing kicked and callvoted etc...