#pragma once

#include <unordered_map>
#include <string>
#include <utility>
#include <chrono>

namespace TwmailerPro{
    class Blacklist{
        private:
            const static int TIMEOUT = 1;
            const static int MAX_LOGIN_TRYS = 3;
            using Clock = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;
            std::unordered_map<std::string, std::pair<int,TimePoint>> list;
        public:
            bool addToBlacklist(std::string item);
            TimePoint getCurrentTime();
            bool incrementLoginCounter(std::string item);    
            bool isCooldown(std::string user);
            bool isBlacklisted(std::string user);
    };
}