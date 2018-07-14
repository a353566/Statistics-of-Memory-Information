#ifndef COLLECTION_FILE_HPP
#define COLLECTION_FILE_HPP

//#define COLLECTION_FILE_HPP_debug

#include <dirent.h>
#include <stdio.h>
#include <list>
#include <vector>
#include <string.h>
#include <iostream>

#include "DateTime.hpp"
#include "StringToNumber.hpp"

using namespace std;

/** class Point
 * 一個時間點的狀況
 * 主要是給 CollectionFile 來紀錄一連串時間用的
 * 但是目前 read.cpp::CollectionAllData 也有拿來用
 * 其中會有 apps[] 來記錄當下時間點的所有 app 的狀況
 */
class Point {
  public :
    /** class App
     * 單一 app 的所有資訊 
     * 其中名字是用 point 的方式指向 appNameVec 其中一個真正的名字
     * name|pid|TotalPss|oom_score|ground|oom_adj
     * TotalPss|oom_score|ground|oom_adj
     */
    class App {
      public :
				const static int NULL_DATA = -10000; // 下面參數 6 個沒東西的話是 NULL_DATA
        int namePoint;
        int pid;
        int totalPss;
        int oom_score; // oom_score 是 NULL_DATA 的話代表沒有資料
        int ground;    // ground 是 NULL_DATA 的話代表沒有資料
        int oom_adj;   // oom_adj 是 NULL_DATA 的話代表沒有資料
        
        App() {
          namePoint = NULL_DATA;
          pid = NULL_DATA;
          totalPss = NULL_DATA;
          oom_score = NULL_DATA;
          ground = NULL_DATA;
          oom_adj = NULL_DATA;
        }
        
        void output() {
          cout << "namePoint:" << namePoint
            << "\tpid:" << pid
            << "\ttotalPss:" << totalPss
            << "\toom_score:" << oom_score
            << "\tground:" << ground
            << "\toom_adj:" << oom_adj <<endl;
        };
        void output(const vector<string> *appNameVec) {
          cout << "name:" << (*appNameVec)[namePoint] << '\n'
            << "pid:" << pid
            << "\ttotalPss:" << totalPss
            << "\toom_score:" << oom_score
            << "\tground:" << ground
            << "\toom_adj:" << oom_adj <<endl;
        };
    };
    
    DateTime date;
    
    bool screen;
    int appNum;
    App *apps;
    
    bool getTime(string dateTime) {
      return date.setAllDateTime(dateTime);
    }
		
		App *getAppWithIndex(int index) {
			return (0<=index && index<appNum)? &(apps[index]) : NULL;
		}
		
		App *getAppWithNamePoint(int namePoint) {
			for (int i=0; i<appNum; i++) {
				if (apps[i].namePoint == namePoint) {
					return &(apps[i]);
				}
			}
			return NULL;
		}
};

class CollectionFile {
  // 一個檔案裡的所有資料 之後會用 CollectionAllData 再整個整合起來
  public :
    string phoneID;
    string folder;
    string fileName;
    DateTime date;
    list<Point> pattern;
    vector<string> appNameVec;
    
		CollectionFile() {}
		
		CollectionFile(string folder, string fileName) {
			this->folder = folder;
			this->fileName = fileName;
		}
		
