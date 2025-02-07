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

	static std::vector<nlohmann::json>/*&*/ fetchStopTimes(const std::vector<auto>& stopIds)
	{
		std::vector<nlohmann::json> timesPerStop;
		//std::vector<cpr::AsyncResponse> responses;

		for (const auto& stopId : stopIds)
			responses.push_back(cpr::GetAsync(cpr::Url{
				"https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" +
				std::to_string(stopId) + "/times"
			}));
		for (auto& times : responses)
		{
			auto r = times.get();
			while (r.status_code != 200)
				r = cpr::GetAsync(r.url).get();
			timesPerStop.push_back(nlohmann::json::parse(r.text));
		}

		return timesPerStop;
	}
};

int main()
{
	//std::array<cpr::AsyncResponse, 2> response{};
	//cpr::AsyncResponse rer[2]{};
	//TODO: Make function validate() to handle fetching failure
	BusTracker busTracker;
}