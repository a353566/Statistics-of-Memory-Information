#include <dirent.h>
#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>

#define PIN_TESTEND_ON_DATAEND
#define TRAINING_INTERVAL_DAY 21
#define TEST_INTERVAL_DAY 7

//#define EXPERIMENT_debug_addOom_adjStat

//#define EXPERIMENT_debug_oomAdj_less_than_0
//#define EXPERIMENT_debug_oomAdj_rate_onEachApp
//#define EXPERIMENT_debug_oomAdj_statistics
//#define EXPERIMENT_debug_Print_Event  // point AppName out
//#define EXPERIMENT_debug_IntervalTime

// ----- experiment part -----
#define EXPERIMENT_GSP_normal_part
//#define EXPERIMENT_GSP_const_level_part
//#define EXPERIMENT_GSP_multiply_of_level_part
//#define EXPERIMENT_GSP_power_of_level_part
//#define EXPERIMENT_LRU_part
//#define EXPERIMENT_MFU_part

#include "include/DateTime.hpp"
#include "include/StringToNumber.hpp"
#include "include/CollectionFile.hpp"
#include "include/SequentialPatternsMining/GSP.hpp"
#include "include/SequentialPatternsMining/GSP_Predict.hpp"
using namespace std;

int getdir(string dir, vector<string> &files);   // 取得資料夾中檔案的方法

class Event {
  public :
    class Case {
      public :
        int namePoint;
        bool isCreat;   // true 的話 只有 nextApp 有東西
        Point::App *thisApp;
        Point::App *nextApp;
        Case() {
          isCreat = false;
        };
    };
    
		// lastTime "useApp" thisDate nextDate
    DateTime *thisDate;		// 紀錄APP的時間點
    DateTime *nextDate;		// 下次紀錄的時間點
    vector<Case> caseVec;
    bool isThisScreenOn;
    bool isNextScreenOn;
    Event() {
      isThisScreenOn = false;
      isNextScreenOn = false;
    }

    /** 將重複的 app 刪掉
		 *  if appear : 1st 保留 oom_score 比較小的
		 *              2nd 保留 appPid 比較小的
		 *  起因 : 因為 Cytus 會有兩個，所以要特別刪掉一個
		 */
    bool sortOut() {
      bool dataGood = true;
      for (int i=0; i<int(caseVec.size())-1; i++) {
        int namePoint = caseVec.at(i).namePoint;
        for (int j=i+1; j<caseVec.size(); j++) {
          // if appear ......
          if (namePoint == caseVec.at(j).namePoint) {
            /*if (dataGood) {
              cout << "-------" << caseVec.size() <<endl;
              for (int k=0; k<caseVec.size(); k++) {
                cout << caseVec.at(k).nextApp->oom_score << " : " << caseVec.at(k).nextApp->namePoint <<endl;
              }
            }*/

            int front = caseVec.at(i).nextApp->oom_score;
            int behind = caseVec.at(j).nextApp->oom_score;
            if (front < behind) {
              caseVec.erase(caseVec.begin()+j);
              j--;
            } else if (front > behind) {
              caseVec.erase(caseVec.begin()+i);
              i--;
              break;
            } else {
              front = caseVec.at(i).nextApp->pid;
              behind = caseVec.at(j).nextApp->pid;
              if (front < behind) {
                caseVec.erase(caseVec.begin()+j);
                j--;
              } else if (front > behind) {
                caseVec.erase(caseVec.begin()+i);
                i--;
                break;
              } else { // 都一樣的話 丟後面的 雖然不太可能發生
                caseVec.erase(caseVec.begin()+j);
                j--;
              }
            }
            dataGood = false;
          }
        }
      }
      /*if (!dataGood) {
        cout << "new--" << caseVec.size() <<endl;
        for (int k=0; k<caseVec.size(); k++) {
          cout << caseVec.at(k).nextApp->oom_score << " : " << caseVec.at(k).nextApp->namePoint <<endl;
        }
      }*/
      return dataGood;
    }
};

class CollectionAllData {
  public :
    // 單一APP的詳細資料，主要是用於分析，對NC檔沒有幫助
    class AppDetail {
      public :
        string appName;
        int findTimes;
        double findRate;
        int oom_adjStat[41]; // 從-20~20 (不過目前只有-17~16)
        
        AppDetail(string appName) {
          this->appName = appName;
          findTimes = 0;
          findRate = 0;
          for (int i=0; i<41; i++) {
            oom_adjStat[i]=0;
          }
        };
        
        bool addOom_adjStat(int oom_adj) {
          if ( -17<=oom_adj && oom_adj<=16 ) {
            oom_adjStat[oom_adj+20]++;
            return true;
          } else if ( -20<=oom_adj && oom_adj<=20 ) {
            oom_adjStat[oom_adj+20]++;
#ifdef EXPERIMENT_debug_addOom_adjStat
            cout << "oom_adj out of value : " << oom_adj << endl;
#endif
            return true;
          } else {
#ifdef EXPERIMENT_debug_addOom_adjStat
            cout << "oom_adj out of value : " << oom_adj << endl;
#endif
            return false;
          }
        };
        int getOom_adjStat(int oom_adj) {
          if ( -20<=oom_adj && oom_adj<=20 ) {
            return oom_adjStat[oom_adj+20];
          } else {
            return -1;
          }
        };
    };
    
    DateTime beginDate;
    DateTime endDate;
    vector<string> allAppNameVec;       // 用 collecFileVec 的資料收集所有的 app's name
    vector<Point> allPatternVec;        // 所有 pattern
    vector<AppDetail> allAppDetailVec;  // 所有 app 的詳細資料
    /** allEventVec 主要是放入了 發生過的事件 並依序放好
     *  其中的 Event 是使用者剛好切換 APP 或是開關螢幕也會知道
     */
    vector<Event> allEventVec; 
    
    CollectionAllData() {};
    