    // 開啟檔案，並開始讀檔
    bool openFileAndRead() {
      //{ 開檔部分
      FILE *file;
			string path = folder + fileName;
      file = fopen(path.c_str(),"r"); // "r" 讀檔而已
      if(file == NULL) {
        cout << "open \"" << fileName << "\" File fail" << endl ;
        return false;
			}//}
      
			//{ initial
      int line=0;
      int getLineSize = 4096;
			char getLine[getLineSize]; //}
      
      //{ 取得 phoneID 以及讀到 "----------" 的話就先往下 (ex: phoneID:fb5e43235974561d)
      while (true) {
        if (fgets(getLine, getLineSize, file) != NULL) {  // 讀一行
          line++;
          if (strstr(getLine, "----------") != NULL) {
            break;
          }
          char* temp = strstr(getLine, "phoneID:");
          if (temp != NULL) {
            phoneID = string(temp+8);
          }
        } else {
					cout << "(error) CollectionFile::openFileAndRead(): Unable to read the start line. File: " << fileName <<endl;
          fclose(file);
          return false;
        }
			}//}
      
			//{ 順著存每一個 Point
      Point lastPoint;
      while (true) {
			  //{ initial
        Point thisPoint;
        bool isBigScan;
        bool appDataGood = true; //}
				
        /** 接下來是固定格式
				 *  1.時間
				 *  2.ProcessRecord : (a)Big or Small (b)procNum (c)AppData
				 *  3.sensor data
				 */
        //{ ----- 1. 時間 (ex: 2018-02-24_00.49.27)
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          // 檢查是不是沒有資料了
          if (strstr(getLine,"over")) {
#ifdef COLLECTION_FILE_HPP_debug
            cout << "fileName:" << fileName << " over data" <<endl;
#endif
            break;
          }
          if (!thisPoint.getTime(string(getLine))) {
            cout << "(error) CollectionFile::openFileAndRead() time: " << getLine <<endl;
            cout << "        fileName: " << fileName <<endl;
            break;
          }
        } else {
          break;
        }//}
        
				//  ----- 2. ProcessRecord
        //{ === 2(a)Big or Small (ex: ProcessRecord:Big or ProcessRecord:Small)
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          // 檢查類型 "Big" or "Small"
          if (strstr(getLine,"Big") != NULL) {
            isBigScan = true;
          } else if (strstr(getLine,"Small") != NULL) {
            isBigScan = false;
          } else {
            cout << "(error) CollectionFile::openFileAndRead() ProcessRecord: " << getLine <<endl;
						cout << "        fileName: " << fileName <<endl;
            break;
          }
        } else {
          break;
        }//}
        //{ === 2(b)procNum 接下來紀錄 APP 的數量 (es: procNum:19)
        if (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          string tempStr(strstr(getLine,"procNum:"));
          int tempN;
          if (StringToNumber(tempStr.substr(8,tempStr.size() - 9), &tempN)) {
            thisPoint.appNum = tempN;
          } else {
            cout << "(error) CollectionFile::openFileAndRead() procNum: " << getLine <<endl;
						cout << "        fileName: " << fileName <<endl;
            break;
          }
        } else {
          break;
        }//}
        //{ === 2(c)AppData 讀取 each App 並且會依照 Big or Small 有不同的處理方式
        thisPoint.apps = new Point::App[thisPoint.appNum];
        for (int getAppNum=0; getAppNum<thisPoint.appNum; getAppNum++) {
          if (fgets(getLine, getLineSize, file) != NULL) {
            line++;
            Point::App tempApp;
            int index = 0;
						//{ == Big or Small collection
            // Big collection : name pid 要從中取出來
						// Small collection : name pid 從是上一個取 (lastPoint)
            if (isBigScan) {
              //{ -- get name
              string appName = subCharArray(getLine, getLineSize, '|', index++);
              if (appName.size()!=0) {
                // 判斷 appName 在 appNameVec 中是第幾個並給 tempApp.namePoint
                int appNamePoint = 0;
                bool isFindName = false;
                for (int i=0; i < appNameVec.size(); i++) {
                  if (appNameVec[i] != appName) {
                    appNamePoint++;
                  } else {
                    isFindName = true;
                    break;
                  }
                }
                tempApp.namePoint = appNamePoint;
                // 沒有的話新增一個
                if (!isFindName) {
                  appNameVec.push_back(appName);
                }
              } else {
                cout << "(error) CollectionFile::openFileAndRead() appName\n" << getLine <<endl;
								cout << "        fileName: " << fileName <<endl;
                appDataGood = false;
                break;
              }//}
              
              //{ -- get pid
              string appPid = subCharArray(getLine, getLineSize, '|', index++);
              if (appPid.size()!=0) {
                if (!StringToNumber(appPid, &tempApp.pid)) {
                  cout << "(error) CollectionFile::openFileAndRead() appPid: " << appPid <<endl;
									cout << "        fileName: " << fileName <<endl;
                  appDataGood = false;
                }
              } else {
                cout << "(error) CollectionFile::openFileAndRead() appPid:NULL \n" << getLine <<endl;
								cout << "        fileName: " << fileName <<endl;
                appDataGood = false;
                break;
              }//}
            } else {
              tempApp.namePoint = lastPoint.apps[getAppNum].namePoint;
              tempApp.pid = lastPoint.apps[getAppNum].pid;
            }//}
            
            //{ == get TotalPss
            string appTotalPss = subCharArray(getLine, getLineSize, '|', index++);
            if (appTotalPss.size()!=0) {
              if (!StringToNumber(appTotalPss, &tempApp.totalPss)) {
                // 可能只是 "手機沒有取得資料"
                if (appTotalPss == "null") {
                  tempApp.totalPss = Point::App::NULL_DATA;
                } else {
                  tempApp.totalPss = Point::App::NULL_DATA;
                  cout << "(error) CollectionFile::openFileAndRead() appTotalPss: " << appTotalPss <<endl;
									cout << "        fileName: " << fileName <<endl;
                  appDataGood = false;
                }
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appTotalPss.size():0 getLine:(last line)\n" << getLine <<endl;
							cout << "        fileName: " << fileName <<endl;
              appDataGood = false;
              break;
            }//}
            
            //{ == get oom_score
            string appOomScore = subCharArray(getLine, getLineSize, '|', index++);
            if (appOomScore.size()!=0) {
              if (!StringToNumber(appOomScore, &tempApp.oom_score)) {
                tempApp.oom_score = Point::App::NULL_DATA;
                cout << "(error) CollectionFile::openFileAndRead() appOomScore: " << appOomScore <<endl;
								cout << "        fileName: " << fileName <<endl;
                appDataGood = false;
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appOomScore.size():0 getLine:(last line)\n" << getLine <<endl;
							cout << "        fileName: " << fileName <<endl;
              appDataGood = false;
              break;
            }//}
            
            //{ == get ground
            string appGround = subCharArray(getLine, getLineSize, '|', index++);
            if (appGround.size()!=0) {
              if (!StringToNumber(appGround, &tempApp.ground)) {
                tempApp.ground = Point::App::NULL_DATA;
                cout << "(error) CollectionFile::openFileAndRead() appGround: " << appGround <<endl;
                cout << "        fileName: " << fileName <<endl;
								appDataGood = false;
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appGround.size():0 getLine:(last line)\n" << getLine <<endl;
							cout << "        fileName: " << fileName <<endl;
              appDataGood = false;
              break;
            }//}
            
            //{ == get oom_adj
            string appOomAdj = subCharArray(getLine, getLineSize, '|', index++);
            if (appOomAdj.size()!=0) {
              if (!StringToNumber(appOomAdj, &tempApp.oom_adj)) {
                // 可能只是 "手機沒有取得資料"
                if (appTotalPss == "null") {
                  tempApp.oom_adj = Point::App::NULL_DATA;
                } else {
                  tempApp.oom_adj = Point::App::NULL_DATA;
                  cout << "(error) CollectionFile::openFileAndRead() appOomAdj: " << appTotalPss <<endl;
                  cout << "        fileName: " << fileName <<endl;
									appDataGood = false;
                }
              }
            } else {
              cout << "(error) CollectionFile::openFileAndRead() appOomAdj.size():0 getLine:(last line)\n" << getLine <<endl;
              cout << "        fileName: " << fileName <<endl;
							appDataGood = false;
              break;
						}//}
            
            // 將 APP 放入 OneShot 中
            thisPoint.apps[getAppNum] = tempApp;
          } else {
						// 到這邊的話代表 App 數量不對
            cout << "(error) CollectionFile::openFileAndRead() procNum not enough"<<endl;
            cout << "        need " << thisPoint.appNum << " app, but we just get " << getAppNum << " app & fileName: " << fileName<<endl;
						appDataGood = false;
						thisPoint.appNum = getAppNum;
            break;
          }
				} //} 讀取 APP 詳細資料 loop
        
        /** ----- 3. sensor data
				 *  接下來為其他資料的讀取
         *  主要是將 ':' 符號前的資料抓出來比對
         *  並且做相對應的事情
         */
        while (fgets(getLine, getLineSize, file) != NULL) {
          line++;
          //{ === 發現和 '----------' 一樣的話就換下一筆資料
          if (strstr(getLine,"----------") != NULL) {
            break;
          }//}
          //{ === 抓出指標物
          string indicate = subCharArray(getLine, getLineSize, ':', 0);
          if (indicate == "Screen") {
            string screen(getLine + 7);
            if (screen.substr(0,2) == "On") {
              thisPoint.screen = true;
            } else if (screen.substr(0,3) == "Off") {
              thisPoint.screen = false;
            } else {
              cout << "(error) CollectionFile::openFileAndRead() Screen:"<< screen <<endl;
							cout << "        fileName: " << fileName <<endl;
              appDataGood = false;
            }
          } else if (indicate == "Battery") { // ===================== bug
            
          } else if (indicate == "WiFi") {
            
          } else if (indicate == "G-sensor") {
						// 讀到結束為止 因為後面世界長
						char c;
						do {
							c = fgetc (file);
						} while (c != '\0' && c != '\n' && c != EOF);
          } else if (indicate == "Location") {
            
          } else if (indicate == "WiFiSensor") {
            
          } else if (indicate == "WiFiConnect") {
            // 後面還有資料
            if (fgets(getLine, getLineSize, file) != NULL) {
              line++;
            } else {
              break;
            }
          } else if (indicate == "scanTime") {
            
          } else { // 都沒有抓出的話
            cout << "(error) CollectionFile::openFileAndRead() indicate:"<< indicate << "fileName:" << fileName << " line:" << line <<endl;
            cout << "        fileName: " << fileName <<endl;
          }//}
        }
        lastPoint = thisPoint;
        pattern.push_back(thisPoint);
      } //} end 順著存每一個 Point
      
      fclose(file);
      return true;
    };

    // string to date 取得日期的 function
    bool setAllDateTime() { // 事先設定好 fileName 可以呼叫這個
      return date.setAllDateTime(fileName);
    };
    bool setAllDateTime(string fileName) {
      this->fileName = fileName;
      return date.setAllDateTime(fileName);
    };
    
    // 比時間哪個比較新舊 (ps: 1:2:4 會大於 1:2:3)
    bool operator > (const CollectionFile &otherFile)const {
      return date > otherFile.date;
    };
    bool operator < (const CollectionFile &otherFile)const {
      return date < otherFile.date;
    };
    
  private:
		/** 剪取中間某一段資料
		 *  charArray : 整段資料 (ex: name|15989|null|658|0|9|)
		 *  size : charArray 長度 (ex: 25)
		 *  subUnit : 要剪的字元 (ex: '|')
		 *  whichData : 要剪的區段 (ex: 1 => return 15989)
		 *  return : 0    1     2    3   4 5  (ps: 以上面為例)
		 *           name|15989|null|658|0|9|
		 */
    string subCharArray(const char* charArray, int size, char subUnit, int whichData) {
      if (whichData<0)  // 防呆
        return string("");
      
      int head = 0;
      int end = 0;
      for (int i=0; i<size; i++) {
        if (charArray[i] == subUnit) { // 尋找相同字元
          whichData--;
          if (whichData == 0) { // 找到頭了
            head = i+1;
          } else if (whichData == -1) { // 確定找完了
            end = i;
            if (head < end)  // 怕裡面沒有資料
							return string(charArray + head, end - head);
            else
              return string("");
          }
        }
      }
      
      return string("");
    }
};
#endif /* COLLECTION_FILE_HPP */