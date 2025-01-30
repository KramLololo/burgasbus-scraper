#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string_view>
#include <format>
using namespace std::literals;

int main()
{
	constexpr auto stopRequestTemplate = "https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops/{}/times"sv;
	constexpr auto stopsApiUrl         = cpr::Url{"https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/stops"sv};
	constexpr auto routesApiUrl        = cpr::Url{"https://telelink.city/api/v1/949021bc-c2c0-43ad-a146-20e19bbc3649/transport/planner/routes"sv};
	constexpr auto stopId              = 340; // TODO: Remove after implementing requesting


	const cpr::Response response = Get(cpr::Url{std::format(templateUrl, station)});
	const nlohmann::json body = response.text;

	if (response.status_code == 200)
		std::cout << std::setw(4) << body << '\n';
}