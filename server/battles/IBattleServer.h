/*
 * IBattleServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/Enumerations.h"
#include "../../lib/battle/BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN
class BattleAction;
class BattleID;
class CBattleInfoCallback;
class CStack;
class IBattleRandomizer;
class IGameInfoCallback;
class ServerCallback;
class SpellCastEnvironment;
struct CPackForClient;
namespace vstd
{
class RNG;
}
VCMI_LIB_NAMESPACE_END

/// What the battle rules (BattleActionProcessor, BattleFlowProcessor) need from the game: CGameHandler provides
/// it in a real game. Another implementation can run the same rules on a copy of a battle (e.g. for search),
/// applying the packs to that copy, with its own random generators and without clients.
class IBattleServer
{
public:
	virtual ~IBattleServer() = default;

	/// Applies a pack to the game state and sends it to the clients
	virtual void sendAndApply(CPackForClient & pack) = 0;
	/// Reports an invalid request; returns true
	virtual bool complain(const std::string & problem) = 0;

	virtual ServerCallback * spellcastEnvironment() const = 0;
	/// Environment for spells cast by obstacles (quicksand, land mines, ...)
	virtual SpellCastEnvironment & obstacleSpellEnvironment() = 0;

	virtual vstd::RNG & getRandomGenerator() = 0;
	/// Luck, morale and creature ability rolls
	virtual IBattleRandomizer & battleRandomizer() = 0;

	virtual IGameInfoCallback & gameInfo() = 0;
	virtual void giveResource(PlayerColor player, GameResID which, int val) = 0;
	virtual void useChargeBasedSpell(const ObjectInstanceID & heroObjectID, const SpellID & spellID) = 0;

	/// Turn timer hooks
	virtual void onBattleStart(const BattleID & battleID) = 0;
	virtual void onBattleNextStack(const BattleID & battleID, const CStack & stack) = 0;
};

/// What the battle rules call back on the battle they belong to (BattleProcessor in a real game)
class IBattleOwner
{
public:
	virtual ~IBattleOwner() = default;

	virtual void updateGateState(const CBattleInfoCallback & battle) = 0;
	/// Ends the battle if it is decided; returns true if it ended
	virtual bool checkBattleStateChanges(const CBattleInfoCallback & battle) = 0;
	virtual bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba) = 0;
	virtual void setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, BattleSide victoriusSide) = 0;
};
