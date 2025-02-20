#include <iostream>
#include <memory>
#include <queue>
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
		constexpr auto stopsApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops"sv;
		constexpr auto routesApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/routes"sv;

		for (const auto& stop : fetchJson(stopsApiUrl))
		{
			const int& stopId = stop["id"].get<int>();
			stopIds.push_back(stopId);
			stopNames[stopId] = stop.at("name").get<std::string_view>();
		}

		for (const auto& route : fetchJson(routesApiUrl))
		{
			const int& routeId = route["id"].get<int>();
			routeIds.push_back(routeId);
			routeNames[routeId] = route.at("shortName").get<std::string_view>();
		}

		prepareBusTimeRequests();
		timesPerStop = fetchStopTimes(stopIds);

		for (const nlohmann::json& stopTimes : timesPerStop)
		{
			std::cout << std::setw(4) << stopTimes << '\n';
		}
	}

private:
	std::vector<int> stopIds;
	std::string_view stopNames[380];
	std::vector<int> routeIds;
	std::string_view routeNames[70];
	std::vector<nlohmann::json> timesPerStop;
	std::unordered_map<int, std::shared_ptr<cpr::Session>> timeRequestSessions;

	void addTimeRequestSession(const int stopId, const std::shared_ptr<cpr::Session>& session)
	{
		timeRequestSessions[stopId] = session;
	}

	std::shared_ptr<cpr::Session>& getTimeRequestSession(const int stopId)
	{
		return timeRequestSessions.at(stopId);
	}

	void prepareBusTimeRequests()
	{
		for (const int stopId : stopIds)
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

	// TODO: Should this function just modify an existing vector? Must see all use cases...
	std::vector<nlohmann::json>/*&*/ fetchStopTimes(const std::vector<int>& stopIds)
	{
		std::vector<nlohmann::json> timesPerStop;

		for (auto& times : requestTimesOfStops(stopIds))
		{
			validateResponse(times);

			timesPerStop.push_back(nlohmann::json::parse(times.text));
		}

		return timesPerStop;
	}
};

int main()
{
	BusTracker busTracker;
}