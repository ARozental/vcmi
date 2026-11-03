/*
 * AsyncRunner.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <tbb/global_control.h>

#include <algorithm>
#include <cstdlib>

VCMI_LIB_NAMESPACE_BEGIN

/// Helper class for running asynchronous tasks using TBB thread pool
class AsyncRunner : boost::noncopyable
{
	tbb::task_arena arena;
	tbb::task_group taskGroup;
	tbb::global_control control;

	static int selectArenaSize()
	{
		// WARNING:
		// Due to blocking waits in AI logic, we require some oversubscription on system with small number of cores
		// othervice, it is possible for AI threads to stuck in blocking wait, while task that will unblock AI never assigned to worker thread
		// TBB provides resumable tasks support, however this support is not available on all systems (most notably - Android)
		// To work around this problem, request TBB to create few extra threads when running on CPU with low max concurrency
		// Issue confirmed to happen (but likely not limited to) on iPhone 12 Pro (2+4 cores) and on virtual systems with 2 cores
		int cores = tbb::this_task_arena::max_concurrency();
		int requiredWorkersCount = 4;
		// alpha_heroes (performance, no behaviour change): VCMI_MAX_WORKERS caps the pool. A headless AI game on a
		// 24-thread machine ran ~23 TBB workers, and Nullkiller2's parallel loops at every AI request kept ~13 of them
		// spinning between loops: ~60% of the game's CPU, in ten games at once about 3 of the machine's 12 cores (2026-11-02)
		if(const char * env = std::getenv("VCMI_MAX_WORKERS"))
			if(const int limit = std::atoi(env); limit > 0)
				cores = std::min(cores, limit);
		return std::max(cores, requiredWorkersCount);
	}

public:
	AsyncRunner()
		:arena(selectArenaSize())
		,control(tbb::global_control::max_allowed_parallelism, selectArenaSize())
	{}

	/// Runs the provided functor asynchronously on a thread from the TBB worker pool.
	template<typename Functor>
	void run(Functor && f)
	{
		arena.enqueue(taskGroup.defer(std::forward<Functor>(f)));
	}

	/// Waits for all previously enqueued task.
	/// Re-entrable - waiting for tasks does not prevent submitting new tasks
	void wait()
	{
		taskGroup.wait();
	}

	~AsyncRunner()
	{
		wait();
	}
};

VCMI_LIB_NAMESPACE_END
