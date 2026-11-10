/*
* Nullkiller.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "PriorityEvaluator.h"
#include "FuzzyHelper.h"
#include "Settings.h"
#include "AIMemory.h"
#include "DeepDecomposer.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Analyzers/BuildAnalyzer.h"
#include "../Analyzers/ArmyManager.h"
#include "../Analyzers/HeroManager.h"
#include "../Analyzers/ObjectClusterizer.h"
#include "../Helpers/ArmyFormation.h"

#include "../../../lib/ConditionalWait.h"

// (fork, alpha_heroes) Nullkiller::keepPlan, lastPlan, Composition::getSubtasks: labels for AlphaMapAI's advice (its
// Advice.cpp builds on them only where this is defined: with an upstream Nullkiller2 it records no plan)
#define NK2_KEEPS_PLAN 1
// (fork, alpha_heroes) Nullkiller::adviceFixes, exchangeSettled: the advising Nullkiller2's meeting fixes (Advice.cpp)
#define NK2_ADVICE_FIXES 1

VCMI_LIB_NAMESPACE_BEGIN

class PathfinderCache;

VCMI_LIB_NAMESPACE_END

namespace NK2AI
{

const float MIN_PRIORITY = 0.01f;
const float SMALL_SCAN_MIN_PRIORITY = 0.4f;

enum class HeroLockedReason
{
	NOT_LOCKED = 0,

	STARTUP = 1,

	DEFENCE = 2,

	HERO_CHAIN = 3
};

enum class ScanDepth
{
	MAIN_FULL = 0,

	SMALL = 1,

	ALL_FULL = 2
};

struct TaskPlanItem
{
	std::vector<ObjectInstanceID> affectedObjects;
	Goals::TSubgoal task;

	TaskPlanItem(const Goals::TSubgoal& goal);
};

class TaskPlan
{
private:
	std::vector<TaskPlanItem> tasks;

public:
	Goals::TTaskVec getTasks() const;
	void mergeAndFilter(const Goals::TSubgoal& task);
};

class Nullkiller
{
private:
	const CGHeroInstance * activeHero;
	int3 targetTile;
	ObjectInstanceID targetObject;
	std::map<const CGHeroInstance *, HeroLockedReason> lockedHeroes;
	std::unique_ptr<PathfinderCache> pathfinderCache;
	ScanDepth scanDepth;
	TResources lockedResources;
	bool useHeroChain;
	bool openMap;
	bool useObjectGraph;
	bool pathfinderInvalidated;

public:
	static std::unique_ptr<ObjectGraph> baseGraph;

	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;
	std::unique_ptr<BuildAnalyzer> buildAnalyzer;
	std::unique_ptr<ObjectClusterizer> objectClusterizer;
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	std::unique_ptr<SharedPool<PriorityEvaluator>> priorityEvaluators;
	std::unique_ptr<AIPathfinder> pathfinder;
	std::atomic_int32_t pathfinderTurnStorageMisses;
	std::unique_ptr<HeroManager> heroManager;
	std::unique_ptr<ArmyManager> armyManager;
	std::unique_ptr<AIMemory> memory;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
	std::unique_ptr<DeepDecomposer> decomposer;
	std::unique_ptr<ArmyFormation> armyFormation;
	std::unique_ptr<Settings> settings;
	/// Same value as AIGateway->playerID
	PlayerColor playerID;
	AIGateway * aiGw;
	/// Same value as AIGateway->cc
	std::shared_ptr<CCallback> cc;
	mutable std::mutex aiStateMutex; // also held while planning (decompose, buildPlanAndFilter, choseBestTask): they read the analyzers
	mutable ThreadInterruption makingTurnInterruption;
	// (fork, alpha_heroes: labels for AlphaMapAI's advice, no behaviour change) with keepPlan, a turn's first pass plans
	// right after its state update, before any task runs, keeps that plan (lastPlan, best first; lastPlanValid) and
	// executes it: the plan stands whatever request the advice then takes, the priority pass's included
	bool keepPlan = false;
	bool lastPlanValid = false;
	int lastPlanTier = 0;
	Goals::TTaskVec lastPlan;
	// (fork, alpha_heroes: AlphaMapAI's advising Nullkiller2 only, no behaviour change unset) adviceFixes: no delivery
	// of an army that holds the receiver's own (GatherArmyBehavior::deliverArmyToHero). exchangeSettled, when set: two
	// heroes with nothing left to exchange (their armies as their last exchange ended), no delivery or chain exchange
	// between them planned
	bool adviceFixes = false;
	std::function<bool(const CGHeroInstance *, const CGHeroInstance *)> exchangeSettled;

	Nullkiller();
	virtual ~Nullkiller();
	void init(const std::shared_ptr<CCallback> & cbInput, AIGateway * aiGwInput);
	virtual void makeTurn();
	bool updateStateAndExecutePriorityPass(Goals::TGoalVec& tempResults, int passIndex);
	bool isActive(const CGHeroInstance * hero) const { return activeHero == hero; }
	bool isHeroLocked(const CGHeroInstance * hero) const;
	HeroPtr getActiveHero() { return HeroPtr(activeHero, cc.get()); }
	HeroLockedReason getHeroLockedReason(const CGHeroInstance * hero) const;
	int3 getTargetTile() const { return targetTile; }
	ObjectInstanceID getTargetObject() const { return targetObject; }
	void setTargetObject(int objid) { targetObject = ObjectInstanceID(objid); }
	void setActive(const CGHeroInstance * hero, int3 tile) { activeHero = hero; targetTile = tile; }
	void lockHero(const CGHeroInstance * hero, HeroLockedReason lockReason) { lockedHeroes[hero] = lockReason; }
	void unlockHero(const CGHeroInstance * hero) { lockedHeroes.erase(hero); }
	bool arePathHeroesLocked(const AIPath & path) const;
	TResources getFreeResources() const;
	int32_t getFreeGold() const { return getFreeResources()[EGameResID::GOLD]; }
	void lockResources(const TResources & res);
	const TResources & getLockedResources() const { return lockedResources; }
	ScanDepth getScanDepth() const { return scanDepth; }
	bool isOpenMap() const { return openMap; }
	bool isObjectGraphAllowed() const { return useObjectGraph; }
	void invalidatePathfinderData();
	std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h) const;
	void invalidatePaths();
	std::map<const CGHeroInstance *, HeroRole> getHeroesForPathfinding() const;

private:
	void resetState();
	void updateState();
	void decompose(Goals::TGoalVec & results, const Goals::TSubgoal& behavior, int decompositionMaxDepth) const;
	Goals::TTask choseBestTask(Goals::TGoalVec & tasks) const;
	Goals::TTaskVec buildPlanAndFilter(Goals::TGoalVec & tasks, int priorityTier) const;
	Goals::TTaskVec planPass(Goals::TGoalVec & tasks, int & prioOfTask) const;
	bool executeTask(const Goals::TTask & task) const;
	bool areAffectedObjectsPresent(const Goals::TTask & task) const;
	HeroRole getTaskRole(const Goals::TTask & task) const;
	void tracePlayerStatus(bool beginning) const;
};

}
