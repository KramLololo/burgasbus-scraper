cmake_minimum_required(VERSION 3.28.3)

project(BurgasbusApiScraper)

set(CMAKE_CXX_STANDARD 20)

find_package(cpr CONFIG REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(BuildTimetables burgasbus-scraper/main.cpp)

target_link_libraries(BuildTimetables cpr::cpr RocksDB::rocksdb nlohmann_json::nlohmann_json)