    void collection(vector<CollectionFile> *collecFileVec) {
      //{ 收集所有的 app's name 用來之後編號
      for (int i=0; i<collecFileVec->size(); i++) {
        for(list<Point>::iterator oneShot = (*collecFileVec)[i].pattern.begin();
            oneShot!=(*collecFileVec)[i].pattern.end(); oneShot++) 
        {
          addonePoint(*oneShot, &(*collecFileVec)[i].appNameVec);
        }
      }//}
			
      //{ beginDate, endDate 設定
      beginDate = allPatternVec.front().date;
      endDate = allPatternVec.back().date;//}
      
      //{ 從 allPatternVec 中統計 app 的出現次數和 each oom_adj 的次數
      for (int i=0; i<allAppNameVec.size(); i++) {
        AppDetail newAppDetail(allAppNameVec[i]); // 放入名字
				// 找所有 allPatternVec::app 中 兩者 point 一樣的 (表示有找到)
        for (int j=0; j<allPatternVec.size(); j++) {
          for (int k=0; k<allPatternVec[j].appNum; k++) {
            if (i==allPatternVec[j].apps[k].namePoint) {
              newAppDetail.findTimes++;
              newAppDetail.addOom_adjStat(allPatternVec[j].apps[k].oom_adj);
              break;
            }
          }
        }
        newAppDetail.findRate = ((double)newAppDetail.findTimes)/allPatternVec.size();
        allAppDetailVec.push_back(newAppDetail); // 放入 allAppDetailVec
      }//}
			
      /** 有些要跳過的 app & 不需要的直接跳過的 app
			 *  不需要的有 : oom_adj 沒出現過 0，表示沒有在前景出現過
			 *               為了收集資料的 app
			 */ //{
      bool unneededAppArray[allAppDetailVec.size()];
      for (int i=0; i<allAppDetailVec.size(); i++) {
        // initial
        unneededAppArray[i] = false;
				
        // oom_adj 沒出現過 0
        if (allAppDetailVec[i].getOom_adjStat(0)==0) {
          unneededAppArray[i] = true;
          continue;
        }
				
        // 自己的收集 App
        int temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:Record",0);
        if(0<=temp) {
          unneededAppArray[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:Recover",0);
        if(0<=temp) {
          unneededAppArray[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo:remote",0);
        if(0<=temp) {
          unneededAppArray[i] = true;
          continue;
        }
        temp = allAppDetailVec[i].appName.find("com.mason.memoryinfo",0);
        if(0<=temp) {
          unneededAppArray[i] = true;
          continue;
        }
      }//}
			
      // 看 oom_adj 有沒有 0 以下的數值
#ifdef EXPERIMENT_debug_oomAdj_less_than_0
			{ bool isHave = false;
				for (int i=0; i<allAppDetailVec.size(); i++) {
					for (int j=-20; j<0; j++) {
						if (allAppDetailVec[i].getOom_adjStat(j)!=0) {
							if (!isHave) {
								cout << "    =============== oom_adj <0 ================" <<endl;
								cout << "   i  oom_adj  Times  Name " <<endl;
								isHave = true;
							}
							printf("%4d %8d  %5d  %s\n", i, j, allAppDetailVec[i].getOom_adjStat(j), allAppDetailVec[i].appName.c_str());
						}
					}
				}
			}
#endif
			
			/**  顯示 each app, oom_adj 零以上的資訊
			 *   print : i findRate(%)  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 (%) App's name
			 *           0|        49|  0|   |   |   |  1|   |  0| 11|   | 29|   | 41|   |   |   | 18|   |---|android.process.media
			 *   i : 第幾個 app
			 *   findRate : 全部 pattern 出現過幾次
			 *   0~16 : oom_adj 的比例 (佔自己出現過的總比例)
			 *   app name
			 */
#ifdef EXPERIMENT_debug_oomAdj_rate_onEachApp
      { cout << "    ============== 0 <= oom_adj ==============" <<endl;
        printf("  i findRate(%%)");
        for (int j=0; j<=16; j++) {
          printf(" %2d ", j);
        }
        cout << "(%) App's name" <<endl;
        
        // 主要輸出 app Detail 前面會先有過濾的動作，可以只看到想看到的訊息
        for (int i=0; i<allAppDetailVec.size(); i++) {
          if (unneededAppArray[i])  continue; // 略過不需要的 app
          
          //{ 用來只找特定的 app
          //int temp = allAppDetailVec[i].appName.find(":", 0); // 看名字中有沒有 ":"
          //if (temp<0)  continue;//}
          
          printf("%3d|      %4.0f|", i, allAppDetailVec[i].findRate*100);
          for (int j=0; j<=16; j++) {
            if (allAppDetailVec[i].getOom_adjStat(j)==0) {
              printf("   |");
              continue;
            }
            
            double oom_adjRate = ((double)allAppDetailVec[i].getOom_adjStat(j))/allAppDetailVec[i].findTimes;
            printf("%3.0lf|", 100*oom_adjRate);
            if (j==3 && allAppDetailVec[i].getOom_adjStat(j)!=0) {
              printf("%3d%4f%%", i, allAppDetailVec[i].findRate);
              cout << " : " << allAppDetailVec[i].appName <<endl;
            }
          }
          cout << "---|" << allAppDetailVec[i].appName <<endl;
        }
      }
#endif
			
      // 整理成 allEventVec
      makeAllEventVec(unneededAppArray);
			
			// 觀察間隔時間 超過此時間(maxIntervalTime) 將會顯示
#ifdef EXPERIMENT_debug_IntervalTime
			DateTime maxIntervalTime;
			maxIntervalTime.initial();
			maxIntervalTime.minute = 5;
			int maxIntervalTimeNum = 0;
			for (int i=0; i<allEventVec.size(); i++) {
				DateTime intervalTime = *allEventVec[i].nextDate - *allEventVec[i].thisDate;
				if (intervalTime>maxIntervalTime) {
					// 有必要在輸出
					if (maxIntervalTimeNum == 0) {
						cout << "    ============== Interval Time ==============" <<endl;
						cout << "Interval Time over ";
						maxIntervalTime.output();
						cout << endl;
						cout << "  #:    i   interval time  this time              next time           screen state             " <<endl;
					}
					printf("%3d:", ++maxIntervalTimeNum);
					printf("%5d :   ", i);
					intervalTime.output();
					printf("     ");
					allEventVec[i].thisDate->output();
					printf(" -> ");
					allEventVec[i].nextDate->output();
					
					//printf(" screen state : ");
					printf(" %s -> " ,allEventVec[i].isThisScreenOn ? " on":"off");
					printf(" %s" ,allEventVec[i].isNextScreenOn ? " on":"off");
					
					printf("\n");
				}
			}
#endif
    }
		
		/** 將 allPatternVec 整理後裝到這裡 allEventVec
		 *  查看 allPatternVec 中 oom_adj 發生改變的狀況
		 *  主要是檢查前後兩個 Point (currPoint, nextPoint) 中的 apps 的變化
		 *  有重大變化則加到 allPatternVec : 
		 *   | 1. reuse app : 檢查 oom_adj -> 0
		 *   | 2. creat app : nextPoint 才出現
		 *   | 3. screen status change : 螢幕開關
		 *  unneededAppArray : 不會用到的 APP 陣列
		 */
    void makeAllEventVec(bool *unneededAppArray) {
      int AppsChangeTimes = 0;	// Point 前後 apps 改變次數
      //{ 檢查前後兩個 Point (currPoint, nextPoint) 中 apps 的變化
			vector<Point>::iterator currPoint = allPatternVec.begin();
			vector<Point>::iterator nextPoint = allPatternVec.begin(); nextPoint++;
      for (; nextPoint!=allPatternVec.end(); currPoint++, nextPoint++) {
				/** connect two point (currPoint, nextPoint)
				 *   1. same pid and name
				 *   2. same name, but difference pid
				 *   3. other
				 */
				//{ initial
        Point::App *currApps = currPoint->apps;
        int currAppsNum = currPoint->appNum;
        Point::App *nextApps = nextPoint->apps;
        int nextAppsNum = nextPoint->appNum;
				// 紀錄是不是所有 current & next 的 App 都有對應到 and build
        bool *currAppsMatch = buildMatchList(&*currPoint, unneededAppArray);
        bool *nextAppsMatch = buildMatchList(&*nextPoint, unneededAppArray);//}
				
				// ----- 1. same pid and name
				vector<pair<int, int> > PNpairs = buildSamePNpairs(&*currPoint, &*nextPoint, currAppsMatch, nextAppsMatch);
				
				// ----- 2. same name, but difference pid
				vector<pair<int, int> > Ppairs = buildSamePpairs(&*currPoint, &*nextPoint, currAppsMatch, nextAppsMatch);
				
				// ----- 3. other (在 Match 表格裡)
				
				/** 分析上面整理出來的配對
				 *  1. reuse App part : analysis PNpairs
				 *       method : oom_adj -> 0 , record it
				 *  2. creat App : analysis Next Match List
				 *       method : next app oom_adj is 0 , record it
				 *  3. reuse App but pid is difference : analysis Ppairs
				 *       method : oom_adj -> 0 , record it
				 *  4. screen status change
				 */
				//{ initial
        Event analysisEvent;
        bool isOom_adjCgToZero; // 看 oom_adj 有沒有變成0
        bool isCreatNewApp;     // 有新開的APP
        bool isScreenChange;    // 螢幕是否變化
				vector<pair<int, int> >::iterator pairIter;
				//}
				
				//{ ----- 1. reuse App part : analysis PNpairs
				isOom_adjCgToZero = false;
				for (pairIter = PNpairs.begin(); pairIter != PNpairs.end(); pairIter++) {
					Point::App *currApp = currPoint->getAppWithIndex(pairIter->first);
					Point::App *nextApp = nextPoint->getAppWithIndex(pairIter->second);
					if (currApp->oom_adj != nextApp->oom_adj && nextApp->oom_adj == 0) {
						isOom_adjCgToZero = true;
						Event::Case newCase;  // 收集 case
						newCase.namePoint = currApp->namePoint;
						newCase.isCreat = false;
						newCase.thisApp = currApp;
						newCase.nextApp = nextApp;
						analysisEvent.caseVec.push_back(newCase);
					}
				}//}
				
				//{ ----- 2. creat App : analysis Next Match List
				isCreatNewApp = false;
        for (int i=0; i<nextAppsNum; i++) {
          if (!nextAppsMatch[i] && nextApps[i].oom_adj==0) {
						isCreatNewApp = true;
						Event::Case newCase; // 收集 case
						newCase.namePoint = nextApps[i].namePoint;
						newCase.isCreat = true;
						newCase.nextApp = &(nextApps[i]);
						analysisEvent.caseVec.push_back(newCase);
          }
        }//}
				
				//{ ----- 3. reuse App but pid is difference : analysis Ppairs
				//map<int, int> SNDPnamePointMap;
				for (pairIter = Ppairs.begin(); pairIter != Ppairs.end(); pairIter++) {
					Point::App *currApp = currPoint->getAppWithIndex(pairIter->first);
					Point::App *nextApp = nextPoint->getAppWithIndex(pairIter->second);
					//printf("SNDPnamePointMap:%1d: %6d:%3d | %6d:%3d | %3d %s\n", ++SNDPnamePointMap[currApp->namePoint], currApp->pid, currApp->oom_adj, nextApp->pid, nextApp->oom_adj, currApp->namePoint, allAppNameVec[currApp->namePoint].c_str());
					
					if (currApp->oom_adj != nextApp->oom_adj && nextApp->oom_adj == 0) {isOom_adjCgToZero = true;
						Event::Case newCase;  // 收集 case
						newCase.namePoint = currApp->namePoint;
						newCase.isCreat = false;
						newCase.thisApp = currApp;
						newCase.nextApp = nextApp;
						analysisEvent.caseVec.push_back(newCase);
					}
				}//}
				
				//  ----- 4. screen status change
				isScreenChange = (currPoint->screen != nextPoint->screen);	
				
				//{ final 有上述改變的話則記錄
        if (isOom_adjCgToZero || isCreatNewApp || isScreenChange) { // 看 oom_adj 有沒有變成0 或是有新開的APP 以及螢幕是否有改變
          analysisEvent.thisDate = &currPoint->date;
          analysisEvent.nextDate = &nextPoint->date;
          analysisEvent.isThisScreenOn = currPoint->screen;
          analysisEvent.isNextScreenOn = nextPoint->screen;
          allEventVec.push_back(analysisEvent);
        }//}
				
        //{ (後面輸出用) Point 前後有改變的話 AppsChangeTimes++
				bool isChange = false;
				for (int i=0; i<currAppsNum && !isChange; i++) {
					if (!currAppsMatch[i]) {
						isChange = true;
					}
				}
				for (int i=0; i<nextAppsNum && !isChange; i++) {
					if (!nextAppsMatch[i]) {
						isChange = true;
					}
				}
        if (isChange) {
          AppsChangeTimes++;
				}//}
			}//}
			
      //{ allEventVec 中的 app 可能出現重複的 app (收集時就有問題)
			for (int i=0; i<allEventVec.size(); i++) {
				allEventVec.at(i).sortOut();
			}//}
      
      // 輸出 each oom_adj 統計
#ifdef EXPERIMENT_debug_oomAdj_statistics
			cout << "    ================= oom_adj =================" <<endl;
			cout << "Change Times : " << AppsChangeTimes <<endl;
			cout << "oom_adj change(creat) to zero times : " << allEventVec.size() <<endl;
			
			cout << "oom_adj change detail : " <<endl;
			cout << "   No    1    2    3    4    5    6    7    8  other" <<endl;
			int chToZero[10] = {0};
			for (int i=0; i<allEventVec.size(); i++) {
				if (allEventVec[i].caseVec.size()<9)
					chToZero[allEventVec[i].caseVec.size()]++;
				else
					chToZero[9]++;
			}
			for (int i=0; i<10; i++) {
				printf("%5d", chToZero[i]);
			}
			cout<<endl;
#endif
		}
    
		/** 建立表格，並去除 unneededAppArray 中的 app
		 *  point : 從中取得 apps 並對此建立
		 *  unneededAppArray : 不需要的 app list
		 *  return : Match List
		 */
		bool *buildMatchList(const Point *point, bool *unneededAppArray) {
			bool *matchList = new bool[point->appNum];
			for (int i=0; i<point->appNum; i++) {
				matchList[i] = unneededAppArray[point->apps[i].namePoint];
			}
			return matchList;
		}
		
		/** 建立 same pid & name pair
		 *  currPoint, nextPoint : 目前和下次的 Point
		 *  currAppsMatch, nextAppsMatch : 有無配對的 list
		 *  return : 回傳配對表，且更改 Match list
		 */
		vector<pair<int, int> > buildSamePNpairs(const Point *currPoint, const Point *nextPoint, bool *currAppsMatch, bool *nextAppsMatch) {
			//{ initial
			vector<pair<int, int> > PNpairs;
			Point::App *currApps = currPoint->apps;
			int currAppsNum = currPoint->appNum;
			Point::App *nextApps = nextPoint->apps;
			int nextAppsNum = nextPoint->appNum;//}
			
			//{ find same pid & namePoint
			for (int i=0; i<currAppsNum; i++) {
				if (currAppsMatch[i]) { continue; }
				
				for (int j=0; j<nextAppsNum; j++) {
					if (nextAppsMatch[j]) { continue; }
					
					if (currApps[i].pid == nextApps[j].pid && currApps[i].namePoint == nextApps[j].namePoint) {
						currAppsMatch[i] = true;
						nextAppsMatch[j] = true;
						PNpairs.push_back(make_pair(i, j));
						break;
					}
				}
			}//}
			
			return PNpairs;
		}
		
		/** 建立 same pid but difference name pair
		 *  currApps, nextApps : 目前和下次的 Point
		 *  currAppsMatch, nextAppsMatch : 有無配對的 list (應該已經去掉不少了)
		 *  return : 回傳配對表，且更改 Match list
		 */
		vector<pair<int, int> > buildSamePpairs(const Point *currPoint, const Point *nextPoint, bool *currAppsMatch, bool *nextAppsMatch) {
			//{ initial
			vector<pair<int, int> > Ppairs;
			Point::App *currApps = currPoint->apps;
			int currAppsNum = currPoint->appNum;
			Point::App *nextApps = nextPoint->apps;
			int nextAppsNum = nextPoint->appNum;//}
			
			//{ find same pid & namePoint
			for (int i=0; i<currAppsNum; i++) {
				if (currAppsMatch[i]) { continue; }
				
				for (int j=0; j<nextAppsNum; j++) {
					if (nextAppsMatch[j]) { continue; }
					
					if (currApps[i].namePoint == nextApps[j].namePoint) {
						currAppsMatch[i] = true;
						nextAppsMatch[j] = true;
						Ppairs.push_back(make_pair(i, j));
						break;
					}
				}
			}//}
			
			return Ppairs;
		}
		
		/** 加入 oneShot 到 allPatternVec 中，並更改 name index
		 *  oneShot : 要加入的 Point
		 *  oneAppNameVec : 舊的名字序列
		 */
    void addonePoint(Point oneShot, const vector<string> *oneAppNameVec) {
      for (int i=0; i<oneShot.appNum; i++) {
        findAppNameIndex(&oneShot.apps[i], &(*oneAppNameVec)[oneShot.apps[i].namePoint]);
      }
      allPatternVec.push_back(oneShot);
    }
    
		/** 回傳 appName 在 allAppNameVec 中是哪一個 index
		 *  並加到 app 中和回傳
		 *  app : 要被改寫的 APP
		 *  appName : app name
		 *  return : appName's index in allAppNameVec
		 */
    int findAppNameIndex(Point::App *app, const string *appName) {
      // 先檢查 allAppNameVec 中有沒有此名字
      for (int i=0; i<allAppNameVec.size(); i++) {
        // 找到後直接回傳新的 namePoint
        if (*appName == allAppNameVec[i]) {
          app->namePoint = i;
          return i;
        }
      }
      
      // 沒找到則將其加進去，並取得新的 index
      allAppNameVec.push_back(*appName);
      app->namePoint = allAppNameVec.size()-1;
      return app->namePoint;
    }
};

class DataMining {
  public :
    string screen_turn_on;
    string screen_turn_off;
    string screen_long_off;
    int EPscreen_turn_on;
    int EPscreen_turn_off;
    int EPscreen_long_off;
    DateTime intervalTime;
    vector<int> trainingDMEventPoint;
    vector<int> testDMEventPoint;
    vector<string> DMEPtoEvent; // Data Mining Each Point to Event
		
		// 將結果印出，主要是避開輸出重複的資料
		class printExperiment {
			public :
				string preStr;
				int *successTimes;
				int failedTimes;
				int maxPredictApp;
				bool printTitle;
				printExperiment(int maxPredictApp) {
					this->maxPredictApp = maxPredictApp;
					successTimes = new int[maxPredictApp];
					initial();
				}
				
				void initial() {
					preStr.clear();
					failedTimes = 0;
					for (int i=0; i<maxPredictApp; i++)
						successTimes[i] = 0;
				}
				
				/** 將結果印出 只印出有改變的兩段資訊
				 *  1. 檢查有沒有變化
				 *  1.true  : 有則輸出，並記錄 newSuccessTimes、newFailedTimes 到自己的資料
				 *  1.false : preStr <= newPreStr
				 *  參數同上 (printExperiment)
				 *   | successTimes : 成功的次數，可以往後讀 maxPredictApp 個
				 *   | failedTimes : 往回 maxPredictApp 個都沒有命中
				 */
				void printImportant(int *newSuccessTimes, int newFailedTimes, string *newPreStr) {
					// 1. 檢查有沒有變化
					bool change = false;
					if (failedTimes != newFailedTimes) {
						change = true;
					} else {
						for (int i=0; i<maxPredictApp; i++) {
							if (successTimes[i] != newSuccessTimes[i]) {
								change = true;
								break;
							}
						}
					}
					
				  // 1.true  : 有則輸出，並記錄 newSuccessTimes、newFailedTimes 到自己的資料
					if (change) {
						// print last point before check this data isn't print
						if (!preStr.empty()) {
							print(&preStr);
							preStr.clear();
						}
						
						// print new point before push data to this object
						failedTimes = newFailedTimes;
						for (int i=0; i<maxPredictApp; i++) {
							successTimes[i] = newSuccessTimes[i];
						}
						print(newPreStr);
					} else {
						// 1.false : preStr <= newPreStr
						preStr = *newPreStr;
					}
				}
				
				/** 將結果印出
				 *  主要是將數值丟到 static print 那裡
				 */
				void print(string *str) {
					cout << *str;
					print(successTimes, failedTimes, maxPredictApp, false);
				}
				
				/** 主要是印出最後的資料
				 */
				void finalPrint() {
					// check this data isn't print
					if (!preStr.empty()) {
						print(&preStr);
						preStr.clear();
					}
				}
				
				/** (static) 將結果印出
				 *  successTimes : 成功的次數，可以往後讀 maxPredictApp 個
				 *  failedTimes : 往回 maxPredictApp 個都沒有命中
				 *  maxPredictApp : 總共預測幾個
				 */
				static void print(int *successTimes, int failedTimes, int maxPredictApp, bool printTitle) {
					if (printTitle) {
						cout << "                             predict App Num & predict rate                             |" <<endl;
						cout << " 1 app  |  2 app  |  3 app  |  4 app  |  5 app  |  6 app  |  7 app  |  8 app  |  9 app  |" <<endl;
					}
					
					// 算全部 totalTimes
					int totalTimes = failedTimes;
					for (int i=0; i<maxPredictApp; i++) {
						totalTimes += successTimes[i];
					}
					// print
					for (int i=0; i<maxPredictApp; i++) {
						int outputST = 0;
						for (int j=0; j<i+1; j++) {
							outputST += successTimes[j];
						}
						printf("%1.5f | ", (double)outputST/totalTimes);
					}
					cout<<endl;
				}
		};
		
    DataMining() {
      screen_turn_on = string("screen turn on");
      screen_turn_off = string("screen turn off");
      screen_long_off = string("screen long off");
      // 看螢幕"暗著"的時間間隔 // mason
      intervalTime.initial();
      intervalTime.hour = 1;
    }

		// main function
    bool mining(vector<Event> *eventVec, vector<string> *allAppNameVec) {
			// check screen state
			checkScreenState(eventVec);
			
      //{ ----- initial
      // 放 app name & screen status
      for (int i=0; i<allAppNameVec->size(); i++) {
        DMEPtoEvent.push_back(allAppNameVec->at(i));
      }
      // screen "screen turn on" "screen turn off" "screen long off"
      DMEPtoEvent.push_back(screen_turn_on);
      EPscreen_turn_on = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_turn_off);
      EPscreen_turn_off = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_long_off);
      EPscreen_long_off = DMEPtoEvent.size()-1;//}

			// point AppName out
#ifdef EXPERIMENT_debug_Print_Event
			for (int i=0; i<DMEPtoEvent.size(); i++)
				cout << i << "\t:" << DMEPtoEvent[i] <<endl;
#endif
			
			//{ screenPattern : 主要是將 screen 亮著的區間做間隔
      vector<pair<int, int> > screenPattern;
      buildScreenPattern(&screenPattern, eventVec);
			cout << "Screen on->off interval have (" << screenPattern.size() << ") times" <<endl;//}
			
			/** 將 screenPattern 用時間分成 training test 兩份
			 *  讓 eventVec 可以直接分成兩個資料
			 *  相關 define parameter 在上面
			 *  TRAINING_INTERVAL_DAY , TEST_INTERVAL_DAY 
			 */
			buildData(&screenPattern, eventVec);
			
			// remove the same action
			removeSameAction();
			
      //{ ----- sequential patterns mining
			//int min_support = trainingDMEventPoint.size()/200; // (0.5%)
			int min_support = 2;
      GSP gsp(trainingDMEventPoint, min_support);
      gsp.Mining();
			vector<int> filterVec;
			filterVec.push_back(EPscreen_turn_on);
			filterVec.push_back(EPscreen_turn_off);
			//filterVec.push_back(EPscreen_long_off);
			gsp.Filter(&filterVec);//}
			
			//{ output GSP statistics
			//gsp.OutputAll();
			//gsp.OutpurAllTop();//}
			
			// ----- ElemStatsTree 實作機率與統計部分
			//{ initial - test data
			GSP_Predict gspPredict(&gsp);
			const int maxBackApp = 9;
			const int maxPredictApp = 9;
			int failedTimes;
			int successTimes[maxPredictApp];
			cout<<endl;//}
			
			// GSP normal Algorithm
#ifdef EXPERIMENT_GSP_normal_part
			{ cout << " ----- GSP normal predict:" <<endl;
				//{ initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}//}
				
				//{ predict
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
					// 預測和記錄
					PredictResult result = gspPredict.predictResult_normal(&shortSeq, maxPredictApp);
					int predictNum = result.predict(testDMEventPoint.at(i+1), maxPredictApp); // (ps: reallyUseApp, maxPredictApp)
					if (predictNum == PredictResult::PREDICT_MISS) {
						failedTimes++;
					} else {
						successTimes[predictNum-1]++;
					}
				}//}
				
				//{ output
				printExperiment::print(successTimes, failedTimes, maxPredictApp, true);
				cout<<endl;//}
			}
#endif
			
			// GSP const level Algorithm
#ifdef EXPERIMENT_GSP_const_level_part
			{ cout << " ----- GSP const level predict:" <<endl;
				cout << "peep | Total   |         |                            predict rate on difference Level                             |" <<endl;
				cout << " App | pattern |   MFU   | Level 1 | Level 2 | Level 3 | Level 4 | Level 5 | Level 6 | Level 7 | Level 8 | Level 9 |" <<endl;
				//{ initial
				const int maxPredictAppforSL = 1;
				failedTimes = 0;
				for (int i=0; i<maxPredictAppforSL; i++) {
					successTimes[i] = 0;
				}//}
				
				//{ 建立預測結果表格 (which sequence, level)
				const int EMPTY = 0;
				const int PREDICT_FAIL = -1;
				const int PREDICT_HEAD = 1; // 後面加幾代表第幾個才預測到
				int predictLevelMap[testDMEventPoint.size()][maxBackApp];
				for (int i=0; i<testDMEventPoint.size(); i++) {
					for (int j=0; j<maxBackApp; j++) {
						predictLevelMap[i][j] = EMPTY;
					}
				}//}
				
				//{ predict & 建立結果表格
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
					int reallyUseApp = testDMEventPoint.at(i+1);
					
					// 預測和判斷是否成功
					for (int level=0; level<maxBackApp; level++) {
						double parameter[1];
						parameter[0] = level;
						PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::ConstLevel, parameter, &shortSeq, maxPredictAppforSL);
						
						// 檢查確實有結果才繼續，否則跳出換下一個
						if (result.resultPairs.at(0).first != PredictResult::NO_APP) {
							if (result.resultPairs.at(0).first == reallyUseApp) {
								predictLevelMap[i][level] = PREDICT_HEAD + 1;	// 1 : 第幾個才預測到
							} else {
								predictLevelMap[i][level] = PREDICT_FAIL;
							}
						} else {
							break;
						}
					}
				}//}
				
				//{ 統計表格 difference level effect & output
				for (int level=0; level<maxBackApp; level++) {
					//{ initial
					for (int i=0; i<maxPredictApp; i++) {
						successTimes[i] = 0;
					}
					vector<int> catchSeqVec; // 知道有哪些是有搜尋到的內容
					failedTimes = 0; //}
					
					//{ 先找自己這層有多少個 & 沒有則跳出
					for (int i=0; i<testDMEventPoint.size(); i++) {
						if (predictLevelMap[i][level] != EMPTY) {
							catchSeqVec.push_back(i);
							if (predictLevelMap[i][level] != PREDICT_FAIL) {
								successTimes[level]++;
							} else {
								failedTimes++;
							}
						} else {
							continue;
						}
					}
					if (catchSeqVec.empty()) {
						break;
					}//}
					
					//{ 之後往下搜尋
					for (int downLevel = level-1; 0<=downLevel; downLevel--) {
						for (vector<int>::iterator seqHead = catchSeqVec.begin(); 
								 seqHead != catchSeqVec.end(); seqHead++)
						{
							if (predictLevelMap[*seqHead][downLevel] != PREDICT_FAIL) {
								successTimes[downLevel]++;
							} else {
								failedTimes++;
							}
						}
					}//}
					
					//{ output
					int totalPattern = catchSeqVec.size();
					printf("%4d | %7d | ", level, totalPattern);
					for (int i = 0; i<=level; i++) {
						printf("%1.5f | ", (double)successTimes[i]/totalPattern);
					}
					cout<<endl;//}
				}
				cout <<endl;//}
			}
#endif
			
