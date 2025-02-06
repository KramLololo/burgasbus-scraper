#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <string_view>
#include <iostream>
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

		for (const auto& stopId : stopIds)
			timesPerStop.push_back(fetchJson("https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" + std::to_string(stopId) + "/times"));

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

	static nlohmann::json fetchJson(const std::string_view& url)
	{
		cpr::Response response = Get(cpr::Url{url});
		std::cout << url << '\n';
		while (response.text.empty() || response.status_code != 200)
		{
			// TODO: Replace with actual failure handling
			std::cout << "retry " << url << '\n';
			response = Get(cpr::Url{url});
		}

		return nlohmann::json::parse(response.text);
	}
};

int main()
{
	BusTracker busTracker;
}