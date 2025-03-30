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
		constexpr auto stopsApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops"sv;
		//constexpr auto routesApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/routes"sv;

		for (const auto& stop : fetchJson(stopsApiUrl))
		{
			const int& stopId = stop["id"].get<int>();
			stopIds.push_back(stopId);
			//stopNames[stopId] = stop["name"].get<std::string_view>(); // string_view loses value out of scope? .dump() is also an option
		}

		/*for (auto& route : fetchJson(routesApiUrl))
		{
			const int& routeId = route["id"].get<int>();
			routeIds.push_back(routeId);
			routeNames[routeId] = route["shortName"].get<std::string_view>();
		}*/

		timesPerStop = fetchStopTimes(stopIds);
		initializeArrivalTimeRequestSessions();

		for (const nlohmann::json& stopTimes : timesPerStop)
		{
			std::cout << std::setw(4) << stopTimes << '\n';
		}

		queueStopsByTime();
	}

private:
	std::vector<int> stopIds;
	std::vector<nlohmann::json> timesPerStop;
	//std::string_view stopNames[380];
	//std::vector<int> routeIds;
	//std::string_view routeNames[70];
	std::unordered_map<int, std::shared_ptr<cpr::Session>> timeRequestSessions;
	std::priority_queue<std::pair<unsigned int, std::pair<int, int>>, std::vector<std::pair<unsigned int, std::pair<int, int>>>, std::greater<>> stopQueue;

	void addTimeRequestSession(const int stopId, const std::shared_ptr<cpr::Session>& session)
	{
		timeRequestSessions[stopId] = session;
	}

	std::shared_ptr<cpr::Session>& getTimeRequestSession(const int stopId)
	{
		return timeRequestSessions.at(stopId);
	}

	{
		for (const int stopId : stopIds)
	void initializeArrivalTimeRequestSessions()
		{
			auto session = std::make_shared<cpr::Session>();
			session->SetUrl(cpr::Url{"https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" +
			std::to_string(stopId) + "/times"});
			addTimeRequestSession(stopId, session);
		}
	}

	static void validateResponse(cpr::Response& response)
	{
		while (response.status_code != 200 || response.text.empty())
		{
			std::cout << "Retrying " << response.url.c_str() << '\n';
			response = Get(response.url);
		}
	}

	static nlohmann::json fetchJson(const std::string_view& url)
	{
		cpr::Response response = Get(cpr::Url{url});
		std::cout << url << '\n';
		validateResponse(response);

		return nlohmann::json::parse(response.text);
	}

	std::vector<cpr::Response> requestTimesOfStops(const std::vector<int>& stopIds)
	{
		cpr::MultiPerform timeRequests;

		for (const auto stopId : stopIds)
		{
			timeRequests.AddSession(getTimeRequestSession(stopId));
		}

		return timeRequests.Get();
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

			for (auto& arrivalInfo : stopArrivalTimes[i])
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