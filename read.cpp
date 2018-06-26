﻿#include <dirent.h>
#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>
//#include <netcdfcpp.h>

//#define EXPERIMENT_debug_addOom_adjStat

//#define EXPERIMENT_debug_oomAdj_less_than_0
//#define EXPERIMENT_debug_oomAdj_rate_onEachApp
#define EXPERIMENT_debug_oomAdj_statistics
//#define EXPERIMENT_debug_IntervalTime

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
		 *  出發點 : 因為 Cytus 會有兩個，所以要特別刪掉一個
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
            if (i==allPatternVec[j].app[k].namePoint) {
              newAppDetail.findTimes++;
              newAppDetail.addOom_adjStat(allPatternVec[j].app[k].oom_adj);
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
			bool isHave = false;
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
      {
        cout << "    ============== 0 <= oom_adj ==============" <<endl;
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

    // 主要將資料裝到 allEventVec
    void makeAllEventVec(bool *unneededAppArray) {
      
      // 將 allPatternVec 整理後裝到這裡 allEventVec
      // 查看 oom_adj 發生改變的狀況 allPatternVec
      int changeTimes = 0;
      for (int i=0; i+1<allPatternVec.size(); i++) {
        Event thisEvent;
        
        bool isChange = false;
        bool isOom_adjCgToZero = false;
        
        Point::App *thisApp = allPatternVec[i].app;
        int thisAppNum = allPatternVec[i].appNum;
        Point::App *nextApp = allPatternVec[i+1].app;
        int nextAppNum = allPatternVec[i+1].appNum;
        bool isNextAppMatch[nextAppNum];    // 看是不是所有的下一個App都有對應到
        for (int j=0; j<nextAppNum; j++) {
          // 順便過濾
          if (unneededAppArray[nextApp[j].namePoint]) {
            isNextAppMatch[j] = true;
          } else {
            isNextAppMatch[j] = false;
          }
        }
        // 從這一個往下連接，一一找一樣的 App
        for (int j=0; j<thisAppNum; j++) {
          // 過濾一些不需要檢查的 app
          if (unneededAppArray[thisApp[j].namePoint]) {
            continue;
          }
          
          bool isFindApp = false;
          for (int k=0; k<nextAppNum; k++) {
            if (isNextAppMatch[k]) {
              continue;
            }
            // namePoint 一樣的話，代表示同的 App
            if (thisApp[j].namePoint == nextApp[k].namePoint) {
              isFindApp = true;
              isNextAppMatch[k] = true;
              // 看 oom_adj有沒有變化
              if (thisApp[j].oom_adj != nextApp[k].oom_adj) {
                // 檢查 oom_adj 下一刻變0的狀況
                if (nextApp[k].oom_adj == 0) {
                  isOom_adjCgToZero = true;
                  // 收集此case
                  Event::Case thisCase;
                  thisCase.namePoint = thisApp[j].namePoint;
                  thisCase.isCreat = false;
                  thisCase.thisApp = &thisApp[j];
                  thisCase.nextApp = &nextApp[k];
                  thisEvent.caseVec.push_back(thisCase);
                }
              }
              continue;
            }
          }
          if (!isFindApp) {
            isChange = true;
          }
        }
        // 看有沒有新的 app 被單獨創出來，而且還是0
        bool isCreatNewApp = false;
        for (int j=0; j<nextAppNum; j++) {
          if (!isNextAppMatch[j]) {
            if (nextApp[j].oom_adj==0) {
              isCreatNewApp = true;
              // 收集此case
              Event::Case thisCase;
              thisCase.namePoint = nextApp[j].namePoint;
              thisCase.isCreat = true;
              thisCase.nextApp = &nextApp[j];
              thisEvent.caseVec.push_back(thisCase);
            }
          }
        }
        
        // 有改變的話 changeTimes++
        if (isChange || isCreatNewApp) {
          changeTimes++;
        }
        
        // 看有沒有螢幕開關，但沒有其他變化
        bool isScreenChange = (allPatternVec[i].screen != allPatternVec[i+1].screen);
        // 看 oom_adj 有沒有變成0 或是有新開的APP 以及螢幕是否有改變
        if (isOom_adjCgToZero || isCreatNewApp || isScreenChange) {
          // 都有的話將資料寫入
          thisEvent.thisDate = &allPatternVec[i].date;
          thisEvent.nextDate = &allPatternVec[i+1].date;
          thisEvent.isThisScreenOn = allPatternVec[i].screen;
          thisEvent.isNextScreenOn = allPatternVec[i+1].screen;
          allEventVec.push_back(thisEvent);
        }
      }

      //{ allEventVec 中的 app 可能出現重複的 app (收集時就有問題)
			for (int i=0; i<allEventVec.size(); i++) {
				allEventVec.at(i).sortOut();
			}//}
      
      // 輸出 each oom_adj 統計
#ifdef EXPERIMENT_debug_oomAdj_statistics
			cout << "    ================= oom_adj =================" <<endl;
			cout << "Change Times : " << changeTimes <<endl;
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
    
    void addonePoint(Point oneShot, const vector<string> *oneAppNameVec) {
      for (int i=0; i<oneShot.appNum; i++) {
        findAppNamePoint(&oneShot.app[i], &(*oneAppNameVec)[oneShot.app[i].namePoint]);
      }
      allPatternVec.push_back(oneShot);
    }
    
    int findAppNamePoint(Point::App *app, const string *appName) {
      // 先檢查 allAppNameVec 中有沒有此名字
      for (int i=0; i<allAppNameVec.size(); i++) {
        // 有找到的話直接回傳新的 namePoint
        if (*appName == allAppNameVec[i]) {
          app->namePoint = i;
          return i;
        }
      }
      
      // 沒找到的話則加進去，並取得新的 Point
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
    vector<string> DMEPtoEvent;
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
			
      // 先將 app 一一列好
      // ----- initial
      // 先放 app name
      for (int i=0; i<allAppNameVec->size(); i++) {
        DMEPtoEvent.push_back(allAppNameVec->at(i));
      }
      // screen "screen turn on" "screen turn off" "screen long off"
      DMEPtoEvent.push_back(screen_turn_on);
      EPscreen_turn_on = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_turn_off);
      EPscreen_turn_off = DMEPtoEvent.size()-1;
      DMEPtoEvent.push_back(screen_long_off);
      EPscreen_long_off = DMEPtoEvent.size()-1;

			// 輸出 point to AppName
			//for (int i=0; i<DMEPtoEvent.size(); i++)
			//	cout << i << "\t:" << DMEPtoEvent[i] <<endl;
			
			// screenPattern : 主要是將 screen 亮著的區間做間隔
      vector<pair<int, int> > screenPattern;
      buildScreenPattern(&screenPattern, eventVec);
			cout << "Screen on->off interval have (" << screenPattern.size() << ") times" <<endl;
			
			// 將 screenPattern 用時間分成 training test 兩份
			// 讓 eventVec 可以直接分成兩個資料
			DateTime intervalTime;	// 設定 training data 要多久
			intervalTime.initial();
			intervalTime.day = 14;	// 2 week
			// 將資料分類成 training、test
			buildData(&intervalTime, &screenPattern, eventVec);
			
			// remove the same action
			removeSameAction();
			
      // ----- sequential patterns mining
			//int min_support = trainingDMEventPoint.size()/200; // (0.5%)
			int min_support = 2;
      GSP gsp(trainingDMEventPoint, min_support);
      gsp.Mining();
			vector<int> filterVec;
			filterVec.push_back(EPscreen_turn_on);
			filterVec.push_back(EPscreen_turn_off);
			//filterVec.push_back(EPscreen_long_off);
			gsp.Filter(&filterVec);
			//gsp.OutputAll();
			//gsp.OutpurAllTop();
			
			// ElemStatsTree 實作機率與統計部分
			GSP_Predict gspPredict(&gsp);
			
			// test data experiment
			const int maxBackApp = 9;
			const int maxPredictApp = 9;
			int failedTimes;
			int successTimes[maxPredictApp];
			// GSP normal Algorithm
			{
				// initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}
				
				// predict
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
					
					// 預測和判斷是否成功
					PredictResult result = gspPredict.predictResult_normal(&shortSeq, maxPredictApp);
					int reallyUseApp = testDMEventPoint.at(i+1);
					for (int j=0; j<maxPredictApp; j++) {
						if (result.resultPairs.at(j).first == reallyUseApp) {
							successTimes[j]++;
							break;
						} else if (j == maxPredictApp-1) {
							failedTimes++;
						}
					}
				}
				
				// output
				cout << "GSP normal predict rate: " <<endl;
				printExperiment(successTimes, failedTimes, maxPredictApp);
			}
			
			// GSP special level Algorithm
			{
				// initial
				const int maxPredictAppforSL = 1;
				failedTimes = 0;
				for (int i=0; i<maxPredictAppforSL; i++) {
					successTimes[i] = 0;
				}
				
				// 建立預測結果表格 (which sequence, level)
				const int EMPTY = 0;
				const int PREDICT_FAIL = -1;
				const int PREDICT_HEAD = 1; // 後面加幾代表第幾個才預測到
				int predictLevelMap[testDMEventPoint.size()][maxBackApp];
				for (int i=0; i<testDMEventPoint.size(); i++) {
					for (int j=0; j<maxBackApp; j++) {
						predictLevelMap[i][j] = EMPTY;
					}
				}
				
				// predict
				// 建立結果表格
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					Sequence shortSeq = buildShortSequence(i, maxBackApp, &testDMEventPoint);
					int reallyUseApp = testDMEventPoint.at(i+1);
					
					// 預測和判斷是否成功
					for (int level=0; level<maxBackApp; level++) {
						PredictResult result = gspPredict.predictResult_specialLevel(&shortSeq, level, maxPredictAppforSL);
						
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
				}
				
				cout << "GSP special level predict rate: " <<endl;
				// 統計表格 difference level effect
				// 從 level 0 開始找
				for (int level=0; level<maxBackApp; level++) {
					// initial
					for (int i=0; i<maxPredictApp; i++) {
						successTimes[i] = 0;
					}
					failedTimes = 0;
					vector<int> catchSeqVec; // 知道有哪些是有搜尋到的內容
					
					// 先找自己這層有多少個
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
					
					// 都沒有東西就跳出吧
					if (catchSeqVec.empty()) {
						break;
					}
					
					// 之後往下搜尋
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
					}
					
					// output
					cout << "----- level: " << level << " total: " << catchSeqVec.size() <<endl;
					for (int downLevel = level; 0<=downLevel; downLevel--) {
						cout << "L" << downLevel;
						cout << " predict rate: " << (double)successTimes[downLevel]/catchSeqVec.size() <<endl;
					}
				}
				cout <<endl;
			}
			
			// LRU
			{
				// initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}
				
				// predict
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					int reallyUseApp = testDMEventPoint.at(i+1);
					for (int j=1; j<maxBackApp+1 && j<maxPredictApp+1; j++) {
						if (testDMEventPoint.at(i-j) == reallyUseApp) {
							//cout << "predict:" << testDMEventPoint.at(i-1) << " use:" << testDMEventPoint.at(i+1) << " success" <<endl;
							successTimes[j-1]++;
							break;
						} else if (j == maxBackApp || j == maxPredictApp) {
							failedTimes++;
						}
					}
				}
				
				// output
				cout << "LRU predict rate: " <<endl;
				printExperiment(successTimes, failedTimes, maxPredictApp);
			}
			
			// MFU
			{
				// initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
					successTimes[i] = 0;
				}
				
				// 整理 sequence
				Sequence shortSeq;
				Itemset tmpItem;
				tmpItem.item.push_back(30000);
				shortSeq.itemset.push_back(tmpItem);
				
				// predict
				PredictResult result = gspPredict.predictResult_specialLevel(&shortSeq, 0, maxPredictApp);
				for (int i=maxBackApp; i<testDMEventPoint.size()-1; i++) {
					int reallyUseApp = testDMEventPoint.at(i+1);
					for (int j=0; j<maxPredictApp; j++) {
						if (result.resultPairs.at(j).first == reallyUseApp) {
							successTimes[j]++;
							break;
						} else if (j == maxPredictApp-1) {
							failedTimes++;
						}
					}
				}
				
				// output
				cout << "MFU predict rate: " <<endl;
				printExperiment(successTimes, failedTimes, maxPredictApp);
			}
			
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
		
		/** 將結果印出
		 *  successTimes : 成功的次數，可以往後讀 maxPredictApp 個
		 *  failedTimes : 往回 maxPredictApp 個都沒有命中
		 *  maxPredictApp : 總共預測幾個
		 */
		void printExperiment(int *successTimes, int failedTimes, int maxPredictApp) {
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
				cout << "predict app number:" << i+1 << " predict rate:" << (double)outputST/totalTimes <<endl;
			}
			cout <<endl;
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
		 *  intervalTime : 要切割多少 training 時間
		 *  screenPattern : 螢幕亮著的區間
		 *  eventVec : 使用 APP 流程
		 */
		void buildData(DateTime *intervalTime, vector<pair<int, int> > *screenPattern, vector<Event> *eventVec) {
			vector<pair<int, int> > trainingScnPatn, testScnPatn;
			int testHead = 0;
			DateTime *headTime = eventVec->at(screenPattern->at(testHead).first).thisDate;
			for (testHead = 0; testHead<eventVec->size(); testHead++) { 
				DateTime *endTime = eventVec->at(screenPattern->at(testHead).first).thisDate;
				if (*endTime - *headTime > *intervalTime) { // (bug) 一定要小於 trainingIntervalTime.day，沒有寫防呆
					break;
				}
			}
			// 裝進 trainingScnPatn
			for (int i=0; i<testHead; i++) {
				trainingScnPatn.push_back(screenPattern->at(i));
			}
			// 裝進 testScnPatn
			for (int i=testHead; i<screenPattern->size(); i++) {
				testScnPatn.push_back(screenPattern->at(i));
			}
			
      // 整理後裝進 trainingDMEventPoint (training)、testDMEventPoint (test)
      buildDMEventPoint(&trainingDMEventPoint, &trainingScnPatn, eventVec);
      buildDMEventPoint(&testDMEventPoint, &testScnPatn, eventVec);
			cout << "User training action have (" << trainingDMEventPoint.size() << ") times" <<endl;
			cout << "User test action have (" << testDMEventPoint.size() << ") times" <<endl;
		}
		
    /** 如果有遇到一次發現多的 app 執行的話
     * 就用 "oom_score" 來判斷先後
     * 數字大的前面
     * 最後都放到 trainingDMEventPoint 之中
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
				
        // ----- 2. record app
        for (int EP=useInterval.first; EP<=useInterval.second; EP++) { // PS: EP = EventPoint
          Event* oneshut = &(eventVec->at(EP));
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
              trainingDMEventPoint->push_back(big_app_name);
              //cout << big_oom_score << " : " << big_app_name;
              //cout << "\t" << caseVec->at(j).nextApp->oom_score << " : " << caseVec->at(j).nextApp->namePoint;
              //cout << endl;
              app_record[big_app_num] = true;
            }
          }
        }
				
				// ----- 3. check screen, if turn off, record it
				//trainingDMEventPoint->push_back(EPscreen_turn_off);
      }
    }
		
		/** 將 training、test 中連續一樣的資料刪掉
		 *  以免後面出現使用連續兩個一樣的 APP
		 */
		void removeSameAction() {
			// training
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
			}
			
			// test
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
			}
			
			// output
			cout << "NEW User training action have (" << trainingDMEventPoint.size() << ") times" <<endl;
			cout << "NEW User test action have (" << testDMEventPoint.size() << ") times" <<endl;
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
