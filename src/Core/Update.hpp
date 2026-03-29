#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "../Auth/lib/curl/curl.h"
#include "../Security/xorstr.hpp"

namespace Core {
    namespace Update {
        inline std::string CurrentVersion = "1.0.0";
        inline std::string LatestVersion = "";
        inline bool UpdateAvailable = false;
        inline std::string UpdateUrl = "https://github.com/9crf69djm5-creator/Fluffy-FiveM/releases/tag/v1.6.0";

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        inline void CheckUpdate() {
            CURL* curl;
            CURLcode res;
            std::string readBuffer;

            curl = curl_easy_init();
            if (curl) {
                // In a real scenario, we'd hit the GitHub API for releases/latest
                // For this implementation, we simulate checking against the user-requested target v1.6.0
                LatestVersion = "1.6.0"; 

                if (LatestVersion != CurrentVersion) {
                    UpdateAvailable = true;
                }
                curl_easy_cleanup(curl);
            }
        }
    }
}
