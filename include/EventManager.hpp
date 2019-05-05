#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "tool/DateTime.hpp"
#include "tool/StringToNumber.hpp"
#include "tool/SubCharArray.hpp"
#include "tool/GetDirectory.hpp"
using namespace std;

#define EVENT_TYPE_READ_PATH "./data/NewPattern/EventTpyeFile"
#define EVENT_TYPE_WRITE_PATH "./data/NewPattern/EventTpyeFile_write"


/* ----- Event -----
 * 紀錄時間和所做的操作
 */
class Event{
	public :
		int eventID;
		DateTime time;
		
		Event() {}
		
		Event(int eventID) {
			this->eventID = eventID;
		}
		
		// ps: 用到 EventManager 所以實作在最下面
		void output() const;
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
		
		EventType() {}
		
		EventType(const string& name) {
			this->name = name;
		}
		
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
		
		void output() {
			printf("eventID:%3d, ignore:%5s, screenEvent:%5s, screenOn:%5s, appEvent:%5s, loadtime:%5f, %s", 
							eventID,
							ignore? "true":"false",
							screenEvent? "true":"false",
							screenOn? "true":"false",
							appEvent? "true":"false",
							loadtime,
							name.c_str());
		}
		
		bool operator == (const EventType& another) const {
			return eventID == another.eventID;
		}
		bool operator != (const EventType& another) const {
			return eventID != another.eventID;
		}
};

/* ----- Event Manager -----
 * 實做許多功能來幫助使用 Event, EventType
 */
class EventManager{
	public:
		static const Event emptyEvent;
		static const EventType emptyEventType;
		static vector<EventType> globalEventTypeVec;
		static bool isBuild; // 是否已經建立 globalEventTypeVec
		
		// build 會去檔案中將資料取出並整理
		static vector<EventType> buildGlobalEventTypeVec() {
			vector<EventType> EventTypeVec;
			// 讀取 eventTpyeFile
			FILE *eventTpyeFile;
			eventTpyeFile = fopen(EVENT_TYPE_READ_PATH, "r");
			
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
				cout << "(error) EventManager::buildGlobalEventTypeVec()"<<endl;
				fclose(eventTpyeFile);
				return EventTypeVec;
			}
			// 跳過一行 title
			if (fgets(getLine, getLineSize, eventTpyeFile) != NULL) {
				// do nothing
			} else {
				cout << "(error) EventManager::buildGlobalEventTypeVec()"<<endl;
				fclose(eventTpyeFile);
				return EventTypeVec;
			}
			
			// 讀各個 EventType
			for (int i=0; i<totalEventAmount; i++) {
				if (fgets(getLine, getLineSize, eventTpyeFile) != NULL) {
					EventType tempEventType;
					tempEventType.setData(getLine, getLineSize);
					EventTypeVec.push_back(tempEventType);
				} else {
					cout << "(error) EventManager::buildGlobalEventTypeVec()"<<endl;
					fclose(eventTpyeFile);
					return EventTypeVec;
				}
			}
			
