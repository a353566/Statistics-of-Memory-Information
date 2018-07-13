#ifndef DATA_MINING_EXPERIMENT_HPP
#define DATA_MINING_EXPERIMENT_HPP

#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

#define PIN_TESTEND_ON_DATAEND
#define TRAINING_INTERVAL_DAY 21
#define TEST_INTERVAL_DAY 7

//#define EXPERIMENT_debug_Print_Event  // point AppName out

// ----- experiment part -----
#define EXPERIMENT_GSP_normal_part
#define EXPERIMENT_GSP_const_level_part
//#define EXPERIMENT_GSP_multiply_of_level_part
#define EXPERIMENT_GSP_power_of_level_part
//#define EXPERIMENT_LRU_part
//#define EXPERIMENT_MFU_part

#include "DateTime.hpp"
#include "MergeFile.hpp"
#include "SequentialPatternsMining/GSP.hpp"
#include "SequentialPatternsMining/GSP_Predict.hpp"
using namespace std;

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
			cout << "min_support: " << min_support <<endl;
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
				cout << "     |      Num |  1 app  |  2 app  |  3 app  |  4 app  |  5 app  |  6 app  |  7 app  |  8 app  |  9 app  |" <<endl;
				//{ parameter initial | method: weight = base + multiplyNum * level(0~n)
				double base = 1;
				double multiplyNum = 900;
				double multiplyNumMax = 900;
				double multiplyGrow = 1;
				printExperiment multiplyPrint(maxPredictApp);//}
				
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
				//{ parameter initial | method: weight = base + powerNum ^ level(0~n)
				double base = 1;
				double powerNum = 900;
				double powerNumMax = 900;
				double powerGrow = 1;
				printExperiment powerPrint(maxPredictApp);//}
				
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

#endif /* DATA_MINING_EXPERIMENT_HPP */