			// GSP multiply of level Algorithm
#ifdef EXPERIMENT_GSP_multiply_of_level_part
			{ cout << " ----- GSP multiply predict rate:" <<endl;
				cout << "base | multiply |                             predict App Num & predict rate                              |" <<endl;
				cout << "     |     Num |  1 app  |  2 app  |  3 app  |  4 app  |  5 app  |  6 app  |  7 app  |  8 app  |  9 app  |" <<endl;
				printExperiment multiplyPrint(maxPredictApp);
				//{ parameter initial | method: weight = base + multiplyNum * level(0~n)
				double base = 1;
				double multiplyNum = 0;
				double multiplyNumMax = 100;
				double multiplyGrow = 0.01;//}
				
				for (multiplyNum; multiplyNum <= multiplyNumMax; multiplyNum+=multiplyGrow) {
					//{ initial
					failedTimes = 0;
					for (int i=0; i<maxPredictApp; i++) {
						successTimes[i] = 0;
					}//}
					
					//{ predict
					double parameter[2];
					parameter[0] = base;
					parameter[1] = multiplyNum;
					for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
						Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
						
						// 預測和記錄
						PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::multiplyOfLevle, parameter, &shortSeq, maxPredictApp);
						int predictNum = result.predict(testDMEventPoint.at(i+1), maxPredictApp); // (ps: reallyUseApp, maxPredictApp)
						if (predictNum == PredictResult::PREDICT_MISS) {
							failedTimes++;
						} else {
							successTimes[predictNum-1]++;
						}
					}//}
					