			fclose(eventTpyeFile);
			isBuild = true;
			return EventTypeVec;
		};
		
		// save 會紀錄 globalEventTypeVec 至檔案中，其中也會包括新的 user 中從未看過的 app
		static void saveEventType() {
			if (!isBuild) {
				printf("(error) EventManager::saveEventType() globalEventTypeVec has not yet been built");
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			// 寫入 EventType
			FILE *appFile;
			appFile = fopen(EVENT_TYPE_WRITE_PATH, "w");
			
			// 多少個 EventType 
			string buffer;
			buffer = to_string(globalEventTypeVec.size()) + "|\n";
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			
			// 每一個 EventType 的內容
			buffer = EventType::getSaveTitleString() + '\n';
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			for (int i=0; i<globalEventTypeVec.size(); i++) {
				buffer = globalEventTypeVec.at(i).getSaveString();
				buffer += '\n';
				fwrite(buffer.c_str(),1,buffer.size(),appFile);
			}
			
			fclose(appFile);
		};
		
		// 取出上一次使用的 app, 沒有的話回傳 emptyEvent
		static const Event& getLastApp(const vector<Event>& eventVec, vector<Event>::const_iterator thisEvent) {
			// 和上一個 app event == true 的 event 比較是不是同一個 app (是:沒換 不是:有換)
			while (thisEvent!=eventVec.cbegin()) {
				thisEvent--;
				if (EventManager::getEventType(*thisEvent).appEvent) {
					return *thisEvent;
				}
			}
			return emptyEvent;
		}
		
		// 檢查 oneEvent 是不是切換 app 的時刻
		static bool isSwitchApp(const vector<Event>& eventVec, const vector<Event>::const_iterator& checkEvent) {
			// 和上一個 app event == true 的 event 比較是不是同一個 app (是:沒換 不是:有換)
			
			if (!EventManager::getEventType(*checkEvent).appEvent) {
				return false;
			}
			
			int nowEventID = checkEvent->eventID;
			const Event lastApp = EventManager::getLastApp(eventVec, checkEvent);
			return nowEventID != lastApp.eventID;
		}
		
		//{ ----- 以下是 依照傳入回傳所需
		const static vector<EventType>& getEventTyprVec() {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			return globalEventTypeVec;
		};
		
		const static string& getEventName(int eventID) {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			const EventType &resultEventType = getEventType(eventID);
			if (resultEventType != emptyEventType) {
				return resultEventType.name;
			} else {
				printf("(error) EventManager::getEventName(...) Can NOT find eventID (%d) in globalEventTypeVec\n", eventID);
				return NULL;
			}
		};
		
		const static EventType& getEventType(int eventID) {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			for (int i=0; i<globalEventTypeVec.size(); i++) {
				if (eventID == globalEventTypeVec.at(i).eventID) {
					return globalEventTypeVec.at(i);
				}
			}
			printf("(error) EventManager::getEventType(...) Can NOT find eventID (%d) in globalEventTypeVec\n", eventID);
			return emptyEventType;
		};
		
		const static EventType& getEventType(const Event& event) {
			return getEventType(event.eventID);
		};
		
		// 利用 name 取得 EventID，如果沒有在紀錄中的話會自行產生一個
		static int getEventID(const string &name) {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			for (int i=0; i<globalEventTypeVec.size(); i++) {
				if (name == globalEventTypeVec.at(i).name) {
					if (globalEventTypeVec.at(i).eventID < 0) {
						cout << "(error) EventManager::getEventID(...) has \"eventID\" problem" <<endl;
					}
					return globalEventTypeVec.at(i).eventID;
				}
			}
			
			EventType mEventType;
			mEventType.name = name;
			mEventType.eventID = globalEventTypeVec.size();
			globalEventTypeVec.push_back(mEventType);
			return mEventType.eventID;
		};
		
		// 回傳依照 index = eventID 的 allEventNameVec
		static vector<string> getAllEventNameVec() {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			vector<string> allEventNameVec;
			// 設置 allEventNameVec 的大小
			int maxID = 0;
			for (auto oneEventType =globalEventTypeVec.begin(); oneEventType!=globalEventTypeVec.end(); oneEventType++) {
				if (maxID < oneEventType->eventID) {
					maxID = oneEventType->eventID;
				}
			}
			allEventNameVec.resize(maxID+1);
			
			for (auto oneEventType =globalEventTypeVec.begin(); oneEventType!=globalEventTypeVec.end(); oneEventType++) {
				int index = oneEventType->eventID;
				allEventNameVec[index] = oneEventType->name;
			}
			return allEventNameVec;
		};
		//}
		
		// print globalEventTypeVec
		static void output() {
			if (!isBuild) {
				globalEventTypeVec = buildGlobalEventTypeVec();
			}
			for (auto oneEventType =globalEventTypeVec.begin(); oneEventType!=globalEventTypeVec.end(); oneEventType++) {
				oneEventType->output();
				printf("\n");
			}
		}
};
const Event EventManager::emptyEvent = Event(-1);
const EventType EventManager::emptyEventType = EventType(string("NULL"));
bool EventManager::isBuild = false;
vector<EventType> EventManager::globalEventTypeVec = EventManager::buildGlobalEventTypeVec();

void Event::output() const {
	printf("%3d: %s", eventID, EventManager::getEventName(eventID).c_str());
};
#endif // EVENT_MANAGER_HPP