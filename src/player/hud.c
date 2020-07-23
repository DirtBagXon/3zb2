#include "../header/local.h"
#include "../header/bot.h"

/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if(!(ent->svflags & SVF_MONSTER))
	{
	ent->client->showscores = true;
//	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	}

	VectorCopy (level.intermission_origin, ent->s.origin);
	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	// RAFAEL
	ent->client->quadfire_framenum = 0;
	
	// RAFAEL
	ent->client->trap_blew_up = false;
	ent->client->trap_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value && !(ent->svflags & SVF_MONSTER)) 
	{
		if(zigmode->value)
			DeathmatchScoreboardMessage (ent, ent->flagholder);
		else
			DeathmatchScoreboardMessage (ent, NULL);

		gi.unicast (ent, true);
	}

}
void SetLVChanged ( int i );
void DelBots2(int i);
int GetNumbots ( void );
int GetLVChanged ( void );

void BeginIntermission (edict_t *targ)
{
	int		i;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// allready activated

//ZOID
	if (deathmatch->value && ctf->value)
		CTFCalcScores();
//ZOID

//	game.autosaved = false;

	// respawn any dead clients
//	for (i=0 ; i<maxclients->value ; i++)
//	{
//		client = g_edicts + 1 + i;
//		if (!client->inuse)
//			continue;
//		if (client->health <= 0)
//			respawn(client);
//	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	// if on same unit, return immediately
	if (!deathmatch->value && (targ->map && targ->map[0] != '*') )
	{	// go immediately to the next level
		level.exitintermission = 1;
		return;
	}
	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}
}


/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total, rtotal;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char		*tag, *mark;

	// protect bprintf() against SZ_Getspace error
	int		broadcast = 16;
	int		topresult = 6;

//ZOID
	if (ctf->value) {
		CTFScoreboardMessage (ent, killer);
		return;
	}
//ZOID

	// sort the clients by score
	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		score = game.clients[i].resp.score;
		for (j=0 ; j<total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k=total ; k>j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}
	rtotal = total;

	// print level name and exit rules
	string[0] = 0;

	stringlength = strlen(string);

	if(level.intermissiontime && ent == &g_edicts[1])
	{
		if(zigmode->value && zigspawn->value && flagbounce > 0)
		{
			CPRepeat('-', strlen(level.mapname) + 11);
			gi.bprintf(PRINT_HIGH, "| %s | ~ %02d |\n", level.mapname, flagbounce);
		}
		else
		{
			CPRepeat('-', strlen(level.mapname) + 4);
			gi.bprintf(PRINT_HIGH, "| %s |\n", level.mapname);
		}

		if(rtotal <= broadcast) {
			CPRepeat('-', 54);
			gi.bprintf(PRINT_HIGH, "| X | Player%-10s ", " ");
			gi.bprintf(PRINT_HIGH, "|  S  |  P  |  T  |  F  |  A  |\n");
			CPRepeat('-', 54);
		}
		else
		{
			CPRepeat('-', 37);
			gi.bprintf(PRINT_HIGH, "| Will not broadcast to +%d players |\n",
					broadcast);
			CPRepeat('-', 37);
		}
	}

	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i=0 ; i<total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];
		mark = " ";

		x = (i>=6) ? 160 : 0;
		y = 32 + 32 * (i%6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			if (zigmode->value)
				tag = "zigtag";
			else
				tag = "tag2";
		else
			tag = NULL;

		if (zigintro->value && !ENT_IS_BOT(cl_ent) && !cl_ent->client->pers.joined)
			tag = "spectag";

		if(zigmode->value)
			if(killer != NULL && cl_ent == killer)
				tag = "zigtag";

		if (tag)
		{
			Com_sprintf (entry, sizeof(entry),
				"xv %i yv %i picn %s ",x+32, y, tag);
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		if(level.intermissiontime && ent == &g_edicts[1] && rtotal <= broadcast && i < topresult)
		{
			if(tag && strcmp(tag, "zigtag") == 0)
				mark = "F";
			else if (zigintro->value && !cl->pers.joined && !ENT_IS_BOT(cl_ent))
				mark = "S";
			else if(i == 0)
				mark = "*";

			gi.bprintf(PRINT_HIGH, "| %s | %-16s | %-3d | %-3d | %-3d | +%-2d | +%-2d |\n",
				mark, cl_ent->client->pers.netname,
				cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600,
				cl_ent->client->resp.possession, cl_ent->client->resp.assassin);
		}

		// send the layout
		Com_sprintf (entry, sizeof(entry),
			"client %i %i %i %i %i %i ",
			x, y, sorted[i], cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600);
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);

	if(level.intermissiontime && ent == &g_edicts[1] && rtotal <= broadcast)
		CPRepeat('-', 54);
}


