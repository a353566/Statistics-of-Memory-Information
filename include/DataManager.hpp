#ifndef DATA_MANAGER_HPP
#define DATA_MANAGER_HPP

#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "tool/DateTime.hpp"
#include "tool/StringToNumber.hpp"
#include "tool/SubCharArray.hpp"
#include "tool/GetDirectory.hpp"
#include "EventManager.hpp"
using namespace std;

class User{
	public :
		int userID;
		string fileName;
		vector<Event> eventVec;
		double blankTime;
		
		// 印出所有資料 包括 eventVec 的內容
		void output() {
			printf(" ----- user %d , file: \"%s\" , size is %lu\n", userID, fileName.c_str(), eventVec.size());
			
			for(auto oneEvent =eventVec.begin(); oneEvent !=eventVec.end(); oneEvent++) {
				oneEvent->time.output_colon_ver();
				printf(" %s\n", EventManager::getEventName(oneEvent->eventID).c_str());
			}
			return;
		};
		
		static bool cmp_by_ID(const User &a, const User &b) {
			return a.userID < b.userID;
		};
};

class DataManager {
	public :
		string inputFolder;
		vector<string> inputFiles;
		vector<double> blankTimesVec;
		DataManager(const string &inputFolder, const string &files, const string &blankTimes) {
			this->inputFolder = inputFolder;
			
			string checkFiles = files + ",";
			for (int i=0; true; i++) {
				string oneFile = subCharArray(checkFiles.c_str(), checkFiles.size(), ',', i);
				if (oneFile != "") {
					inputFiles.push_back(oneFile);
				} else {
					break;
				}
			}
			
			string mblankTimes = blankTimes + ",";
			for (int i=0; true; i++) {
				string oneblankTime = subCharArray(mblankTimes.c_str(), mblankTimes.size(), ',', i);
				if (oneblankTime != "") {
					blankTimesVec.push_back(stod(oneblankTime));
				} else {
					break;
				}
			}
		};
		
		/*bool initialUserFiles() {
			// 取出檔案 list
			vector<string> fileVec;
			if (getdir(inputFolder.c_str(), fileVec) == -1) {
				printf("(error) can Not open %s\n", inputFolder.c_str());
				return false;
			}
			// 檢查有沒有前綴 "user"，有則加入 inputFiles
			for (auto onefile =fileVec.begin(); onefile!=fileVec.end(); onefile++) {
				if (onefile->compare(0, 4, "user") == 0) {
					inputFiles.push_back(*onefile);
				}
			}
		}*/
		
