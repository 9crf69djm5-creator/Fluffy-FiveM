#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "../Auth/lib/curl/curl.h"
#include "../Security/xorstr.hpp"
#include "../Auth/json.hpp" // JSON parsing

namespace Core {
    namespace Update {
        inline std::string CurrentVersion = "1.7.0"; // Matches APP_VERSION
        inline std::string LatestVersion = "";
        inline bool UpdateAvailable = false;
        inline std::string UpdateUrl = "https://api.github.com/repos/9crf69djm5-creator/Fluffy-FiveM/releases/latest";
        inline std::string DownloadUrl = "";

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
                curl_easy_setopt(curl, CURLOPT_URL, UpdateUrl.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                
                // GitHub requires a valid User-Agent header
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "User-Agent: Fluffy-Updater");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                res = curl_easy_perform(curl);
                if (res == CURLE_OK && !readBuffer.empty()) {
                    try {
                        auto jsonResponse = nlohmann::json::parse(readBuffer);
                        if (jsonResponse.contains("tag_name")) {
                            LatestVersion = jsonResponse["tag_name"];
                            // Clean up 'v' if it starts with 'v' like 'v1.0.1'
                            if (!LatestVersion.empty() && LatestVersion[0] == 'v') {
                                LatestVersion = LatestVersion.substr(1);
                            }

                            if (LatestVersion != CurrentVersion) {
                                UpdateAvailable = true;
                                if (jsonResponse.contains("assets") && jsonResponse["assets"].is_array() && jsonResponse["assets"].size() > 0) {
                                    DownloadUrl = jsonResponse["assets"][0]["browser_download_url"];
                                }
                            }
                        }
                    } catch (...) {
                        // Ignore JSON parsing errors
                    }
                }
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
            }
        }
    }
}
