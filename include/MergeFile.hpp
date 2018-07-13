#ifndef MERGE_FILE_HPP
#define MERGE_FILE_HPP

#include <vector>
#include <list>
#include <string.h>
#include <stdio.h>
#include <iostream>

//#define MERGEFILE_debug_addOom_adjStat
//#define MERGEFILE_debug_oomAdj_less_than_0
//#define MERGEFILE_debug_oomAdj_rate_onEachApp
//#define MERGEFILE_debug_oomAdj_statistics
//#define MERGEFILE_debug_IntervalTime
//#define MERGEFILE_debug_adj_score

#include "DateTime.hpp"
#include "StringToNumber.hpp"
#include "CollectionFile.hpp"
using namespace std;

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

class MergeFile {
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
#ifdef MERGEFILE_debug_addOom_adjStat
            cout << "oom_adj out of value : " << oom_adj << endl;
#endif
            return true;
          } else {
#ifdef MERGEFILE_debug_addOom_adjStat
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
    vector<string> allAppNameVec;       // 用 collectFileVec 的資料收集所有的 app's name
    vector<Point> allPatternVec;        // 所有 pattern
    vector<AppDetail> allAppDetailVec;  // 所有 app 的詳細資料
    /** allEventVec 主要是放入了 發生過的事件 並依序放好
     *  其中的 Event 是使用者剛好切換 APP 或是開關螢幕也會知道
     */
    vector<Event> allEventVec; 
    
    MergeFile() {};
    
    void collection(vector<CollectionFile> *collectFileVec) {
			//{ 收集所有的 app's name 用來之後編號
      for (vector<CollectionFile>::iterator oneCollectFile = collectFileVec->begin();
					 oneCollectFile != collectFileVec->end(); oneCollectFile++)
			{
        for (list<Point>::iterator oneShot = oneCollectFile->pattern.begin();
						 oneShot != oneCollectFile->pattern.end(); oneShot++) 
        {
          addonePoint(*oneShot, &oneCollectFile->appNameVec);
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
#ifdef MERGEFILE_debug_oomAdj_less_than_0
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
#ifdef MERGEFILE_debug_oomAdj_rate_onEachApp
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
#ifdef MERGEFILE_debug_IntervalTime
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
    
			// 檢查 oom_score & oom_adj 差異
#ifdef MERGEFILE_debug_adj_score
			int appNameID = -1;
			//string findName("android.process.media");
			//string findName("jp.co.hit_point.tabikaeru");
			//string findName("com.madhead.tos.zh");
			//string findName("com.nianticlabs.pokemongo");
			//string findName("com.google.android.apps.translate");
			string findName("com.facebook.orca:videoplayer");
			for (int i=0; i<allAppNameVec.size(); i++) {
				if (allAppNameVec[i].compare(0, findName.size(), findName) == 0) {
					appNameID = i;
					break;
				}
			}
			
			if (appNameID == -1) {
				cout << "(error) check part::oom_score & oom_adj: Can't find check App Name!!!" <<endl;
			} else {
				cout << "Find out!! " << appNameID << ":" << allAppNameVec[appNameID] <<endl;
				// 有找到則輸出所有的 oom_adj & oom_score
				bool isFind = false;
				for (vector<Point>::iterator onePoint = allPatternVec.begin();
						 onePoint != allPatternVec.end(); onePoint++)
				{
					Point::App *checkApp = onePoint->getAppWithNamePoint(appNameID);
					if (checkApp != NULL) {
						isFind = true;
						printf("%3d|%6d\n", checkApp->oom_adj, checkApp->oom_score);
					} else if (isFind) {
						isFind = false;
						cout << "out|out" <<endl;
					}
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
				for (pairIter = Ppairs.begin(); pairIter != Ppairs.end(); pairIter++) {
					Point::App *currApp = currPoint->getAppWithIndex(pairIter->first);
					Point::App *nextApp = nextPoint->getAppWithIndex(pairIter->second);
					
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
#ifdef MERGEFILE_debug_oomAdj_statistics
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

#endif /* MERGE_FILE_HPP */