#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include "tool/DateTime.hpp"
#include "tool/StringToNumber.hpp"
#include "tool/SubCharArray.hpp"
using namespace std;

int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

/* ----- Event -----
 * 紀錄時間和所做的操作
 */
class Event{
	public :
		int eventID;
		DateTime time;
};

/* ----- Event Type -----
 * 紀錄一個 Event 所代表的意思
 * 可以從其中知道 Event.eventID 所代表的是
 *  | 使用的 APP
 *  | 開機關機
 *  | 螢幕狀態
 *  └ 返回桌面 ...等
 * 以及更細節的資料 (e.g. 使用那個 app、螢幕是亮還暗)
 */
class EventType{
	public :
		int eventID = -1;
		string name;
		bool ignore = false;
		bool screenEvent = false;
		bool screenOn = true; // screenEvent == true 此數值才有意義
		bool appEvent = false;
		double loadtime = -1;
		
		string getSaveString() {
			string buffer;
			// eventID"name"ignore"screenEvent"screenOn"appEvent"loadtime"
			buffer  = to_string(eventID);
			buffer += '"';
			buffer += name;
			buffer += '"';
			buffer += ignore ? "1":"0";
			buffer += '"';
			buffer += screenEvent ? "1":"0";
			buffer += '"';
			buffer += screenOn ? "1":"0";
			buffer += '"';
			buffer += appEvent ? "1":"0";
			buffer += '"';
			buffer += to_string(loadtime);
			buffer += '"';
			return buffer;
		}
		
		static string getSaveTitleString() {
			string buffer;
			// eventID"name"ignore"screenEvent"screenOn"appEvent"loadtime"
			buffer  = "eventID";
			buffer += '"';
			buffer += "name";
			buffer += '"';
			buffer += "ignore";
			buffer += '"';
			buffer += "screenEvent";
			buffer += '"';
			buffer += "screenOn";
			buffer += '"';
			buffer += "appEvent";
			buffer += '"';
			buffer += "loadtime";
			buffer += '"';
			return buffer;
		}
		
		bool setData(const char* charArray, int size) {
			string temp;
			int value;
			
			// 0 : eventID
			temp = subCharArray(charArray, size, '"', 0);
			if (StringToNumber(temp, &value)) {
				eventID = value;
			} else {
				return false;
			}
			
			// 1 : name
			temp = subCharArray(charArray, size, '"', 1);
			name = string(temp);
			
			// 2 : ignore
			temp = subCharArray(charArray, size, '"', 2);
			if (StringToNumber(temp, &value)) {
				if (value == 1) {
					ignore = true;
				} else if (value == 0) {
					ignore = false;
				} else { return false; }
			} else { return false; }
			
			// 3 : screenEvent
			temp = subCharArray(charArray, size, '"', 3);
			if (StringToNumber(temp, &value)) {
				if (value == 1) {
					screenEvent = true;
				} else if (value == 0) {
					screenEvent = false;
				} else { return false; }
			} else { return false; }
			
			// 4 : screenOn
			temp = subCharArray(charArray, size, '"', 4);
			if (StringToNumber(temp, &value)) {
				if (value == 1) {
					screenOn = true;
				} else if (value == 0) {
					screenOn = false;
				} else { return false; }
			} else { return false; }
			
			// 5 : appEvent
			temp = subCharArray(charArray, size, '"', 5);
			if (StringToNumber(temp, &value)) {
				if (value == 1) {
					appEvent = true;
				} else if (value == 0) {
					appEvent = false;
				} else { return false; }
			} else { return false; }
			
			// 6 : loadtime (double)
			temp = subCharArray(charArray, size, '"', 6);
			string::size_type sz;     // alias of size_t
			loadtime = std::stod(temp, &sz);
			
			return true;
		}
};

class User{
	public :
		int userID;
		string fileName;
		vector<Event> eventVec;
		User() {};
		
		void output() {
			printf("%lu\n", eventVec.size());
			return;
		};
		
		static bool cmp_by_ID(const User &a, const User &b) {
			return a.userID < b.userID;
		};
};
class UserManager{
	public :
		vector<User> userVec;
		vector<EventType> eventTypeVec;
		
		int getEventID(const string &name) {
			for (int i=0; i<eventTypeVec.size(); i++) {
				if (name == eventTypeVec.at(i).name) {
					if (eventTypeVec.at(i).eventID < 0) {
						cout << "(error) UserManager::getEventID(...) has \"eventID\" problem" <<endl;
					}
					return eventTypeVec.at(i).eventID;
				}
			}
			
			EventType mEventType;
			mEventType.name = name;
			mEventType.eventID = eventTypeVec.size();
			eventTypeVec.push_back(mEventType);
			return mEventType.eventID;
		};
		
		string* getName(int eventID) {
			EventType *resultEventType = getEventType(eventID);
			if (resultEventType != NULL) {
				return &resultEventType->name;
			} else {
				printf("(error) UserManager::getName(...) Can NOT find eventID (%d) in eventTypeVec\n", eventID);
				return NULL;
			}
		};
		
		EventType* getEventType(int eventID) {
			for (int i=0; i<eventTypeVec.size(); i++) {
				if (eventID == eventTypeVec.at(i).eventID) {
					return &eventTypeVec.at(i);
				}
			}
			printf("(error) UserManager::getEventType(...) Can NOT find eventID (%d) in eventTypeVec\n", eventID);
			return NULL;
		};
		