					//{ output
					char outChar[64];
					snprintf(outChar, 64, "%.2f | %.5f | ", base, multiplyNum);
					string str = string(outChar);
					multiplyPrint.printImportant(successTimes, failedTimes, &str);//}
				}
				multiplyPrint.finalPrint();
				cout<<endl;
			}
#endif
			
			// GSP power of level Algorithm
#ifdef EXPERIMENT_GSP_power_of_level_part
			{ cout << " ----- GSP power predict rate:" <<endl;
				cout << "base |   power |                             predict App Num & predict rate                              |" <<endl;
				cout << "     |     Num |  1 app  |  2 app  |  3 app  |  4 app  |  5 app  |  6 app  |  7 app  |  8 app  |  9 app  |" <<endl;
				printExperiment powerPrint(maxPredictApp);
				//{ parameter initial | method: weight = base + powerNum ^ level(0~n)
				double base = 1;
				double powerNum = 900;
				double powerNumMax = 900;
				double powerGrow = 1;//}
				
				for (powerNum; powerNum <= powerNumMax; powerNum+=powerGrow) {
					//{ initial
					failedTimes = 0;
					for (int i=0; i<maxPredictApp; i++) {
						successTimes[i] = 0;
					}//}
					
					//{ predict
					double parameter[2];
					parameter[0] = base;
					parameter[1] = powerNum;
					for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
						Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
						
						// 預測和記錄
						PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::powerOfLevel, parameter, &shortSeq, maxPredictApp);
						int predictNum = result.predict(testDMEventPoint.at(i+1), maxPredictApp); // (ps: reallyUseApp, maxPredictApp)
						if (predictNum == PredictResult::PREDICT_MISS) {
							failedTimes++;
						} else {
							successTimes[predictNum-1]++;
						}
					}//}
					
					//{ output
					char outChar[64];
					snprintf(outChar, 64, "%.2f | %.5f | ", base, powerNum);
					string str = string(outChar);
					powerPrint.printImportant(successTimes, failedTimes, &str);//}
				}
				powerPrint.finalPrint();
				cout<<endl;
			}
