#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
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
		queueStopsByTime();
	}

private:
	std::vector<int> stopIds;
	//std::string_view stopNames[380];
	//std::vector<int> routeIds;
	//std::string_view routeNames[70];
	std::vector<std::shared_ptr<cpr::Session>> timeRequestSessions;

	struct stopDeparture
	{
		unsigned int timeUntilBusDeparture;
		int stopId;
		int routeId;

		// For use in the priority queue
		bool operator<(const stopDeparture& rhs) const
		{
			return timeUntilBusDeparture > rhs.timeUntilBusDeparture;
		}
	};
	std::priority_queue<stopDeparture> departureQueue;

	void initializeStopIds()
	{
		const auto& stops = fetchJson("https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops");

		stopIds.reserve(stops.size());
		for (const nlohmann::json& stop : stops)
		{
			const int stopId = stop["id"].get<int>();
			stopIds.push_back(stopId);
			//stopNames[stopId] = stop["name"].get<std::string_view>(); // string_view loses value out of scope? .dump() is also an option. What if we used stopIds' last element inside the [] instead of stopId although that does sound a bit wasteful
		}
	}

	static std::string createArrivalTimeRequestSessionUrl(const int stopId)
	{
		return "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" +
		       std::to_string(stopId) + "/times";
	}

	static auto createArrivalTimeRequestSession(const int stopId)
	{
		auto session = std::make_shared<cpr::Session>();
		session->SetUrl(cpr::Url{createArrivalTimeRequestSessionUrl(stopId)});
		return session;
	}

	void initializeArrivalTimeRequestSessions()
	{
		timeRequestSessions.resize(*std::ranges::max_element(stopIds) + 1); // so we can map all ids to their sessions. An actual map in this case is more expensive than a vector.
		for (const int stopId : stopIds)
		{
			timeRequestSessions[stopId] = createArrivalTimeRequestSession(stopId);
		}
	}

	std::shared_ptr<cpr::Session>& getArrivalTimeRequestSession(const int stopId)
	{
		return timeRequestSessions[stopId];
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

	std::vector<nlohmann::json> fetchStopArrivalTimes(const std::vector<int>& stopIds)
	{
		std::vector<nlohmann::json> stopArrivalTimes;

		cpr::MultiPerform timeRequests;
		for (const int stopId : stopIds)
		{
			timeRequests.AddSession(getArrivalTimeRequestSession(stopId));
		}

		for (auto& response : timeRequests.Get())
		{
			validateResponse(response);
			stopArrivalTimes.push_back(nlohmann::json::parse(response.text));
		}

		return stopArrivalTimes;
	}

	static unsigned int convertIsoToUnixTimestamp(const std::string& isoTimestamp)
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
		std::vector<nlohmann::json> stopArrivalTimes = fetchStopArrivalTimes(stopIds);

		for (size_t i = 0; i < stopIds.size(); i++)
		{
			for (const nlohmann::json& arrivalInfo : stopArrivalTimes[i])
			{
				departureQueue.emplace(
					convertIsoToUnixTimestamp(arrivalInfo["times"][0]["scheduledDeparture"].get<std::string>()),
					stopIds[i], arrivalInfo["route"]["routeId"].get<int>());
			}
		}
	}
};

int main()
{
	BusTracker busTracker;
}