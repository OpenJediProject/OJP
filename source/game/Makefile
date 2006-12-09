# Linux Makefile for Jedi Academy MP SDK
# By Patrick Hemmer
# Version 2
# 
# Created Nov 29, 2003
# The Void - http://www.divoid.net
#
# You may set your own optimizations on the CFLAGS line. If you dont know what optimizations are, then just leave it as is.
# Run 'make' to compile the code, once done, you will have a file called 'jampgamei386.so' in the 'game' directory. Copy this file to the 'base' folder of your server or to your mod folder.
# If for some strange reason, you are running on less than a 586 processor, change the i586 to i386 on the CFLAGS line.

CC = gcc
CFLAGS = -fPIC -DFINAL_BUILD -DNDEBUG -DQAGAME -D_JK2 -D__linux__ -march=i586 -O3 
TARGET = jampgamei386.so

OFILES = g_crash.o g_saberbeh.o NPC_AI_Civilian.o g_autosave.o NPC_AI_SaberDroid.o NPC_AI_RocketTrooper.o ai_aotc.o ai_hybrid.o g_dynmusic.o g_roff.o AnimalNPC.o FighterNPC.o NPC.o NPC_AI_Atst.o NPC_AI_Default.o NPC_AI_Droid.o NPC_AI_GalakMech.o NPC_AI_Grenadier.o NPC_AI_Howler.o NPC_AI_ImperialProbe.o NPC_AI_Interrogator.o NPC_AI_Jedi.o NPC_AI_Mark1.o NPC_AI_Mark2.o NPC_AI_MineMonster.o NPC_AI_Rancor.o NPC_AI_Remote.o NPC_AI_Seeker.o NPC_AI_Sentry.o NPC_AI_Sniper.o NPC_AI_Stormtrooper.o NPC_AI_Utils.o NPC_AI_Wampa.o NPC_behavior.o NPC_combat.o NPC_goal.o NPC_misc.o NPC_move.o NPC_reactions.o NPC_senses.o NPC_sounds.o NPC_spawn.o NPC_stats.o NPC_utils.o SpeederNPC.o WalkerNPC.o ai_main.o ai_util.o ai_wpnav.o bg_g2_utils.o bg_misc.o bg_panimate.o bg_pmove.o bg_saber.o bg_saberLoad.o bg_saga.o bg_slidemove.o bg_vehicleLoad.o bg_weapons.o g_ICARUScb.o g_active.o g_arenas.o g_bot.o g_client.o g_cmds.o g_combat.o g_exphysics.o g_items.o g_log.o g_main.o g_mem.o g_misc.o g_missile.o g_mover.o g_nav.o g_navnew.o g_object.o g_saga.o g_session.o g_spawn.o g_strap.o g_svcmds.o g_syscalls.o g_target.o g_team.o g_timer.o g_trigger.o g_turret.o g_turret_G2.o g_utils.o g_vehicleTurret.o g_vehicles.o g_weapon.o q_math.o q_shared.o tri_coll_test.o w_force.o w_saber.o aotctc_holocron.o g_breakable.o ai_tab.o g_camera.o NPC_AI_AssassinDroid.o NPC_AI_Sand_creature.o

$(TARGET): $(OFILES)
	$(CC) -shared -o $(TARGET) $(OFILES) -lm

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm -f $(OFILES)
	rm -f $(TARGET)
