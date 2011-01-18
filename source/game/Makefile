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
#
# Revised by Raz0r on Jan 18, 2011
# 	Inline assembler will use intel syntax
#	Forcing a 32bit binary. Ignoring the -march switch in favour of -m32
#

# Compiler - GCC
CC = gcc
TARGET =  jampgamei386.so

# Compiler Flags - swap CONFIG for release/debug builds
#CFLAGS += -O3 -DNDEBUG -DFINAL_BUILD
CFLAGS += -g3 -D_DEBUG
CFLAGS += -I./
CFLAGS += -D_JK2 -DQAGAME
CFLAGS += -fPIC -Wall -masm=intel -m32# -fno-strict-aliasing -march=i586

OFILES += ai_aotc.o ai_hybrid.o ai_main.o ai_tab.o ai_util.o ai_wpnav.o
OFILES += aotctc_holocron.o
OFILES += bg_g2_utils.o bg_misc.o bg_panimate.o bg_pmove.o bg_saber.o bg_saberLoad.o bg_saga.o bg_slidemove.o bg_vehicleLoad.o bg_weapons.o
OFILES += g_autosave.o g_active.o g_arenas.o g_bot.o g_breakable.o g_camera.o g_client.o g_cmds.o g_crash.o g_combat.o g_dynmusic.o g_exphysics.o g_ICARUScb.o g_items.o \
			g_log.o g_main.o g_mem.o g_misc.o g_missile.o g_mover.o g_nav.o g_navnew.o g_object.o g_roff.o g_saberbeh.o g_saga.o g_session.o g_spawn.o g_strap.o g_svcmds.o \
			g_syscalls.o g_target.o g_team.o g_timer.o g_trigger.o g_turret.o g_turret_G2.o g_utils.o g_vehicleTurret.o g_vehicles.o g_weapon.o
OFILES += NPC.o AnimalNPC.o FighterNPC.o SpeederNPC.o WalkerNPC.o
OFILES += NPC_AI_AssassinDroid.o NPC_AI_Atst.o NPC_AI_Civilian.o NPC_AI_Default.o NPC_AI_Droid.o NPC_AI_GalakMech.o NPC_AI_Grenadier.o NPC_AI_Howler.o NPC_AI_ImperialProbe.o \
			NPC_AI_Interrogator.o NPC_AI_Jedi.o NPC_AI_Mark1.o NPC_AI_Mark2.o NPC_AI_MineMonster.o NPC_AI_Rancor.o NPC_AI_Remote.o NPC_AI_RocketTrooper.o NPC_AI_SaberDroid.o \
			NPC_AI_Sand_creature.o NPC_AI_Seeker.o NPC_AI_Sentry.o NPC_AI_Sniper.o NPC_AI_Stormtrooper.o NPC_AI_Utils.o NPC_AI_Wampa.o
OFILES += NPC_behavior.o NPC_combat.o NPC_goal.o NPC_misc.o NPC_move.o NPC_reactions.o NPC_senses.o NPC_sounds.o NPC_spawn.o NPC_stats.o NPC_utils.o
OFILES += q_math.o q_shared.o
OFILES += tri_coll_test.o
OFILES += w_force.o w_saber.o


$(TARGET): $(OFILES)
	$(CC) -shared -o $(TARGET) $(OFILES) -lm

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm -f $(OFILES)
	rm -f $(TARGET)
