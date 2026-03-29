#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/Core.hpp>


#include <fstream>
#include <string>
#include <regex>
#include <iostream>
#include <mutex>

#define CURL_STATIC_LIB
#include <Security/Api/curl/curl.h>
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "Normaliz.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "Wldap32.lib" )




using json = nlohmann::json;

namespace Core
{
	namespace Threads
	{
		class cUpdateNames
		{
		private:
			std::string ServerIp;
			std::string ServerToken;
			std::string DirFiveM;
			std::string RedirectUrl;
		public:
			std::unordered_map<int, Core::SDK::Game::NetworkInfo> NetworkMap;
			std::mutex NetworkMapMutex;
		private:
			static size_t WriteCallBack(void* contents, size_t size, size_t nmemb, void* userp)
			{
				((std::string*)userp)->append((char*)contents, size * nmemb);
				return size * nmemb;
			}

			bool GetServerIpPort(const std::string& crashometryPath, std::string& outIp, int& outPort)
			{
				if (crashometryPath.empty())
					return false;

				std::ifstream file(crashometryPath, std::ios::binary);
				if (!file.is_open())
					return false;

				std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				std::string rawText(buffer.begin(), buffer.end());
				file.close();

				if (rawText.empty())
					return false;

				size_t pos = rawText.find("last_server_url");
				if (pos == std::string::npos)
					return false;

				std::string sub = rawText.substr(pos + strlen("last_server_url"));

				size_t startServer = sub.find("last_server");
				if (startServer == std::string::npos)
					return false;

				sub = sub.substr(startServer + strlen("last_server"));

				size_t colonPos = sub.find(':');
				if (colonPos == std::string::npos)
					return false;

				outIp = sub.substr(0, colonPos);

				// Port is assumed to be next 4-5 chars after colon (try to extract digits)
				std::string portStr;
				size_t portStart = colonPos + 1;

				while (portStart < sub.size() && isdigit(sub[portStart]) && portStr.size() < 6) {
					portStr += sub[portStart];
					portStart++;
				}

				if (portStr.empty())
					return false;

				outPort = std::stoi(portStr);
				return true;
			}



		public:

			std::string WstringToString(const std::wstring& wstr) {
				if (wstr.empty())
					return {};
				int n = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
				if (n <= 0)
					return {};
				std::string out(static_cast<size_t>(n), '\0');
				WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), out.data(), n, nullptr, nullptr);
				return out;
			}

			std::string GetCrashometryPath() {
				HKEY hKey;
				WCHAR buffer[MAX_PATH];
				DWORD bufferSize = sizeof(buffer);

				if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\CitizenFX\\FiveM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
					if (RegQueryValueExW(hKey, L"Last Run Location", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
						std::wstring basePath(buffer);
						std::string crashometryPath = WstringToString(basePath) + "\\data\\cache\\crashometry";

						if (std::filesystem::exists(crashometryPath)) {
							RegCloseKey(hKey);
							return crashometryPath;
						}
					}
					RegCloseKey(hKey);
				}

				return "";
			}

			nlohmann::json GetPlayerData(const std::string& crashometryFilePath) {
				std::string ServerIP;
				int ServerPort = 0;

				if (!GetServerIpPort(crashometryFilePath, ServerIP, ServerPort)) {
					return nullptr;
				}

				std::string ApiUrl = "http://" + ServerIP + ":" + std::to_string(ServerPort) + "/players.json";

				std::string ResponseStr;
				CURL* hnd = curl_easy_init();
				if (hnd) {
					curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
					curl_easy_setopt(hnd, CURLOPT_URL, ApiUrl.c_str());

					struct curl_slist* headers = NULL;
					headers = curl_slist_append(headers, "User-Agent: Fluffy-FiveM/1.0");
					curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

					curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack);
					curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &ResponseStr);
					curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 8L);
					curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 5L);

					curl_easy_perform(hnd);
					curl_slist_free_all(headers);
					curl_easy_cleanup(hnd);
				}

				if (ResponseStr.empty())
					return nullptr;

				try {
					nlohmann::json PlayersArray = nlohmann::json::parse(ResponseStr);
					return PlayersArray;
				}
				catch (...) {
					return nullptr;
				}
			}


			static bool ParsePlayerSlotId(const nlohmann::json& Player, int& outId)
			{
				outId = -1;
				if (!Player.contains("id"))
					return false;
				const auto& idNode = Player["id"];
				try {
					if (idNode.is_number_integer())
						outId = static_cast<int>(idNode.get<int64_t>());
					else if (idNode.is_number_unsigned())
						outId = static_cast<int>(idNode.get<uint64_t>());
					else if (idNode.is_string())
						outId = std::stoi(idNode.get<std::string>());
				}
				catch (...) {
					return false;
				}
				return outId >= 0;
			}

			void GetPlayerNames()
			{

				std::string crashometryPath = GetCrashometryPath();
				if (crashometryPath.empty())
					return;
				
				// Try to get player data from server API
				nlohmann::json PlayersArr = GetPlayerData(crashometryPath);

				// Always create entries for detected players, even if API fails
				{
					std::lock_guard<std::mutex> lk(NetworkMapMutex);
					
					// First, process API data if available
					if (PlayersArr.is_array())
					{
						for (const auto& Player : PlayersArr)
						{
							int PlayerId = -1;
							if (!ParsePlayerSlotId(Player, PlayerId))
								continue;

							if (!Player.contains("name"))
								continue;

							std::string PlayerName;
							if (Player["name"].is_string())
								PlayerName = Player["name"].get<std::string>();
							else
								PlayerName = Player["name"].dump();

							nlohmann::json Identifiers = Player.contains("identifiers") ? Player["identifiers"] : nlohmann::json::array();

							std::string Discord, SteamId;

							if (Identifiers.is_array())
							{
								for (const auto& Identifier : Identifiers)
								{
									if (!Identifier.is_string())
										continue;

									std::string IdentifierVal = Identifier.get<std::string>();

									if (IdentifierVal.rfind("discord:", 0) == 0)
										Discord = IdentifierVal.substr(8);
									else if (IdentifierVal.rfind("steam:", 0) == 0)
										SteamId = IdentifierVal.substr(6);
								}
							}

							NetworkMap[PlayerId] = {
								PlayerName, Discord, SteamId
							};
						}
					}
					
					// Then, ensure all detected entities have entries (fallback)
					for (const auto& Entity : Core::SDK::Game::EntityList)
					{
						if (Entity.Id > 0 && NetworkMap.find(Entity.Id) == NetworkMap.end())
						{
							// Create a basic entry with just the ID
							Core::SDK::Game::NetworkInfo BasicInfo;
							BasicInfo.UserName = "";  // Empty - will be handled by GetPlayerListLabel fallback
							BasicInfo.SteamId = "";
							BasicInfo.DiscordId = "";
							NetworkMap[Entity.Id] = BasicInfo;
						}
					}
				}
			}

			void Update()
			{
				try {
					GetPlayerNames();
				}
				catch (...) {
				}

				while (true)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
					try {

						GetPlayerNames();
					}
					catch (const std::exception& e) {
						std::string errorMessage = xorstr("Crash Detected. Code: 2\nException: ");
						errorMessage += e.what();
						MessageBox(NULL, errorMessage.c_str(), xorstr("Error"), MB_ICONERROR);
						break;
					}
					catch (...) {
						MessageBox(NULL, xorstr("Crash Detected. Code: 2\nUnknown exception caught."), xorstr("Error"), MB_ICONERROR);
						break;
					}

				}
			}

		};

		inline cUpdateNames g_UpdateNames;
	}

}