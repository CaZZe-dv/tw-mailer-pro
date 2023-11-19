#include <unordered_map>
#include <string>
#include <utility>
#include <chrono>

class Blacklist{
    private:
        const static int TIMEOUT = 1;
        const static int MAX_LOGIN_TRYS = 3;
        using Clock = std::chrono::steady_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        std::unordered_map<std::string, std::pair<int,TimePoint>> list;
    public:
        bool addToBlacklist(std::string item){
            if(list.find(item) == list.end()){
                list[item] = std::make_pair(1,getCurrentTime());
                return true;
            }
            return false;
        }

        TimePoint getCurrentTime(){
            return Clock::now();
        }

        bool incrementLoginCounter(std::string item){
            if(list[item].first+1 < MAX_LOGIN_TRYS){
                list[item] = std::make_pair()
            }
        }

        bool isBlacklisted(std::string user, std::string ip){
            if(!addToBlacklist(user)){

            }
        }
        










}
