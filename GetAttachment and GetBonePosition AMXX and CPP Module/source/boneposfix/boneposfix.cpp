#include "sdk/amxxmodule.h"
#include <studio.h>
#include <cbase.h>

/*
Comment out next line if:
- The game is not cstrike, or if
- The game is cstrike AND you have a fixed game dll - IE ReGameDLL_CS build with REGAMEDLL_FIXES

This "fixes" a bug in CS's StudioSetupBones.
*/
#define FIXCSANGLES
/*
Comment out next line if you live in a wonderland where "The Stupid Quake Bug" doesn't exist.

This "fixes" a bug in GetBonePosition
*/
#define FIXQUAKESWAP
/*
Comment out next line if you don't want automatic frame advancing.

This implements frame advancing behaviour for non-animating entites - IE cycler_sprite, info_null with animating model.
*/
#define FIXSVFRAME

inline void FrameAdvance(edict_t * pEnt) {
	float flInterval = gpGlobals->time - pEnt->v.animtime;
	if(flInterval <= 0.001)
		return;

	studiohdr_t * pStudioHDR = (studiohdr_t*)GET_MODEL_PTR(pEnt);
	if(!pStudioHDR)
		return;
	if(pEnt->v.sequence >= pStudioHDR->numseq || pEnt->v.sequence < 0)
		return;
	mstudioseqdesc_t * pSeqDesc = (mstudioseqdesc_t *)((byte *)pStudioHDR + pStudioHDR->seqindex) + (int)pEnt->v.sequence;
	float flFrameRate = 256.0;
	if(pSeqDesc->numframes > 1)
		flFrameRate = 256 * pSeqDesc->fps / (pSeqDesc->numframes - 1);
	pEnt->v.frame += flInterval * flFrameRate * pEnt->v.framerate;

	if(pEnt->v.frame < 0 || pEnt->v.frame >= 256) {
		if(pSeqDesc->flags & STUDIO_LOOPING)
			pEnt->v.frame -= (int)(pEnt->v.frame / 256.0) * 256.0;
		else
			pEnt->v.frame = (pEnt->v.frame < 0.0) ? 0 : 255;
	}
	pEnt->v.animtime = gpGlobals->time;
}

inline void FixCSAngles(const edict_t * pEnt, Vector& vecOrigin) {
	int iEnt = ENTINDEX(pEnt);
	if(iEnt >= 1 && iEnt <= gpGlobals->maxClients)
		return;
	Vector locOrigin = vecOrigin = vecOrigin - pEnt->v.origin;
	float cos = cosf(pEnt->v.angles[1] * (M_PI / 180));
	float sin = sinf(pEnt->v.angles[1] * (M_PI / 180));
	vecOrigin[0] = locOrigin[0] * cos - locOrigin[1] * sin;
	vecOrigin[1] = locOrigin[1] * cos + locOrigin[0] * sin;
	vecOrigin = vecOrigin + pEnt->v.origin;
}

static cell AMX_NATIVE_CALL GetBonePosition(AMX *amx, cell *params) {
	edict_t * pEnt = INDEXENT(params[1]);
	if(!pEnt)
		return false;
	int iBone = params[2];
	Vector vecOrigin;
	cell * cPrtOrigin = MF_GetAmxAddr(amx, params[3]);
	if(!cPrtOrigin)
		return false;

#ifdef FIXSVFRAME
	FrameAdvance(pEnt);
#endif
#ifdef FIXQUAKESWAP
	pEnt->v.angles[0] = -pEnt->v.angles[0];
#endif

	GET_BONE_POSITION(pEnt, iBone, vecOrigin, NULL);

#ifdef FIXQUAKESWAP
	pEnt->v.angles[0] = -pEnt->v.angles[0];
#endif
#ifdef FIXCSANGLES
	FixCSAngles(pEnt, vecOrigin);
#endif

	cPrtOrigin[0] = amx_ftoc(vecOrigin[0]);
	cPrtOrigin[1] = amx_ftoc(vecOrigin[1]);
	cPrtOrigin[2] = amx_ftoc(vecOrigin[2]);
	return true;
}

static cell AMX_NATIVE_CALL GetAttachment(AMX *amx, cell *params) {
	edict_t * pEnt = INDEXENT(params[1]);
	if(!pEnt)
		return false;
	int iAttachment = params[2];
	Vector vecOrigin;
	cell * cPrtOrigin = MF_GetAmxAddr(amx, params[3]);
	if(!cPrtOrigin)
		return false;

#ifdef FIXSVFRAME
	FrameAdvance(pEnt);
#endif

	GET_ATTACHMENT(pEnt, iAttachment, vecOrigin, NULL);

#ifdef FIXCSANGLES
	FixCSAngles(pEnt, vecOrigin);
#endif

	cPrtOrigin[0] = amx_ftoc(vecOrigin[0]);
	cPrtOrigin[1] = amx_ftoc(vecOrigin[1]);
	cPrtOrigin[2] = amx_ftoc(vecOrigin[2]);
	return true;
}

AMX_NATIVE_INFO amxxfunctions[] = {
	{"GetBonePosition", GetBonePosition},
	{"GetAttachment", GetAttachment},
	{NULL, NULL}
};

void OnAmxxAttach() {
	MF_AddNatives(amxxfunctions);
	RETURN_META(MRES_IGNORED);
}