/*
 * TestQueue.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

/// Many test-map games in one headless client (alpha_heroes; not a bug fix). With the environment variable
/// VCMI_TEST_QUEUE (a Unix socket) the client asks it for each game instead of playing --testmap once, so the process,
/// its libraries and the game data are loaded once for thousands of games (each start cost ~0.6 s of CPU).
///
/// The client sends a line {"ready": true} and gets the next game as one line of JSON:
///   {"map": "Maps/...", "env": {"NAME": "value", ...}, "clear_env": ["PREFIX_", ...], "settings": {...}}
/// env: environment variables to set first (those starting with a clear_env prefix and not given are unset), so each
/// game's AIs read their own; settings: merged into the in-memory settings (e.g. the server's random seed). A line
/// without "map" (e.g. {}) ends the process.
class TestQueue
{
	int fd = -1;
	std::mutex mutex;
	std::condition_variable changed;
	bool over = false;

	explicit TestQueue(const std::string & socketPath);

public:
	/// The queue of this process, or nullptr without VCMI_TEST_QUEUE
	static TestQueue * get();

	/// The next game's map, its environment and settings applied; nullopt when there are no more games
	std::optional<std::string> next();
	/// The game ended (called by the network thread when a player's game ends the test)
	void gameOver();
	/// Blocks until gameOver()
	void waitForGameOver();
};