/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
	if(zigmode->value)
		DeathmatchScoreboardMessage (ent, ent->flagholder);
	else
		DeathmatchScoreboardMessage (ent, ent->enemy);

	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;

//ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
//ZOID

	if (!deathmatch->value && !coop->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard (ent);
}


/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer (edict_t *ent)
{
	char	string[1024];
	char	*sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// send the layout
	Com_sprintf (string, sizeof(string),
		"xv 32 yv 8 picn help "			// background
		"xv 202 yv 12 string2 \"%s\" "		// skill
		"xv 0 yv 24 cstring2 \"%s\" "		// level name
		"xv 0 yv 54 cstring2 \"%s\" "		// help 1
		"xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ", 
		sk,
		level.level_name,
		game.helpmessage1,
		game.helpmessage2,
		level.killed_monsters, level.total_monsters, 
		level.found_goals, level.total_goals,
		level.found_secrets, level.total_secrets);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->resp.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->resp.helpchanged = 0;
	HelpComputer (ent);
}


/*
==========
Player ID
==========
*/

static qboolean visiblemask(edict_t *self, edict_t *other, int mask)
{
    vec3_t  spot1;
    vec3_t  spot2;
    trace_t trace;
    int     i;

    VectorCopy(self->s.origin, spot1);
    spot1[2] += self->viewheight;

    VectorCopy(other->s.origin, spot2);
    spot2[2] += other->viewheight;

    for (i = 0; i < 10; i++) {
        trace = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, mask);

        if (trace.fraction == 1.0f)
            return true;

        if (trace.allsolid && (trace.contents & MASK_WATER)) {
            mask &= ~MASK_WATER;
            continue;
        }

        if (trace.ent == world && trace.surface &&
            (trace.surface->flags & (SURF_TRANS33 | SURF_TRANS66))) {
            mask &= ~MASK_WATER;
            VectorCopy(trace.endpos, spot1);
            continue;
        }

        break;
    }
    return false;
}


static edict_t *find_by_tracing(edict_t *ent)
{
    edict_t     *ignore;
    vec3_t      forward;
    trace_t     tr;
    vec3_t      start;
    vec3_t      mins = { -4, -4, -4 };
    vec3_t      maxs = { 4, 4, 4 };
    int         i;
    int         tracemask;

    VectorCopy(ent->s.origin, start);
    start[2] += ent->viewheight;

    AngleVectors(ent->client->v_angle, forward, NULL, NULL);

    VectorScale(forward, 4096, forward);
    VectorAdd(ent->s.origin, forward, forward);

    ignore = ent;

    tracemask = CONTENTS_SOLID | CONTENTS_MONSTER | MASK_WATER;

    for (i = 0; i < 10; i++) {
        tr = gi.trace(start, mins, maxs, forward, ignore, tracemask);

        if (tr.allsolid && (tr.contents & MASK_WATER)) {
            tracemask &= ~MASK_WATER;
            continue;
        }

        if (tr.ent == world && tr.surface &&
            (tr.surface->flags & (SURF_TRANS33 | SURF_TRANS66))) {
            tracemask &= ~MASK_WATER;
            VectorCopy(tr.endpos, start);
            continue;
        }

        if (tr.ent == world || tr.fraction == 1.0f)
            break;

        if (tr.ent && tr.ent->client && tr.ent->health > 0 &&
            visiblemask(tr.ent, ent, CONTENTS_SOLID | MASK_WATER)) {
            return tr.ent;
        }

        VectorCopy(tr.endpos, start);
        ignore = tr.ent;
    }

    return NULL;
}

