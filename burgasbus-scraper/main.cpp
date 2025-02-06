#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <format>
#include <string_view>
#include <iostream>
using namespace std::literals;

nlohmann::json fetchJson(std::string_view url);

class BusTracker
{
public:
	BusTracker()
	{
		constexpr auto stopsApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops"sv;
		constexpr auto routesApiUrl = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/routes"sv;

		for (const auto& stops = fetchJson(stopsApiUrl); const auto& stop : stops)
		{
			stopIds.push_back(stop.at("id").get<int>());
			stopNames[stop.at("id").get<int>()] = stop.at("name").get<std::string_view>();
		}

		for (const auto& routes = fetchJson(routesApiUrl); const auto& route : routes)
		{
			routeIds.push_back(route.at("id").get<int>());
			routeNames[route.at("id").get<int>()] = route.at("shortName").get<std::string_view>();
		}

		for (const auto& stopId : stopIds)
			responses.push_back(fetchJson("https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/" + std::to_string(stopId) + "/times"));

		for (const auto& response : responses)
		{
			std::cout << std::setw(4) << response << '\n';
		}
	}

private:


	std::vector<int> stopIds;
	std::string_view stopNames[380];
	std::vector<int> routeIds;
	std::string_view routeNames[70];
	std::vector<nlohmann::json> responses;
};

nlohmann::json fetchJson(const std::string_view url)
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

int main()
{

	BusTracker busTracker;
}