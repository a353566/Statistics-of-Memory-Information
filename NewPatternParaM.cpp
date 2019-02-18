//mason 2018/12/13
// ：重要參數可以在 include/DataMiningExperimentForM.hpp 的 EXPERIMENT_GSP_power_of_level_part 地方改

#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

// ----- display part -----
//#define MAIN_display_File_name

// ----- "interval day" of training & test -----
#define ADD_TIME_SPACE_ON_HEAD // 開起前面加一些空白時段
// ----- parameter
int START_SPACE_TIME;
int TRAINING_INTERVAL_DAY;
int TEST_INTERVAL_DAY;

#define MORE_DETAIL_OUTPUT  //顯示細節

#include "include/MergeFile.hpp"
#include "include/DataMiningExperimentForM.hpp"
using namespace std;
int getNameID(string *name, vector<string> *allAppNameVec);
int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

class TempEvent{
	public :
		int namePoint;
		DateTime time;
};

class AppManager{
	public :
		class AppDetail{
			public :
				string name;
				int nameID = -1;
				bool ignore = false;
				bool screenStatus = false;
				bool screenOn = true; // screenStatus == true 此數值才有意義
				double loadtime = -1;
				
				string getSaveString() {
					string buffer;
					buffer  = to_string(nameID);
					buffer += '"';
					buffer += name;
					buffer += '"';
					buffer += ignore ? "1":"0";
					buffer += '"';
					buffer += screenStatus ? "1":"0";
					buffer += '"';
					buffer += screenOn ? "1":"0";
					buffer += '"';
					buffer += to_string(loadtime);
					buffer += '"';
					return buffer;
				}
				
				static string getSaveTitleString() {
					string buffer;
					buffer  = "ID";
					buffer += '"';
					buffer += "name";
					buffer += '"';
					buffer += "ignore";
					buffer += '"';
					buffer += "screenStatus";
					buffer += '"';
					buffer += "screenOn";
					buffer += '"';
					buffer += "loadtime";
					buffer += '"';
					return buffer;
				}
				
