/*
 * TestQueue.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TestQueue.h"

#include "../lib/CConfigHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/json/JsonNode.h"
#include "../lib/json/JsonUtils.h"

#ifndef VCMI_WINDOWS
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

extern char ** environ;
#endif

TestQueue::TestQueue(const std::string & socketPath)
{
#ifndef VCMI_WINDOWS
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	if(socketPath.size() >= sizeof(address.sun_path))
		throw std::runtime_error("VCMI_TEST_QUEUE path too long: " + socketPath);
	std::copy(socketPath.begin(), socketPath.end(), address.sun_path);
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0 || connect(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0)
		throw std::runtime_error("cannot connect to VCMI_TEST_QUEUE " + socketPath);
#else
	throw std::runtime_error("VCMI_TEST_QUEUE is not supported on Windows");
#endif
}

TestQueue * TestQueue::get()
{
	static std::unique_ptr<TestQueue> queue = []() -> std::unique_ptr<TestQueue>
	{
		const char * path = std::getenv("VCMI_TEST_QUEUE");
		if(!path || !*path)
			return nullptr;
		return std::unique_ptr<TestQueue>(new TestQueue(path));
	}();
	return queue.get();
}

std::optional<std::string> TestQueue::next()
{
#ifndef VCMI_WINDOWS
	const std::string ready = "{\"ready\": true}\n";
	if(::send(fd, ready.data(), ready.size(), 0) != static_cast<ssize_t>(ready.size()))
		return std::nullopt;
	std::string line;
	char c = 0;
	while(::recv(fd, &c, 1, 0) == 1 && c != '\n')
		line.push_back(c);
	if(line.empty())
		return std::nullopt;
	const JsonNode game(reinterpret_cast<const std::byte *>(line.data()), line.size(), "VCMI_TEST_QUEUE");
	if(!game["map"].isString())
		return std::nullopt;

	std::vector<std::string> unset;
	for(char ** variable = environ; *variable; ++variable)
	{
		const std::string entry(*variable);
		const auto name = entry.substr(0, entry.find('='));
		for(const auto & prefix : game["clear_env"].Vector())
			if(boost::algorithm::starts_with(name, prefix.String()) && game["env"][name].isNull())
				unset.push_back(name);
	}
	for(const auto & name : unset)
		unsetenv(name.c_str());
	for(const auto & [name, value] : game["env"].Struct())
		setenv(name.c_str(), value.String().c_str(), 1);

	for(const auto & [key, value] : game["settings"].Struct())
	{
		Settings node = settings.write[key];
		JsonNode source = value;
		JsonUtils::merge(*node.operator->(), source);
	}
	// The map was written after this process started: let the filesystem see it
	CResourceHandler::get()->updateFilteredFiles([](const std::string & mount) { return boost::algorithm::starts_with(mount, "MAPS/"); });

	std::scoped_lock lock(mutex);
	over = false;
	return game["map"].String();
#else
	return std::nullopt;
#endif
}

void TestQueue::gameOver()
{
	{
		std::scoped_lock lock(mutex);
		over = true;
	}
	changed.notify_all();
}

void TestQueue::waitForGameOver()
{
	std::unique_lock lock(mutex);
	changed.wait(lock, [this] { return over; });
}
