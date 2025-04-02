#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
using namespace std::literals;

class BusTracker
{
public:
	BusTracker()
	{
		initializeStopIds();

		// TODO: Extract this into its own function
		/*for (auto& route : fetchJson("https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/routes"))
		{
			int routeId = route["id"].get<int>();
			routeIds.push_back(routeId);
			routeNames[routeId] = route["shortName"].get<std::string_view>();
		}*/

		initializeArrivalTimeRequestSessions();

		for (const nlohmann::json& stopTimes : getStopArrivalTimes(stopIds))
		{
			std::cout << std::setw(4) << stopTimes << '\n';
		}

		queueStopsByTime();
	}

private:
	std::vector<int> stopIds;
	//std::string_view stopNames[380];
	//std::vector<int> routeIds;
	//std::string_view routeNames[70];
	std::unordered_map<int, std::shared_ptr<cpr::Session>> timeRequestSessions;
	std::priority_queue<std::pair<unsigned int, std::pair<int, int>>, std::vector<std::pair<unsigned int, std::pair<int, int>>>, std::greater<>> stopQueue;

	void initializeStopIds()
	{
		for (const auto& stop : fetchJson("https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops"))
		{
			const int& stopId = stop["id"].get<int>();
			stopIds.emplace_back(stopId);
			//stopNames[stopId] = stop["name"].get<std::string_view>(); // string_view loses value out of scope? .dump() is also an option
		}
	}

	void setArrivalTimeRequestSession(const int stopId, const std::shared_ptr<cpr::Session>& session)
	{
		timeRequestSessions.try_emplace(stopId, session);
	}

	std::shared_ptr<cpr::Session>& getArrivalTimeRequestSession(const int stopId)
	{
		return timeRequestSessions.at(stopId);
	}

	static auto getArrivalTimeRequestSessionUrl(const int stopId)
	{
		return "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" +
		       std::to_string(stopId) + "/times";
	}

	auto createArrivalTimeRequestSession(const int stopId)
	{
		auto session = std::make_shared<cpr::Session>();
		session->SetUrl(cpr::Url{getArrivalTimeRequestSessionUrl(stopId)});
		return session;
	}

	void initializeArrivalTimeRequestSessions()
	{
		for (const int& stopId : stopIds)
		{
			setArrivalTimeRequestSession(stopId, createArrivalTimeRequestSession(stopId));
		}
	}

	void validateResponse(auto& response)
	{
		while (response.text.empty() || response.status_code != 200)
		{
			std::cout << "Retrying " << response.url.str() << '\n';
			response = Get(response.url);
		}
	}

	nlohmann::json fetchJson(const std::string_view& url)
	{
		auto response = Get(cpr::Url{url});
		validateResponse(response);

		return nlohmann::json::parse(response.text);
	}

	std::vector<nlohmann::json> getStopArrivalTimes(const std::vector<int>& stopIds)
	{
		std::vector<nlohmann::json> stopArrivalTimes;

		cpr::MultiPerform timeRequests;
		for (const auto stopId : stopIds)
		{
			timeRequests.AddSession(getArrivalTimeRequestSession(stopId));
		}

		for (auto& response : timeRequests.Get())
		{
			validateResponse(response);
			stopArrivalTimes.emplace_back(nlohmann::json::parse(response.text));
		}

		return stopArrivalTimes;
	}

	unsigned int convertIsoToUnixTimestamp(const std::string& isoTimestamp)
	{
		std::istringstream timestamp(isoTimestamp);
		std::tm unixTimestamp;

		timestamp >> std::get_time(&unixTimestamp, "%Y-%m-%dT%H:%M:%SZ");

		return std::mktime(&unixTimestamp);
	}

	// The function takes advantage of the fact that the responses are ordered in the same way as the stopIds. Create a test for this if you have to.
	// after this I bet there will be an updateQueue function then this should be prefixed w create or init
	// It's an interesting idea to use i instead of the stopId
	void queueStopsByTime()
	{
		auto stopArrivalTimes = getStopArrivalTimes(stopIds);

		for (int i = 0; i < 380; i++)
		{
			if (stopArrivalTimes[i].empty()) continue;

			for (const nlohmann::json& arrivalInfo : stopArrivalTimes[i])
			{
				stopQueue.emplace(convertIsoToUnixTimestamp(arrivalInfo["times"][0]["scheduledDeparture"].dump()), std::make_pair(stopIds[i], arrivalInfo["route"]["routeId"].get<int>()));
			}
		}
	}
};

int main()
{
	BusTracker busTracker;
}