		// 回傳依照 index = eventID 的 allEventNameVec
		vector<string> getAllEventNameVec() {
			vector<string> allEventNameVec;
			// 設置 allEventNameVec 的大小
			int maxID = 0;
			for (auto oneEventType =eventTypeVec.begin(); oneEventType!=eventTypeVec.end(); oneEventType++) {
				if (maxID < oneEventType->eventID) {
					maxID = oneEventType->eventID;
				}
			}
			allEventNameVec.resize(maxID+1);
			
			for (auto oneEventType =eventTypeVec.begin(); oneEventType!=eventTypeVec.end(); oneEventType++) {
				int index = oneEventType->eventID;
				allEventNameVec[index] = oneEventType->name;
			}
			return allEventNameVec;
		};
};

class FileManager {
	public :
		string inputFolder;
		vector<string> inputFiles;
		FileManager(const string &inputFolder) {
			this->inputFolder = inputFolder;
		};
		
		bool initialUserFiles() {
			// 取出檔案 list
			bool isFind = false;
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
		}
		
		UserManager buildUserManager() {
			UserManager userManager;
			
			// ----- build EventTypeVec of userManager
			userManager.eventTypeVec = buildEventType();
			
			//{ ----- initial userVec of userManager
			for (auto onefile = inputFiles.begin(); onefile!=inputFiles.end(); onefile++) {
				userManager.userVec.push_back(User());
				User *user = &userManager.userVec.back();
				user->fileName = *onefile;
				user->userID = stoi(onefile->substr(4));
			}
			sort(userManager.userVec.begin(), userManager.userVec.end(), User::cmp_by_ID);//}
			
			// ----- build all userVec of userManager
			for (auto oneUser =userManager.userVec.begin(); oneUser!=userManager.userVec.end(); oneUser++) {
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
					printf("(error) FileManager::buildUserManager(...): Unable to read the start line. File: %s\n", path.c_str());
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
						int eventID = userManager.getEventID(eventName);
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
						
						// 裝入 tempEventVec
						oneUser->eventVec.push_back(tempEvent);
					} else {
						printf("(error) FileManager::buildUserManager(...): Unable to read the Part 2. File: %s\n", path.c_str());
						break;
					}
				}
				fclose(userFile);//}
				
				// 實驗所需的數據
				//vector<Event> eventVec;
				//vector<string> allAppNameVec = appManager.getAllEventNameVec();
				
				/*//{ 刪掉 ignore
				for(vector<TempEvent>::iterator iter = tempEventVec.begin(); iter != tempEventVec.end();) {
					EventManager::AppDetail *thisAppDetial = appManager.getEventType(iter->namePoint);
					if (thisAppDetial->ignore) {
						iter = tempEventVec.erase(iter);
					} else {
						iter++;
					}
				}//}*/
				
				/*//{ 刪掉重複
				vector<TempEvent>::iterator lastIter = tempEventVec.begin();
				vector<TempEvent>::iterator thisIter = tempEventVec.begin();
				thisIter++;
				while (thisIter != tempEventVec.end()) {
					if (lastIter->namePoint != thisIter->namePoint) {
						// 不一樣的話沒事
						lastIter++;
						thisIter++;
					} else {
						// 一樣的話要刪掉 thisIter
						thisIter = tempEventVec.erase(thisIter);
					}
				}//}*/
			}
			return userManager;
		};
		
		vector<EventType> buildEventType() {
			vector<EventType> eventTypeVec;
			// 讀取 eventTpyeFile
			FILE *eventTpyeFile;
			string path = "./data/NewPattern/EventTpyeFile";
			eventTpyeFile = fopen(path.c_str(), "r");
			
			int getLineSize = 1024;
			char getLine[getLineSize];
			int tempInt;
			
			int totalEventAmount = 0;
			
			// 讀取數量
			if (fgets(getLine, getLineSize, eventTpyeFile) != NULL) {
				string eventID = subCharArray(getLine, getLineSize, '|', 0);
				StringToNumber(eventID, &tempInt);
				totalEventAmount = tempInt;
			} else {
				cout << "(error) EventManager::ReadData()"<<endl;
				fclose(eventTpyeFile);
				return eventTypeVec;
			}
			// 跳過一行 title
			if (fgets(getLine, getLineSize, eventTpyeFile) != NULL) {
				// do nothing
			} else {
				cout << "(error) EventManager::ReadData()"<<endl;
				fclose(eventTpyeFile);
				return eventTypeVec;
			}
			
			// 讀各個 EventType
			for (int i=0; i<totalEventAmount; i++) {
				if (fgets(getLine, getLineSize, eventTpyeFile) != NULL) {
					EventType tempEventType;
					tempEventType.setData(getLine, getLineSize);
					eventTypeVec.push_back(tempEventType);
				} else {
					cout << "(error) EventManager::ReadData()"<<endl;
					fclose(eventTpyeFile);
					return eventTypeVec;
				}
			}
			
			fclose(eventTpyeFile);
			return eventTypeVec;
		};
		
		void saveEventType(vector<EventType> &eventTypeVec) {
			// 寫入 EventType
			FILE *appFile;
			string path = "./data/NewPattern/EventTpyeFileExtra";
			appFile = fopen(path.c_str(), "w");
			
			// 多少個 EventType 
			string buffer;
			buffer = to_string(eventTypeVec.size()) + "|\n";
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			
			// 每一個 EventType 的內容
			buffer = EventType::getSaveTitleString() + '\n';
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			for (int i=0; i<eventTypeVec.size(); i++) {
				buffer = eventTypeVec.at(i).getSaveString();
				buffer += '\n';
				fwrite(buffer.c_str(),1,buffer.size(),appFile);
			}
			
			fclose(appFile);
		}
};