#endif
			
			// LRU
#ifdef EXPERIMENT_LRU_part
			{ cout << " ----- LRU predict:" <<endl;
				//{ initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}//}
				
				//{ predict
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					int reallyUseApp = testDMEventPoint.at(i+1);
					for (int j=1; j<maxBackApp+1 && j<maxPredictApp+1; j++) {
						if (testDMEventPoint.at(i-j) == reallyUseApp) {
							successTimes[j-1]++;
							break;
						} else if (j == maxBackApp || j == maxPredictApp) {
							failedTimes++;
						}
					}
				}//}
				
				//{ output
				printExperiment::print(successTimes, failedTimes, maxPredictApp, true);
				cout<<endl;//}
			}
#endif
			
			// MFU
#ifdef EXPERIMENT_MFU_part
			{ cout << " ----- MFU predict:" <<endl;
				//{ initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}//}
				
				//{ 整理 sequence
				Sequence shortSeq;
				Itemset tmpItem;
				tmpItem.item.push_back(30000);
				shortSeq.itemset.push_back(tmpItem);//}
				
				//{ predict
				double parameter[1];
				parameter[0]=0;
				PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::ConstLevel, parameter, &shortSeq, maxPredictApp);
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					// 預測和記錄
					int predictNum = result.predict(testDMEventPoint.at(i+1), maxPredictApp); // (ps: reallyUseApp, maxPredictApp)
					if (predictNum == PredictResult::PREDICT_MISS) {
						failedTimes++;
					} else {
						successTimes[predictNum-1]++;
					}
				}//}
				
				//{ output
				printExperiment::print(successTimes, failedTimes, maxPredictApp, true);
				cout<<endl;//}
			}