static edict_t *find_by_angles(edict_t *ent)
{
    vec3_t      forward;
    edict_t     *who, *best;
    vec3_t      dir;
    float       distance, bdistance = 0.0f;
    float       bd = 0.0f, d;

    AngleVectors(ent->client->v_angle, forward, NULL, NULL);
    best = NULL;

    for (who = g_edicts + 1; who <= g_edicts + game.maxclients; who++) {
        if (!who->inuse)
            continue;
        if (who->client->pers.connected == false)
            continue;
        if (who->health <= 0)
            continue;

        if (who == ent)
            continue;

        VectorSubtract(who->s.origin, ent->s.origin, dir);
        distance = VectorLength(dir);

        VectorNormalize(dir);
        d = DotProduct(forward, dir);

        if (d > bd &&
            visiblemask(ent, who, CONTENTS_SOLID | MASK_WATER) &&
            visiblemask(who, ent, CONTENTS_SOLID | MASK_WATER)) {
            bdistance = distance;
            bd = d;
            best = who;
        }
    }

    if (!best) {
        return NULL;
    }

    if ((bdistance < 150 && bd > 0.50f) ||
        (bdistance < 250 && bd > 0.90f) ||
        (bdistance < 600 && bd > 0.96f) ||
        bd > 0.98f) {
        return best;
    }

    return NULL;
}

int G_GetPlayerIdView(edict_t *ent)
{
    edict_t *target;

    target = find_by_tracing(ent);
    if (!target) {
        target = find_by_angles(ent);
        if (!target) {
            return 0;
        }
    }

    return CS_PLAYERNAMES + (target - g_edicts) - 1;
}

void G_WriteTime(int remaining)
{
	char buffer[16];
	char message[32];
	char end[4];
	char Highlight[MAX_STRING_CHARS];
	int sec = remaining % 60;
	int min = remaining / 60;
	int i;

	sprintf(message, "[ Time: ");
	sprintf(end, " ]");

	if(remaining < 0)
		sprintf(buffer, " 0:00");
	else
	{
		sprintf(buffer, "%2d:%02d", min, sec);

		if (remaining <= 30 && (sec & 1) == 0) {
			for (i = 0; buffer[i]; i++) {
				buffer[i] |= 128;
			}
		}
	}

	HighlightStr(Highlight, buffer, MAX_STRING_CHARS);
	strcat(message, Highlight);
	strcat(message, end);
	gi.configstring(CS_TIME, message);
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
	int			index, cells;
	int			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	if (!ent->client->ammo_index /* || !ent->client->pers.inventory[ent->client->ammo_index] */)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}
	
	//
	// armor
	//
	power_armor_type = PowerArmorType (ent);
	if (power_armor_type)
	{
		cells = ent->client->pers.inventory[ITEM_INDEX(Fdi_CELLS/*FindItem ("cells")*/)];
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;
		}
	}

	index = ArmorIndex (ent);
	if (power_armor_type && (!index || (level.framenum & 8) ) )
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex ("i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex (index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
	}
	// RAFAEL
	else if (ent->client->quadfire_framenum > level.framenum)
	{
		// note to self
		// need to change imageindex
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quadfire");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quadfire_framenum - level.framenum)/10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		if((level.framenum - ent->client->resp.spawnframe) > SPAWNPROTECT) {
			ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_invulnerability");
			ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
		}
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_envirosuit");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum)/10;
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_rebreather");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum)/10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex (itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	// targeting_id

	if(playerid->value)
		ent->client->ps.stats[STAT_VIEWID] = G_GetPlayerIdView(ent);
	else
		ent->client->ps.stats[STAT_VIEWID] = 0;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores) {
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
			ent->client->ps.stats[STAT_VIEWID] = 0;
			ent->client->ps.stats[STAT_CHASE] = 0;
		}

		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	//
	// rank and time
	//
	if(zigmode->value && combathud->value && (level.framenum&8)
			&& !level.intermissiontime)
	{
		ent->client->ps.stats[STAT_RANK] = ent->client->pers.rank;

		if(timelimit->value > 0)
			ent->client->ps.stats[STAT_TIME] = CS_TIME;
	}

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->resp.helpchanged && (level.framenum&8) )
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex ("i_help");
	else if ( (ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91)
		&& ent->client->pers.weapon)
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
	else
		ent->client->ps.stats[STAT_HELPICON] = 0;