				bool setData(const char* charArray, int size) {
					string temp;
					int value;
					
					// 0 : ID
					temp = subCharArray(charArray, size, '"', 0);
					if (StringToNumber(temp, &value)) {
						nameID = value;
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
					
					// 3 : screenStatus
					temp = subCharArray(charArray, size, '"', 3);
					if (StringToNumber(temp, &value)) {
						if (value == 1) {
							screenStatus = true;
						} else if (value == 0) {
							screenStatus = false;
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
					
					// 5 : loadtime (double)
					temp = subCharArray(charArray, size, '"', 5);
					std::string::size_type sz;     // alias of size_t
					loadtime = std::stod(temp,&sz);
					
					return true;
				}
		};
		
		vector<AppDetail> appDetailVec;
		
		int getAppID(string *name) {
			for (int i=0; i<appDetailVec.size(); i++) {
				if (*name == appDetailVec.at(i).name) {
					if (appDetailVec.at(i).nameID == -1) {
						cout << "(error) AppManager::getAppID(...) has \"nameID\" problem" <<endl;
					}
					return appDetailVec.at(i).nameID;
				}
			}
			
			AppDetail mAppDetail;
			mAppDetail.name = *name;
			mAppDetail.nameID = appDetailVec.size();
			appDetailVec.push_back(mAppDetail);
			return mAppDetail.nameID;
		}
		
		string getName(int nameID) {
			for (int i=0; i<appDetailVec.size(); i++) {
				if (nameID == appDetailVec.at(i).nameID) {
					return appDetailVec.at(i).name;
				}
			}
			cout << "(error) AppManager::getName(...) Can NOT find ID in appDetailVec" <<endl;
			return string("");
		}
		
		AppDetail* getAppDetail(int nameID) {
			for (int i=0; i<appDetailVec.size(); i++) {
				if (nameID == appDetailVec.at(i).nameID) {
					return &appDetailVec.at(i);
				}
			}
			cout << "(error) AppManager::getName(...) Can NOT find ID in appDetailVec" <<endl;
			return NULL;
		}
		
		vector<string> getAllAppNameVec() {
			vector<string> allAppNameVec;
			for (int i=0; i<appDetailVec.size(); i++) {
				allAppNameVec.push_back(appDetailVec.at(i).name);
			}
			return allAppNameVec;
		}
		
		void SaveData() {
			// 寫入 app 總檔
			FILE *appFile;
			string path = "./data/NewPattern/NewAppNameFile";
			appFile = fopen(path.c_str(), "w");
			
			// 多少個 app
			string buffer;
			buffer = to_string(appDetailVec.size()) + "|\n";
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			
			// app detail
			buffer = AppDetail::getSaveTitleString() + '\n';
			fwrite(buffer.c_str(),1,buffer.size(),appFile);
			for (int i=0; i<appDetailVec.size(); i++) {
				buffer = appDetailVec.at(i).getSaveString() + '\n';
				fwrite(buffer.c_str(),1,buffer.size(),appFile);
			}
			
			fclose(appFile);
		}
		void ReadData() {
			// 讀取 app 總檔
			FILE *appFile;
			string path = "./data/NewPattern/NewAppNameFile";
			appFile = fopen(path.c_str(), "r");
			
      int getLineSize = 1024;
			char getLine[getLineSize];
			string temp;
			int tempInt;
			
			int totalAppNum = 0;
			
			// 讀取數量
			if (fgets(getLine, getLineSize, appFile) != NULL) {
				string appNum = subCharArray(getLine, getLineSize, '|', 0);
				StringToNumber(appNum, &tempInt);
				totalAppNum = tempInt;
			} else {
				cout << "(error) AppManager::ReadData()"<<endl;
        fclose(appFile);
        return;
      }
			// 跳過一行
			if (fgets(getLine, getLineSize, appFile) != NULL) {
			} else {
				cout << "(error) AppManager::ReadData()"<<endl;
        fclose(appFile);
        return;
      }
			
			// 讀 app 資訊
			for (int i=0; i<totalAppNum; i++) {
				if (fgets(getLine, getLineSize, appFile) != NULL) {
					AppDetail tempAppDetail;
					tempAppDetail.setData(getLine, getLineSize);
					appDetailVec.push_back(tempAppDetail);
					
				} else {
					cout << "(error) AppManager::ReadData()"<<endl;
					fclose(appFile);
					return;
				}
			}
			
			fclose(appFile);
		}
};

int main(int argc, char** argv) {
	//{ ----- parameter initial
	string inputFolder("./data/");
	string file("fb5e43235974561d");
	vector<string> fileVec;
	for (int i=1; i<argc; i++) {
		char *temp;
		// input
		temp = strstr(argv[i], "input=");
		if (temp != NULL) {
			inputFolder = string(temp+6);
			cout << "input folder is \"" << inputFolder << "\"" <<endl;
			continue;
		}
		// file
		temp = strstr(argv[i], "file=");
		if (temp != NULL) {
			file = string(temp+5);
			cout << "file is \"" << file << "\"\n" <<endl;
			continue;
		}
	}//}
	
	if (argc >= 5) {
		StringToNumber(string(argv[3]), &START_SPACE_TIME);
		StringToNumber(string(argv[4]), &TRAINING_INTERVAL_DAY);
		StringToNumber(string(argv[5]), &TEST_INTERVAL_DAY);
		cout << START_SPACE_TIME << "\n" << TRAINING_INTERVAL_DAY << "\n" << TEST_INTERVAL_DAY <<endl;
	} else {
		cout << "(error) the parameters is not enough" <<endl;
		return 0;
	}
	
	//{ ----- files initial
  // 取出檔案，並判斷有沒有問題
  if (getdir(inputFolder, fileVec) == -1) {
    cout << "Error opening" << inputFolder << endl;
    return -1;
  }//}
	
	string fileName;
	for (auto onefile =fileVec.begin(); onefile!=fileVec.end(); onefile++) {
		if (*onefile == file) {
			
			fileName = inputFolder + *onefile;
			
			// New Pattern
			vector<TempEvent> tempEventVec;
			AppManager appManager;
			appManager.ReadData();
			
			// 讀檔
			//{ 開檔部分
      FILE *file;
			string path = fileName;
      file = fopen(path.c_str(),"r"); // "r" 讀檔而已
      if(file == NULL) {
        cout << "open \"" << fileName << "\" File fail" << endl ;
        return false;
			}//}
			
			//{ initial
      int line=0;
      int getLineSize = 4096;
			char getLine[getLineSize]; //}
			
			//{ 開始讀檔
			if (fgets(getLine, getLineSize, file) != NULL) {  // 跳過一行
          line++;
			} else {
				cout << "(error) NewPatternExpMain::Main(): Unable to read the start line. File: " << fileName <<endl;
        fclose(file);
        return false;
      }
      while (true) {
        if (fgets(getLine, getLineSize, file) != NULL) {  // 讀一行
          line++;
					// 檢察是不是最後一行
          if (strstr(getLine, "\"\"") != NULL) {
            break;
          }
					TempEvent tempE;
					
					//{ Name
					string appName = subCharArray(getLine, getLineSize, '"', 1);
					int appID = appManager.getAppID(&appName);
					tempE.namePoint = appID;//}
					
					//{ Date
					int tempInt;
					string tempStr;
					tempE.time.initial();
					string date = subCharArray(getLine, getLineSize, '"', 3) + '/';
					tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // test "2019/1/15" or "12/27/18"
					StringToNumber(tempStr, &tempInt);
					if (tempInt>2000) { // "2019/1/15" 類型
						tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // year
						StringToNumber(tempStr, &tempInt);
						tempE.time.year = tempInt;
						tempStr = subCharArray(date.c_str(), date.size(), '/', 1); // month
						StringToNumber(tempStr, &tempInt);
						tempE.time.month = tempInt;
						tempStr = subCharArray(date.c_str(), date.size(), '/', 2); // day
						StringToNumber(tempStr, &tempInt);
						tempE.time.day = tempInt;
					} else { // "12/27/18" 類型
						tempStr = subCharArray(date.c_str(), date.size(), '/', 2); // year
						StringToNumber(tempStr, &tempInt);
						tempE.time.year = 2000 + tempInt;
						tempStr = subCharArray(date.c_str(), date.size(), '/', 0); // month
						StringToNumber(tempStr, &tempInt);
						tempE.time.month = tempInt;
						tempStr = subCharArray(date.c_str(), date.size(), '/', 1); // day
						StringToNumber(tempStr, &tempInt);
						tempE.time.day = tempInt;
						
					}//}
					
					//{ Time
					string time = subCharArray(getLine, getLineSize, '"', 5) + ':';
					tempStr = subCharArray(time.c_str(), time.size(), ':', 0); // hour
					if (tempStr.substr(0,6) == "上午" || tempStr.substr(0,6) == "下午") { // "上午12:33:37" 類型
						StringToNumber(tempStr.substr(6), &tempInt);
						tempE.time.hour = (tempInt==12)? 0 : tempInt;
						if ((tempStr.substr(0,6) == "下午")) {
							tempE.time.hour += 12;
						}
					} else if (time.substr(time.size()-3,2) == "AM" || time.substr(time.size()-3,2) == "PM") { // "12:16:37 AM" 類型
						StringToNumber(tempStr, &tempInt);
						tempE.time.hour = (tempInt==12)? 0 : tempInt;
						if (time.substr(time.size()-3,2) == "PM") {
							tempE.time.hour += 12;
						}
					} else { // "00:10:20" 類型
						StringToNumber(tempStr, &tempInt);
						tempE.time.hour = tempInt;
					}
					tempStr = subCharArray(time.c_str(), time.size(), ':', 1); // minute
					StringToNumber(tempStr, &tempInt);
					tempE.time.minute = tempInt;
					tempStr = subCharArray(time.c_str(), time.size(), ':', 2); // second
					StringToNumber(tempStr.substr(0,2), &tempInt);
					tempE.time.second = tempInt;//}
					
					// Date Time test output
					//tempE.time.output(); cout << " real: " << date << " - " << time <<endl;
					
					// 裝入 tempEventVec
					tempEventVec.push_back(tempE);
        } else {
					cout << "(error) NewPatternExpMain::Main(): Unable to read the Part 2. File: " << fileName <<endl;
          fclose(file);
          return false;
        }
			}
			fclose(file);//}
			
			// 實驗所需的數據
			vector<Event> eventVec;
			vector<string> allAppNameVec = appManager.getAllAppNameVec();
			
			//{ 刪掉 ignore
			for(vector<TempEvent>::iterator iter = tempEventVec.begin(); iter != tempEventVec.end(); iter++) {
				AppManager::AppDetail *thisAppDetial = appManager.getAppDetail(iter->namePoint);
				if (thisAppDetial->ignore) {
					iter = tempEventVec.erase(iter);
				}
			}//}
			
			//{ 刪掉重複
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
			}//}
			
			//{ 裝入 eventVec
			for (int i = tempEventVec.size()-1; i>0/*最後一個不放入，怕出錯*/; i--) {
				Event event;
				AppManager::AppDetail *thisAppDetial = appManager.getAppDetail(tempEventVec.at(i).namePoint);
				AppManager::AppDetail *nextAppDetial = appManager.getAppDetail(tempEventVec.at(i-1).namePoint);
				
				//{ ----- time
				event.thisDate = &tempEventVec.at(i).time;
				event.nextDate = &tempEventVec.at(i-1).time;//}
				
				//{ ----- screen
				// this
				if (thisAppDetial->screenStatus) {
					event.isThisScreenOn = thisAppDetial->screenOn;
				} else {
					event.isThisScreenOn = true; // 有用 App 通常是亮的吧
				}
				// next
				if (nextAppDetial->screenStatus) {
					event.isNextScreenOn = nextAppDetial->screenOn;
				} else {
					event.isNextScreenOn = true; // 有用 App 通常是亮的吧
				}//}
				
				//{ ----- Case
				// 若是螢幕事件的話，不用放 case
				if (thisAppDetial->screenStatus) {
					// Do nothing
				} else {
					// AppInfo
					AppInfo *appInfo = new AppInfo;
					appInfo->namePoint = tempEventVec.at(i).namePoint;
					appInfo->pid = tempEventVec.at(i).namePoint; // 先隨便給
					appInfo->oom_score = 50; // 先隨便給
					
					// Event
					Event::Case mCase;
					mCase.namePoint = tempEventVec.at(i).namePoint;
					mCase.isCreat = true;
					mCase.currApp = NULL;
					mCase.nextApp = appInfo;
					
					event.caseVec.push_back(mCase);
				}//}
				
				// 放入
				eventVec.push_back(event);
			}//}
			
			/*for (int i=0; i<allAppNameVec.size(); i++) {
				cout << i << '\t' << allAppNameVec.at(i) <<endl;
			}*/
			
			// eventVec test output
			/*for (auto iter=eventVec.begin(); iter!=eventVec.end(); iter++) {
				iter->thisDate->output();
				if (iter->caseVec.size()>0) {
					cout << " | " << allAppNameVec.at(iter->caseVec.at(0).nextApp->namePoint) <<endl;
				} else {
					string screen = (iter->isThisScreenOn)? "Screen on" : "Screen off";
					cout << " | " << screen <<endl;
				}
			}*/
			appManager.SaveData();
			// 開始實驗
			mainOfExperiment(&eventVec, &allAppNameVec); //mason
		}
	}

  //cout << " ┌-----------------┐\n" <<
	//        " | experiment over |\n" <<
	//				" └-----------------┘"   <<endl;
  return 0;
}

int getdir(string folder, vector<string> &files) {
  // 創立資料夾指標
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(folder.c_str())) == NULL) {
    return -1;
  }
  // 如果dirent指標非空
  while((dirp = readdir(dp)) != NULL) {
    // 將資料夾和檔案名放入vector
    files.push_back(string(dirp->d_name));
  }
  // 關閉資料夾指標
  closedir(dp);
  return 0;
}
