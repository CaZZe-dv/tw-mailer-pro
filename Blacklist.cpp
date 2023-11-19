#include <unordered_map>
#include <string>
#include <utility>
#include <chrono>
#include "Blacklist.hpp"

namespace TwmailerPro{
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    bool Blacklist::addToBlacklist(std::string item){
        if(list.find(item) == list.end()){
            list[item] = std::make_pair(0,getCurrentTime());
            return true;
        }
        return false;
    }
    TimePoint Blacklist::getCurrentTime(){
        return Clock::now();
    }
    bool Blacklist::incrementLoginCounter(std::string item){
        if(list[item].first == MAX_LOGIN_TRYS){
            return false;
        }
        list[item] = std::make_pair(list[item].first+1,getCurrentTime());
        return list[item].first < MAX_LOGIN_TRYS;
    }
    bool Blacklist::isCooldown(std::string user){
        TimePoint currentTime = Clock::now();
        auto cooldown = std::chrono::duration_cast<std::chrono::minutes>(currentTime-list[user].second);
        return cooldown.count() < TIMEOUT;
    }
    bool Blacklist::isBlacklisted(std::string user){
        if(!addToBlacklist(user)){
            if(!incrementLoginCounter(user)){
                if(!isCooldown(user)){
                    list[user].first = 0;
                    return false;
                }
                return true;
            }else{
                return false;
            }
        }
        return false;
    }
}
