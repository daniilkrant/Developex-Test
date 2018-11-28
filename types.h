#ifndef TYPES_H
#define TYPES_H
#include <string>

using URL_PAIR = std::pair<std::string, std::string>; //url, status
constexpr auto invalidInpStringErr = "URL or keyword is empty, make no sense to start a program";
constexpr auto invalidInpIntErr = "Threads or Depth value = 0, make no sense to start a program";
constexpr auto invalidUrlErr = "Invalid URL provided or server is unavailable";
constexpr auto keywordFoundStatus = "found";
constexpr auto keywordNotFoundStatus = "not found";
constexpr auto workingOnThatStatus = "analyzing";
constexpr auto errorPrefix= "error: ";
constexpr auto STATUS_WORKING = 0;
constexpr auto STATUS_PAUSED = 1;
constexpr auto STATUS_STOPPED = 2;

#endif // TYPES_H