#endif
			
    }
		
		/** 建立 sequence 方便搜尋和預測
		 *  (bug) 不會比較 seqEnd 有沒有比 maxBackApp 還長，所以可能會往後挖過頭
		 *  (bug) 也不會比較 seqEnd 有沒有超過 DMEventPoint
		 */
		Sequence buildShortSequence(int seqEnd, int maxBackApp, const vector<int> *DMEventPoint) {
			Sequence shortSeq;
			// 整理 sequence
			for (int j=seqEnd-maxBackApp; j<=seqEnd; j++) {
				Itemset tmpItem;
				tmpItem.item.push_back(DMEventPoint->at(j));
				shortSeq.itemset.push_back(tmpItem);
			}
			//shortSeq.Output();
			return shortSeq;
		}
		
		/** 檢查一下螢幕資料是否有問題 
		 *  有連續的兩個點連接的螢幕資訊不一樣的話，會出輸警告
		 */
		void checkScreenState(vector<Event> *eventVec) {
			bool lastScreen = eventVec->at(0).isNextScreenOn;
			for (int i=1; i<eventVec->size(); i++) {
				if (lastScreen != eventVec->at(i).isThisScreenOn) {
					cout << "(Warning) screen data is NOT good. at (" << i << ") event" <<endl;
					break;
				}
				lastScreen = eventVec->at(i).isNextScreenOn;
			}
		}
		
		/** 取得 screen 開關 Pattern
		 *  主要是取得 on -> off 也就是亮著的時間間隔
		 */
		void buildScreenPattern(vector<pair<int, int> > *screenPattern, vector<Event> *eventVec) {
      // 看螢幕"暗著"的時間間隔 // mason
      pair<int, int> onePattern = make_pair(0,0);
      int pI = 0;
      // PI 有起頭之後就用 while 跑
      while (pI<eventVec->size()) {
        // 找接下來變"亮"的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (eventVec->at(pI).isThisScreenOn) {
						onePattern.first = pI;
            break;
          }
        }
				
        // 找接下來變"暗"的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (!eventVec->at(pI).isThisScreenOn) {
            onePattern.second = pI;
            break;
          }
        }
				
        // 找到亮暗的區間
        // 先檢察是不是還在 pattern 範圍內，是的話就記錄，不是就 break
        if (pI < eventVec->size()) {
          screenPattern->push_back(onePattern);
        } else {
					break;
				}
      }
		}
		
		/** 利用螢幕分隔(screenPattern)來切成兩個時間段
		 *  並將其資料分別裝入 trainingDMEventPoint、testDMEventPoint
		 *  screenPattern : 螢幕亮著的區間
		 *  eventVec : 使用 APP 流程
		 *  TRAINING_INTERVAL_DAY, TEST_INTERVAL_DAY : 分別為 training, test 時間
		 *  (bug) 一定要小於 TrainingIT.day & TestIT.day，沒有寫防呆
		 */
		void buildData(vector<pair<int, int> > *screenPattern, vector<Event> *eventVec) {
			//{ 設定 training & test data 要多久
			DateTime TrainingIT, TestIT; // Training interval time , test interval time
			TrainingIT.initial_Day(TRAINING_INTERVAL_DAY);
			TestIT.initial_Day(TEST_INTERVAL_DAY);//}
			
			//{ initial
			vector<pair<int, int> > trainingScnPatn, testScnPatn;
			int trainingStart = 0;
			int trainingEnd = 0;
			int testStart = 0;
			int testEnd = screenPattern->size()-1;//}
			
			// 找出兩種資料的開頭和結尾
#ifdef PIN_TESTEND_ON_DATAEND
			//{ 以 data end 開始往前收集
			DateTime *endTime = eventVec->at(screenPattern->at(testEnd).second).thisDate;
			for (testStart = testEnd; 0<=testStart; testStart--) {
				DateTime *headTime = eventVec->at(screenPattern->at(testStart).first).thisDate;
				if (*endTime - *headTime > TestIT) { // (bug) 一定要小於 TestIT.day，沒有寫防呆
					break;
				}
			}
			trainingEnd = testStart-1;
			endTime = eventVec->at(screenPattern->at(trainingEnd).second).thisDate;
			for (trainingStart = trainingEnd; 0<=trainingStart; trainingStart--) {
				DateTime *headTime = eventVec->at(screenPattern->at(trainingStart).first).thisDate;
				if (*endTime - *headTime > TrainingIT) { // (bug) 一定要小於 TrainingIT.day，沒有寫防呆
					break;
				}
			}//}
#else
			//{ 以 data start 開始往後收集
			DateTime *headTime = eventVec->at(screenPattern->at(trainingStart).first).thisDate;
			for (trainingEnd = trainingStart; trainingEnd<eventVec->size(); trainingEnd++) {
				DateTime *endTime = eventVec->at(screenPattern->at(trainingEnd).second).thisDate;
				if (*endTime - *headTime > TrainingIT) { // (bug) 一定要小於 TrainingIT.day，沒有寫防呆
					break;
				}
			}
			testStart = trainingEnd+1;
			headTime = eventVec->at(screenPattern->at(testStart).first).thisDate;
			for (testEnd = testStart; testEnd<eventVec->size(); testEnd++) {
				DateTime *endTime = eventVec->at(screenPattern->at(testEnd).second).thisDate;
				if (*endTime - *headTime > TestIT) { // (bug) 一定要小於 TestIT.day，沒有寫防呆
					break;
				}
			}//}
#endif
			
			//{ 裝進 trainingScnPatn & testScnPatn
			for (int i=trainingStart; i<=trainingEnd; i++) {
				trainingScnPatn.push_back(screenPattern->at(i));
			}
			testStart = trainingEnd+1; // 通常
			for (int i=testStart; i<=testEnd; i++) {
				testScnPatn.push_back(screenPattern->at(i));
			}//}
			
      //{ 整理後裝進 trainingDMEventPoint (training)、testDMEventPoint (test)
      buildDMEventPoint(&trainingDMEventPoint, &trainingScnPatn, eventVec);
      buildDMEventPoint(&testDMEventPoint, &testScnPatn, eventVec);//}
			
			//{ 輸出相關資訊
			cout << "Training action data is between ";
			eventVec->at(screenPattern->at(trainingStart).first).thisDate->output();
			cout << " and\n                                ";
			eventVec->at(screenPattern->at(trainingEnd).second).thisDate->output();
			cout << "\nTraining data mining event point have (" << trainingDMEventPoint.size() << ") times" <<endl;
			
			cout << "Test action data is between ";
			eventVec->at(screenPattern->at(testStart).first).thisDate->output();
			cout << " and\n                            ";
			eventVec->at(screenPattern->at(testEnd).second).thisDate->output();
			cout << "\nTest data mining event point have (" << testDMEventPoint.size() << ") times" <<endl;//}
		}
		
    /** 如果有遇到一次發現多的 app 執行的話
     *  就用 "oom_score" 來判斷先後
     *  數字大的前面
     *  最後都放到 trainingDMEventPoint 之中
     */
    void buildDMEventPoint(vector<int> *trainingDMEventPoint, vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 用 usePattern 來取出
      for (int i=0; i<usePattern->size(); i++) {
        pair<int, int> useInterval = usePattern->at(i);
				// 順序是先
				// 1. check screen, if turn on, record it
				// 2. record app
				// 3. check screen, if turn off, record it
				
				// ----- 1. check screen, if turn on, record it
				//trainingDMEventPoint->push_back(EPscreen_turn_on);
				
        //{ ----- 2. record app
        for (int EP=useInterval.first; EP<=useInterval.second; EP++) { // PS: EP = EventPoint
          Event* oneshut = &(eventVec->at(EP));
          vector<Event::Case> *caseVec = &(oneshut->caseVec);
          //{ 有東西才記錄
          if (caseVec->size()!=0) {
            bool app_record[caseVec->size()];
            for (int j=0; j<caseVec->size(); j++) {
              app_record[j] = false;
            }
            for (int j=0; j<caseVec->size(); j++) {
              int big_oom_score = -1;
              int big_app_num = -1;
              int big_app_name = -1;
              for (int k=0; k<caseVec->size(); k++) {
                if (app_record[k]) {
                  continue;
                }
                if (big_oom_score <= caseVec->at(k).nextApp->oom_score) {
                  big_oom_score = caseVec->at(k).nextApp->oom_score;
                  big_app_num = k;
                  big_app_name = caseVec->at(k).nextApp->namePoint;
                }
              }
              if (big_oom_score<0 || big_app_num<0 || big_app_name<0) {
                cout << "bad" <<endl;
              }
              trainingDMEventPoint->push_back(big_app_name);
              app_record[big_app_num] = true;
            }
          }//}
        }//}
				
				// ----- 3. check screen, if turn off, record it
				//trainingDMEventPoint->push_back(EPscreen_turn_off);
      }
    }
		
		/** 將 training、test 中連續一樣的資料刪掉
		 *  以免後面出現使用連續兩個一樣的 APP
		 */
		void removeSameAction() {
			//{ training
			vector<int>::iterator lastIter = trainingDMEventPoint.begin();
			vector<int>::iterator thisIter = trainingDMEventPoint.begin();
			thisIter++;
			while (thisIter!=trainingDMEventPoint.end()) {
				if (*lastIter != *thisIter) {
					// 不一樣的話沒事
					lastIter++;
					thisIter++;
				} else {
					// 一樣的話要刪掉 thisIter
					thisIter = trainingDMEventPoint.erase(thisIter);
				}
			}//}
			
			//{ test
			lastIter = testDMEventPoint.begin();
			thisIter = testDMEventPoint.begin();
			thisIter++;
			while (thisIter!=testDMEventPoint.end()) {
				if (*lastIter != *thisIter) {
					// 不一樣的話沒事
					lastIter++;
					thisIter++;
				} else {
					// 一樣的話要刪掉 thisIter
					thisIter = testDMEventPoint.erase(thisIter);
				}
			}//}
			
			//{ output
			cout << "NEW User training action have (" << trainingDMEventPoint.size() << ") times" <<endl;
			cout << "NEW User test action have (" << testDMEventPoint.size() << ") times" <<endl;//}
		}
		
		/** (bug)
		 * 目前發現會不小心將螢幕暗著的時候也記錄進去
		 * 所以先捨棄不用
		 * 和下面的 buildtrainingDMEventPoint 一樣不用
		 */
    /*bool getUserPattern(vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 看螢幕"暗著"的時間間隔 // mason
      cout << "    ========= Interval Time > " << intervalTime.hour << " hour ==========" <<endl;
      cout << "              start                     end     interval" <<endl;
      Event *screenChgOnShut, *screenChgOffShut;
      pair<int, int> onePattern = make_pair(0,0);
      int pI;
      // 先找第一次變亮的時候
      for (pI = 0; pI<eventVec->size(); pI++) {
        if (eventVec->at(pI).isThisScreenOn) {
          screenChgOnShut = &(eventVec->at(pI));
          onePattern.first = pI;
          break;
        }
      }
      // PI 有起頭之後就用 while 跑
      while (pI<eventVec->size()) {
        // 再找第一次變暗的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (!eventVec->at(pI).isThisScreenOn) {
            screenChgOffShut = &(eventVec->at(pI));
            onePattern.second = pI;
            break;
          }
        }
        // 這裡的 screenChgOnShut screenChgOffShut 之間是 螢幕亮的期間

        // 新的一次 找第一次變亮的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (eventVec->at(pI).isThisScreenOn) {
            screenChgOnShut = &(eventVec->at(pI));
            break;
          }
        }

        // 先檢察是不是還在 pattern 範圍內
        if (!(pI<eventVec->size())) {
          break;
        }
        // 有找到的話 檢查中間間隔是不是夠久
        // 比 intervalTime 還長的話就代表有 並紀錄
        else if (*(screenChgOnShut->thisDate) - *(screenChgOffShut->thisDate) > intervalTime) {
          usePattern->push_back(onePattern);
          onePattern.first = pI;
          //screenChgOffShut->thisDate->output();
          //cout << "\t";
          //screenChgOnShut->thisDate->output();
          //cout << "\t";
          //(*(screenChgOnShut->thisDate) - *(screenChgOffShut->thisDate)).output();
          //cout << endl;
        }
      }
      return true;
    }*/

    /** 如果有遇到一次發現多的 app 執行的話
     * 就用 "oom_score" 來判斷先後
     * 數字大的前面
     * 最後都放到 trainingDMEventPoint 之中
		 * (bug) 和 getUserPattern 一樣，對螢幕亮暗的紀錄不太好
		 * 所以也先暫時不用
     */
    /*void buildDMEventPoint(vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 用 usePattern 來取出
      for (int i=0; i<usePattern->size(); i++) {
        pair<int, int> useInterval = usePattern->at(i);
        // EP = EventPoint
        for (int EP=useInterval.first; EP<=useInterval.second; EP++) {
          // 順序是先
          // 1. check screen, if turn on, record it
          // 2. record app
          // 3. check screen, if turn off, record it
					
          Event* oneshut = &(eventVec->at(EP));
          // ----- 1. check screen, if turn on, record it
          if (!oneshut->isThisScreenOn && oneshut->isNextScreenOn) {
            trainingDMEventPoint.push_back(EPscreen_turn_on);
          }

          // ----- 2. record app
          vector<Event::Case> *caseVec = &(oneshut->caseVec);
          // 有東西才記錄
          if (caseVec->size()!=0) {
            bool app_record[caseVec->size()];
            for (int j=0; j<caseVec->size(); j++) {
              app_record[j] = false;
            }
            for (int j=0; j<caseVec->size(); j++) {
              int big_oom_score = -1;
              int big_app_num = -1;
              int big_app_name = -1;
              for (int k=0; k<caseVec->size(); k++) {
                if (app_record[k]) {
                  continue;
                }
                if (big_oom_score <= caseVec->at(k).nextApp->oom_score) {
                  big_oom_score = caseVec->at(k).nextApp->oom_score;
                  big_app_num = k;
                  big_app_name = caseVec->at(k).nextApp->namePoint;
                }
              }
              if (big_oom_score<0 || big_app_num<0 || big_app_name<0) {
                cout << "bad" <<endl;
              }
              trainingDMEventPoint.push_back(big_app_name);
              //cout << big_oom_score << " : " << big_app_name;
              //cout << "\t" << caseVec->at(j).nextApp->oom_score << " : " << caseVec->at(j).nextApp->namePoint;
              //cout << endl;
              app_record[big_app_num] = true;
            }
          }
          
          // ----- 3. check screen, if turn off, record it
          if (oneshut->isThisScreenOn && !oneshut->isNextScreenOn) {
            trainingDMEventPoint.push_back(EPscreen_turn_off);
          }
        }
        // 接下來是很久時間都沒打開螢幕
        trainingDMEventPoint.push_back(EPscreen_long_off);
      }
    }*/
};