		vector<User> buildUserDatas() {
			vector<User> userVec;
			
			//{ ----- initial userVec
			//for (auto onefile = inputFiles.begin(); onefile!=inputFiles.end(); onefile++) {
			for (int i=0; i<inputFiles.size(); i++) {
				const string &onefile = inputFiles[i];
				userVec.push_back(User());
				User &user = userVec.back();
				user.fileName = onefile;
				user.userID = stoi(onefile.substr(4));
				user.blankTime = blankTimesVec[i];
			}
			sort(userVec.begin(), userVec.end(), User::cmp_by_ID);//}
			
			// ----- build all userVec
			for (auto oneUser =userVec.begin(); oneUser!=userVec.end(); oneUser++) {
				// ----- read one file
				//{ 開檔部分
				FILE *userFile;
				string path = inputFolder + oneUser->fileName;
				userFile = fopen(path.c_str(),"r"); // "r" 讀檔而已
				if(userFile == NULL) {
					printf("open \"%s\" File fail\n", path.c_str());
					continue;
				}//}
				
				//{ initial
				int line=0;
				int getLineSize = 4096;
				char getLine[getLineSize]; //}
				
				//{ 開始讀檔
				if (fgets(getLine, getLineSize, userFile) != NULL) {  // 跳過一行
						line++;
				} else {
					printf("(error) DataManager::buildUserManager(...): Unable to read the start line. File: %s\n", path.c_str());
					fclose(userFile);
					continue;
				}
				while (true) {
					if (fgets(getLine, getLineSize, userFile) != NULL) {  // 讀一行
						line++;
						// 若讀完則跳出結束 (讀到 "" 代表結束)
						if (strstr(getLine, "\"\"") != NULL) {
							break;
						}
						Event tempEvent;
						
						//{ Name
						string eventName = subCharArray(getLine, getLineSize, '"', 1);
						int eventID = EventManager::getEventID(eventName);
						//int eventID = 0;
						tempEvent.eventID = eventID;//}
						
						//{ Date
						int tempInt;
						string tempStr;
						tempEvent.time.initial();
						string date = subCharArray(getLine, getLineSize, '"', 3) + '/';
						tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // test "2019/1/15" or "12/27/18"
						StringToNumber(tempStr, &tempInt);
						if (tempInt>2000) { // "2019/1/15" 類型
							tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // year
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.year = tempInt;
							tempStr = subCharArray(date.c_str(), date.size(), '/', 1); // month
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.month = tempInt;
							tempStr = subCharArray(date.c_str(), date.size(), '/', 2); // day
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.day = tempInt;
						} else { // "12/27/18" 類型
							tempStr = subCharArray(date.c_str(), date.size(), '/', 2); // year
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.year = 2000 + tempInt;
							tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // month
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.month = tempInt;
							tempStr = subCharArray(date.c_str(), date.size(), '/', 1); // day
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.day = tempInt;
							
						}//}
						
						//{ Time
						string time = subCharArray(getLine, getLineSize, '"', 5) + ':';
						tempStr = subCharArray(time.c_str(), time.size(), ':', 0); // hour
						if (tempStr.substr(0,6) == "上午" || tempStr.substr(0,6) == "下午") { // "上午12:33:37" 類型
							StringToNumber(tempStr.substr(6), &tempInt);
							tempEvent.time.hour = (tempInt==12)? 0 : tempInt;
							if ((tempStr.substr(0,6) == "下午")) {
								tempEvent.time.hour += 12;
							}
						} else if (time.substr(time.size()-3,2) == "AM" || time.substr(time.size()-3,2) == "PM") { // "12:16:37 AM" 類型
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.hour = (tempInt==12)? 0 : tempInt;
							if (time.substr(time.size()-3,2) == "PM") {
								tempEvent.time.hour += 12;
							}
						} else { // "00:10:20" 類型
							StringToNumber(tempStr, &tempInt);
							tempEvent.time.hour = tempInt;
						}
						tempStr = subCharArray(time.c_str(), time.size(), ':', 1); // minute
						StringToNumber(tempStr, &tempInt);
						tempEvent.time.minute = tempInt;
						tempStr = subCharArray(time.c_str(), time.size(), ':', 2); // second
						StringToNumber(tempStr.substr(0,2), &tempInt);
						tempEvent.time.second = tempInt;//}
						
						// Date Time test output
						//tempEvent.time.output(); cout << " real: " << date << " - " << time <<endl;
						
						// 裝入 tempEventVec (從前面裝，讓順序是由舊到新)
						oneUser->eventVec.insert(oneUser->eventVec.begin(), tempEvent);
					} else {
						printf("(error) DataManager::buildUserManager(...): Unable to read the Part 2. File: %s\n", path.c_str());
						break;
					}
				}
				fclose(userFile);//}
			}
			
			// ----- remove blank time
			for (auto oneUser =userVec.begin(); oneUser!=userVec.end(); oneUser++) {
				// 要空出來的時間
				DateTime blankDateTime;
				blankDateTime.initial();
				blankDateTime.initial_Day(oneUser->blankTime);
				
				vector<Event> &eventVec = oneUser->eventVec;
				const DateTime startTime = eventVec[0].time;
				vector<Event>::iterator newStartEvent = eventVec.begin();
				while (blankDateTime > newStartEvent->time - startTime) {
					newStartEvent = eventVec.erase(newStartEvent);
				}
				
			}
			
			return userVec;
		};

};

#endif // DATA_MANAGER_HPP