//ponpoko

	// zigmode now hijacks this - can't find an instance of zsight being used...
	if(zigmode->value != 1) {
		if(ent->client->zc.aiming == 1)
		{
			ent->client->ps.stats[STAT_SIGHT_PIC] = gi.imageindex ("zsight");
		}
		else if(ent->client->zc.aiming == 3)
		{
			if(ent->client->zc.lockon) ent->client->ps.stats[STAT_SIGHT_PIC] = gi.imageindex ("zsight_l1");
			else ent->client->ps.stats[STAT_SIGHT_PIC] = gi.imageindex ("zsight_l0");
		}
		else
			ent->client->ps.stats[STAT_SIGHT_PIC] = 0;
	}

//ponpoko

//ZOID
	SetCTFStats(ent);
//ZOID

}


/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;
	cl->ps.stats[STAT_VIEWID] = 0;
	cl->ps.stats[STAT_RANK] = 0;

	if(combathud->value)
		cl->ps.stats[STAT_TIME] = CS_TIME;
	else
		cl->ps.stats[STAT_TIME] = 0;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERNAMES +
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = CS_OBSERVE;
}

/*
======================================================================

Get Player Ranking in HUD

======================================================================
*/

int G_GetRank(edict_t *ent)
{
	edict_t	*cl_ent;
	int	i,j,k;
	int	score, total;
	int	sorted[MAX_CLIENTS];
	int	sortedscores[MAX_CLIENTS];

	total = 0;
        for (i=0 ; i<game.maxclients ; i++)
        {
                cl_ent = g_edicts + 1 + i;

                if (!cl_ent->inuse)
                        continue;

                score = game.clients[i].resp.score;

                for (j=0 ; j<total ; j++)
                {
                        if (score > sortedscores[j])
                                break;
                }

                for (k=total ; k>j ; k--)
                {
                        sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
                }

                sorted[j] = i;
		sortedscores[j] = score;
		total++;
        }

	for (i=0 ; i<total ; i++)
	{
		cl_ent = g_edicts + 1 + sorted[i];

		if(cl_ent == ent)
			return i + 1;
	}
	return 0;
}


/*
=====================================================

Push the latest ranking

=====================================================
*/
void G_SendRanks (void)
{
	edict_t         *cl_ent;
	int             i;

	if(combathud->value)
	{
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if(cl_ent->client && !ENT_IS_BOT(cl_ent))
				cl_ent->client->ps.stats[STAT_RANK] =  G_GetRank(cl_ent);
		}
	}
}

/*
=====================================================

Killer Flag messages

=====================================================
*/
void Flag_Msg(char *response, size_t length)
{
	char pants[length];
	int x = rand() % 4;
	int i = 0;

	switch(x)
	{
		case 0:
			strncpy(pants, "with a vampirical tendency", length);
			break;
		case 1:
			strncpy(pants, "that's slaying stamina", length);
			break;
		case 2:
			strncpy(pants, "while slaughtering health", length);
			break;
		case 3:
			strncpy(pants, "drawing their life blood", length);
			break;
	}

	while (length-- > 0) {
		*response++ = pants[i];
		i++;
	}
	*response = '\0';
}