int main(int argc, char** argv) {
  // 資料夾路徑(絕對位址or相對位址) 目前用現在的位置
  string dir = (argc == 2) ? string(argv[1]):string(".");
	
  vector<string> files;
  vector<CollectionFile> collecFileVec; // 所有檔案的 vector
  CollectionAllData collecAllData;      // 整理所有資料
  DataMining dataMining;
  // 取出檔案，並判斷有沒有問題
  if (getdir(dir, files) == -1) {
    cout << "Error opening" << dir << endl;
    return -1;
  }

  // 整理所有檔案內容
  for(int i=0; i<files.size(); i++) {
    CollectionFile oneFile;
    oneFile.fileName = files[i];
    // 檢查檔案看是不是需要的檔案，像 2017-12-24_03.20.27 就是正確的檔名
	  if (!oneFile.setAllDateTime()) {
      continue;
    }
    oneFile.fileName = dir + oneFile.fileName;
    // 確定沒問題後打開檔案
    if (oneFile.openFileAndRead()) {
      // 檔案資料正確後，放入檔案的 linked list 中
      // 順便排序
      int ins = 0;
      for (ins=0; ins<collecFileVec.size(); ins++) {
        if (collecFileVec[ins] > oneFile)
          break;
      }
      collecFileVec.insert(collecFileVec.begin()+ins, oneFile);
    } else {
      continue;
    }
  }
  // 檔名依序輸出
  //for (int i=0; i<collecFileVec.size(); i++)
  //  collecFileVec[i].date.output();
  
  // 輸出數量
  cout << "收集了多少" << collecFileVec.size() << "檔案" <<endl;
  
  // 將檔案給 collecAllData 整理成可讓 dataMining 讀的資料
  collecAllData.collection(&collecFileVec);
  // 將 collecAllData 中的 allEventVec appNameVec 給 dataMining 去整理
  dataMining.mining(&collecAllData.allEventVec, &collecAllData.allAppNameVec);
  
  cout << "  over" <<endl;
  return 0;
}

int getdir(string dir, vector<string> &files) {
  // 創立資料夾指標
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(dir.c_str())) == NULL) {
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
