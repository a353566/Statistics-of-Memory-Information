#ifndef DATA_MINING_EXPERIMENT_HPP
#define DATA_MINING_EXPERIMENT_HPP

#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include "MergeFile.hpp"
#include "GSPMining/GSP.hpp"
#include "GSPMining/GSP_Predict.hpp"
#include "tool/DateTime.hpp"
using namespace std;

// 實驗部分要不要 LOOP 起來，主要是用 static 變數可以慢慢加
//#define MAIN_experiment_LOOP

// add screen status
//#define EXPERIMENT_add_screen_status  // (bug)

// ----- "interval day" of training & test -----
//#define PIN_TESTEND_ON_DATAEND
#define ADD_TIME_SPACE_ON_HEAD // 前面加一些空白時段
	#define START_SPACE_TIME 28
// ----- parameter
#define TRAINING_INTERVAL_DAY 21
#define TEST_INTERVAL_DAY 7
//#define USING_THE_SAME_DATA

// ----- 時間間隔實驗 (hour) // 無視 TRAINING_INTERVAL_DAY 直接從 TRAINING_INTERVAL_increase_time 開始
//#define EXPERIMENT_Interval_time //{
	#ifdef EXPERIMENT_Interval_time
		// == Interval part == (only chooes one)
		#define EXPERIMENT_Interval_day
		//#define EXPERIMENT_Interval_hour
		// xxxxx do not change //{
			#define MAIN_experiment_LOOP
				int TRAINING_INTERVAL_increase_time = 1;
		//}
	#endif
//}
	
// ----- experiment part -----
//#define EXPERIMENT_parameter_part_print_change_result // 只印出有變化的數據

//#define EXPERIMENT_GSP_normal_part
//#define EXPERIMENT_GSP_const_level_part
//#define EXPERIMENT_GSP_multiply_of_level_part
#define EXPERIMENT_GSP_power_of_level_part
#define EXPERIMENT_LRU_part
#define EXPERIMENT_MFU_part
//#define EXPERIMENT_ram_part //{
	#ifdef EXPERIMENT_ram_part
		//#define EXPERIMENT_ram_part_add_old_memory
		// == RAM part == (only chooes one)
		#define EXPERIMENT_ram_part__BEST
		//#define EXPERIMENT_ram_part__LRU
		//#define EXPERIMENT_ram_part__MFU
		//#define EXPERIMENT_ram_part__normal
		//#define EXPERIMENT_ram_part__power
		//#define EXPERIMENT_ram_part__forward_predict //{
			#ifdef EXPERIMENT_ram_part__forward_predict
				// ----- FAN (Forward Application Number) ----- 
				// fast ver. 可以用比較快的版本
				//#define EXPERIMENT_ram_part__forward_predict_has_fast_ver
					#ifdef EXPERIMENT_ram_part__forward_predict_has_fast_ver
						//#define EXPERIMENT_ram_part__forward_predict_fast_ver // 沒註解才有效 (PS:目前沒問題)
					#endif
				// add old memory ver. 可加入舊的記憶體 (要 define 上面的 EXPERIMENT_ram_part_add_old_memory)
				#define EXPERIMENT_ram_part__forward_predict_add_old_memory_ver
				// ----- parameter
				#define EXPERIMENT_FAN 5 // Forward Application Number
				#define EXPERIMENT_increase_FAN // increase FAN
				#define EXPERIMENT_increase_FAN_head 1
				#define EXPERIMENT_increase_FAN_end 9
					// xxxxx do not change //{
						#ifndef EXPERIMENT_increase_FAN // single FAN (not define)
							static int FAN=EXPERIMENT_FAN;
						#else  // increase FAN
							#define MAIN_experiment_LOOP
							static int FAN=EXPERIMENT_increase_FAN_head;
						#endif
					//}
			#endif //}
	#endif//}

//#define EXPERIMENT_GSP_normal_and_power_compare // xxx)

// --- parameter part -----
#define EXPERIMENT_maxBackApp 9 // maxBackApp
#define EXPERIMENT_default_ram 1 // ram
#define EXPERIMENT_ram_head 1 // head to end of ram
#define EXPERIMENT_ram_end 15
#define EXPERIMENT_base 0.00000000001 // power 0.000000001 (8個0)是極限
#define EXPERIMENT_power 5

// ----- display information -----
//#define EXPERIMENT_debug_Print_Event  // print Point AppName
//#define EXPERIMENT_display_add_old_memory  // is "add old memory" success ?
//#define EXPERIMENT_display_detailed_information_of_Memory // 顯示 ram_part 的記憶體內容
//#define EXPERIMENT_display_launch_count_for_each_App // 為每個 APP 統計啟動的數量 (PS:只統計 training data)
	//#define EXPERIMENT_display_launch_count_for_each_App_without_zero // 不要輸出次數為0的
	#define EXPERIMENT_display_launch_count_for_each_App_print_pattern // 輸出所有 pattern
#define EXPERIMENT_display_training_and_test_date // 輸出 training & test 的日期及收集數量
	#define EXPERIMENT_display_training_and_test_date_simple_ver // 是否用簡單版 (上面 define 後才有效果)

//   以下比較少用
//#define EXPERIMENT_display_screen_interval_time // 顯示螢幕亮的次數


class DataMining {
  public :
		// 將結果印出，連續輸出的話可以避開輸出重複的資料
		class Experiment {
			public :
				class Memory {
					public :
						const static int empty = -1;
						int ram;
						int appSize;
						elemType nowUseApp;
						elemType *keepApps;
						elemType extraApp;
						
						int successTimes;
						int failTimes;
						
						Memory() {
							ram = EXPERIMENT_default_ram;
							keepApps = new elemType[ram];
							initial();
						}
						
						Memory(int ram) {
							this->ram = ram;
							keepApps = new elemType[ram];
							initial();
						}
						
						void initial() {
							appSize = 0;
							nowUseApp = empty;
							extraApp = empty;
							for (int i=0; i<ram; i++) {
								keepApps[i] = empty;
							}
							successTimes = 0;
							failTimes = 0;
						}
						
						int fullMemory(vector<elemType> *testDMEventPoint, int seqPoint) {
							int head = seqPoint;
							initial();
							int i;
							for (i=head; true; i++) {// (bug) 沒有特別觀察最多可以放幾個
								if (!addUseApp(testDMEventPoint->at(i))) {
									break;
								}
							}
							successTimes = 0;
							failTimes = 0;
							return i;
						}
						
						bool addUseApp(elemType app) {
							if (nowUseApp == app) {
								cout << "(error) Memory::addUseApp(): The same nowUseApp (" << nowUseApp << ")" <<endl;
								return true;
							}
							
							// 看有沒有 hit
							int which = at(app);
							if (which != empty) { // hit
								nowUseApp = app;
								successTimes++;
								return true;
							} else if (appSize<ram) { // 沒有 hit 但剛好有多餘的空間
								keepApps[at(empty)] = app;
								nowUseApp = app;
								appSize++;
								return true;
							} else if (extraApp == empty) { // 確實 miss
								extraApp = nowUseApp;
								keepApps[at(nowUseApp)] = app;
								nowUseApp = app;
								failTimes++;
								return false;
							} else { // error
								cout << "(error) Memory::addUseApp(): extraApp isn't empty" <<endl;
								return false;
							}
						}
						
						bool update(PredictResult *result) {
							if (extraApp == empty) {
								return false;
							} else {
								multimap<int, elemType> importantMap;
								int important = result->predict(extraApp);
								importantMap.insert(make_pair(important, extraApp));
								for (int i=0; i<ram; i++) {
									if (keepApps[i] == nowUseApp) {
										important = 0;
									} else {
										important = result->predict(keepApps[i]);
									}
									importantMap.insert(make_pair(important, keepApps[i]));
								}
								
								auto app = importantMap.begin();
								for (int i=0; i<ram; i++) {
									keepApps[i] = app->second;
									app++;
								}
								extraApp = empty;
								
								#ifdef EXPERIMENT_display_detailed_information_of_Memory
									printf("%4d: ", failTimes);
									app = importantMap.begin();
									for (int i=0; i<ram; i++) {
										printf("%6d:%3d |", app->first, app->second);
										app++;
									}
									printf("- %6d:%3d\n", app->first, app->second);
								#endif
								
								return true;
							}
						}
						
						int at(elemType app) {
							for (int i=0; i<ram; i++) {
								if (keepApps[i] == app) {
									return i;
								}
							}
							return empty;
						}
						
						void print() {
							int totalTimes = successTimes + failTimes;
							printf("%1.5f | %4d | ", (double)successTimes/totalTimes, totalTimes);
						}
						
				};
				
				string preStr;
				int maxBackApp;
				int maxPredictApp;
				int failedTimes;
				int *successTimes;
				
				int seqPoint;
				vector<elemType> *testDMEventPoint;
				bool recordMem;
				Memory memory;
				
				// preData
				bool preData;
				vector<elemType> *preEventPoint;
				
				Experiment(int maxBackApp, int maxPredictApp) {
					this->maxBackApp = maxBackApp;
					this->maxPredictApp = maxPredictApp;
					recordMem = false;
					preData = false;
					initial();
				}
				
				Experiment(int maxBackApp, int maxPredictApp, vector<elemType> *testDMEventPoint) {
					this->maxBackApp = maxBackApp;
					this->maxPredictApp = maxPredictApp;
					this->testDMEventPoint = testDMEventPoint;
					recordMem = false;
					preData = false;
					initial();
				}
				
				Experiment(int maxBackApp, int maxPredictApp, vector<elemType> *testDMEventPoint, int ram) {
					this->maxBackApp = maxBackApp;
					this->maxPredictApp = maxPredictApp;
					this->testDMEventPoint = testDMEventPoint;
					recordMem = true;
					preData = false;
					memory = Memory(ram);
					initial();
				}
				
				void initial() {
					preStr.clear();
					failedTimes = 0;
					successTimes = new int[maxPredictApp];
					for (int i=0; i<maxPredictApp; i++) {
						successTimes[i] = 0;
					}
					seqPoint = maxBackApp;
					if (recordMem) {
						buildMemory();
					}
				}
				
				void buildMemory() {
					// 裝滿記憶體。如果太多的話，回傳給 seqPoint
					seqPoint = memory.fullMemory(testDMEventPoint, seqPoint);
				}
				
				// 看有沒有下一個
				bool next() {
					seqPoint++;
					return seqPoint < testDMEventPoint->size()-1;  // -1是要被預測的那一個
				}
				
				// return shortSeq
				Sequence buildShortSeq() {
					Sequence shortSeq;
					// 整理 sequence
					int start = (seqPoint>=maxBackApp) ? seqPoint-maxBackApp : 0;
					for (start; start<=seqPoint; start++) {
						shortSeq += testDMEventPoint->at(start);
					}
					return shortSeq;
				}
				
				// 更新預測資料
				int updatePrediction(PredictResult *result) {
					int predictApp = PredictResult::PREDICT_MISS;
					elemType reallyUseApp = testDMEventPoint->at(seqPoint+1);
					// 依照 result 更新記憶體
					if (recordMem) {
						memory.update(result);
						memory.addUseApp(reallyUseApp);
					}
					
					int predict = result->predict(reallyUseApp, maxPredictApp);
					if (predict == PredictResult::PREDICT_MISS) {
						failedTimes++;
					} else {
						successTimes[predict-1]++;
						predictApp = predict;
					}
					return predictApp;
				}
				
				//  ┌----------------┐
				//  | print function |
				/** └----------------┘
				 *  將結果印出 只印出有改變的兩段資訊
				 *  1. 檢查有沒有變化
				 *  1.true  : 有則輸出，並記錄 newSuccessTimes、newFailedTimes 到自己的資料
				 *  1.false : preStr <= newPreStr
				 *  參數同上 (Experiment)
				 *   | successTimes : 成功的次數，可以往後讀 maxPredictApp 個
				 *   | failedTimes : 往回 maxPredictApp 個都沒有命中
				 */ //{
				void printImportant(Experiment *experiment, string *newPreStr) {
					printImportant(experiment->successTimes, experiment->failedTimes, newPreStr);
				}
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
					print(false);
				}
				void print() {
					print(true);
				}
				void print(bool printTitle) {
					if (printTitle) {
						cout << "    predict App Num & predict rate     " <<endl;
						cout << " 1 app  |  2 app  |  3 app  | .......  " <<endl;
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
				void memoryPrint() {
					memory.print();
				}
				/** 主要是印出最後的資料
				 */
				void finalPrint() {
					// check this data isn't print
					if (!preStr.empty()) {
						print(&preStr);
						preStr.clear();
					}
				} //}
				
		};
		
    string screen_turn_on;
    string screen_turn_off;
    string screen_long_off;
    elemType EPscreen_turn_on;
    elemType EPscreen_turn_off;
    elemType EPscreen_long_off;
    DateTime intervalTime;
    vector<elemType> trainingDMEventPoint;
    vector<elemType> testDMEventPoint;
    vector<string> DMEPtoEvent; // Data Mining Each Point to Event
		
    DataMining() {
      screen_turn_on = string("screen turn on");
      screen_turn_off = string("screen turn off");
      screen_long_off = string("screen long off");
      // 看螢幕"暗著"的時間間隔
      intervalTime.initial();
      intervalTime.hour = 1;
    }
		
		//  ┌----------------┐
		//  | build function |
		/** └----------------┘
		 *  建立 training & test data & DMEPtoEvent
		 *  讓後面的 experiment 可以運作
		 *  1. Build DMEPtoEvent
		 *  2. Segment eventVec into screenPattern that the screen is "ON" : 主要是將 screen 亮著的區間做間隔
		 *  3. Divide eventVec into two data : 將 screenPattern 用時間分成 training test 兩份
		 *  eventVec : 所有事件
		 */ //{
    void build(vector<Event> *eventVec, vector<string> *allAppNameVec) {
			// (check) screen state
			checkScreenState(eventVec);
			
      // ----- 1. Build DMEPtoEvent
			buildDMEPtoEvent(allAppNameVec);

			//{ ----- 2. Segment eventVec into screenPattern that the screen is "ON" : 主要是將 screen 亮著的區間做間隔
      vector<pair<int, int> > screenPattern = segmentScreenPattern(eventVec);

#ifdef EXPERIMENT_display_screen_interval_time
			cout << "Screen on->off interval have (" << screenPattern.size() << ") times" <<endl;//}
#endif

			// ----- 3. Divide eventVec into two data : 將 screenPattern 用時間分成 training test 兩份
			buildData(&screenPattern, eventVec);
    }
		
		/** 利用螢幕分隔(screenPattern)來切成兩個時間段
		 *  並將其資料分別裝入 trainingDMEventPoint、testDMEventPoint
		 *  1. defind Start & End on two data : 找出兩種資料的開頭和結尾
		 *  2. push "screen_ON interval" into two datas : 將 screen_ON interval 裝進 兩個資料 (trainingScnPatn & testScnPatn)
		 *  3. Build two data : 利用 "screen_ON interval" 整理 兩筆資料 (trainingDMEventPoint & testDMEventPoint)
		 *  screenPattern : 螢幕亮著的區間
		 *  eventVec : 使用 APP 流程
		 *  TRAINING_INTERVAL_DAY, TEST_INTERVAL_DAY : 分別為 training, test 時間
		 *  (bug) 一定要小於 TrainingIT.day & TestIT.day，沒有寫防呆
		 */
		void buildData(vector<pair<int, int> > *screenPattern, vector<Event> *eventVec) {
			//{ initial
			vector<pair<int, int> > trainingScnPatn, testScnPatn;
			int trainingStart = 0;
			int trainingEnd = 0;
			int testStart = 0;
			int testEnd = screenPattern->size()-1;
			DateTime TrainingIT, TestIT; // 設定 training & test data 要多久 (ps: IT = interval time)
			
#ifdef EXPERIMENT_Interval_time
	#ifdef EXPERIMENT_Interval_hour
			TrainingIT.initial_hour(TRAINING_INTERVAL_increase_time);
	#endif
	#ifdef EXPERIMENT_Interval_day
			TrainingIT.initial_Day(TRAINING_INTERVAL_increase_time);
	#endif
#else
			TrainingIT.initial_Day(TRAINING_INTERVAL_DAY);
#endif
			
			TestIT.initial_Day(TEST_INTERVAL_DAY);//}
			
			// ----- 1. defind Start & End on two data : 找出兩種資料的開頭和結尾
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
			DateTime *headTime;
	#ifdef ADD_TIME_SPACE_ON_HEAD
			int headSpaceStart = 0;
			int headSpaceEnd = 0;
			DateTime StartSpaceT;
			StartSpaceT.initial_Day(START_SPACE_TIME);
			headTime = eventVec->at(screenPattern->at(headSpaceStart).first).thisDate;
			for (headSpaceEnd = headSpaceStart; headSpaceEnd<eventVec->size(); headSpaceEnd++) {
				DateTime *endTime = eventVec->at(screenPattern->at(headSpaceEnd).second).thisDate;
				if (*endTime - *headTime > StartSpaceT) { // (bug) 一定要小於 TrainingIT.day，沒有寫防呆
					break;
				}
			}
			trainingStart = headSpaceEnd+1;
	#endif
			
			headTime = eventVec->at(screenPattern->at(trainingStart).first).thisDate;
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
			
			//{ ----- 2. push "screen_ON interval" into two datas : 將 screen_ON interval 裝進 兩個資料 (trainingScnPatn & testScnPatn)
			for (int i=trainingStart; i<=trainingEnd; i++) {
				trainingScnPatn.push_back(screenPattern->at(i));
			}
			testStart = trainingEnd+1; // 通常
			for (int i=testStart; i<=testEnd; i++) {
				testScnPatn.push_back(screenPattern->at(i));
			}//}
			
      //{ ----- 3. Build two data : 利用 "screen_ON interval" 整理 兩筆資料 (trainingDMEventPoint & testDMEventPoint)
      buildDMEventPoint(&trainingDMEventPoint, &trainingScnPatn, eventVec);
			
#ifdef EXPERIMENT_display_launch_count_for_each_App
			{
				int appCountArray[DMEPtoEvent.size()];
				for (int i=0; i<DMEPtoEvent.size(); i++) { appCountArray[i]=0; }
				// 開始數
				for (int i=0; i<trainingDMEventPoint.size(); i++) {
					// output pattern
	#ifdef EXPERIMENT_display_launch_count_for_each_App_print_pattern
					printf("%4d|", trainingDMEventPoint.at(i));
	#endif
					appCountArray[trainingDMEventPoint.at(i)]++;
				}
	#ifdef EXPERIMENT_display_launch_count_for_each_App_print_pattern
					printf("\n");
	#endif
				// output
				for (int i=0; i<DMEPtoEvent.size(); i++) {
	#ifdef EXPERIMENT_display_launch_count_for_each_App_without_zero
					if (appCountArray[i] == 0) { continue; }
	#endif
					printf("%4d |%6d |", i, appCountArray[i]);
					cout << DMEPtoEvent.at(i) <<endl;
				}
			}
#endif
			
#ifndef USING_THE_SAME_DATA
      buildDMEventPoint(&testDMEventPoint, &testScnPatn, eventVec);
#else
      buildDMEventPoint(&testDMEventPoint, &trainingScnPatn, eventVec);
#endif//}
		
#ifdef EXPERIMENT_ram_part_add_old_memory
	#ifdef EXPERIMENT_display_add_old_memory
			cout << " trainingDMEventPoint      :" <<endl;
			for (auto temp =trainingDMEventPoint.begin(); temp!= trainingDMEventPoint.end(); temp++) {
				printf("%4d,", *temp);
			}
			
			cout << "\n testDMEventPoint          :" <<endl;
			for (auto temp =testDMEventPoint.begin(); temp!= testDMEventPoint.end(); temp++) {
				printf("%4d,", *temp);
			}
	#endif
			
			// 裝一下 testDMEventPoint 前 EXPERIMENT_maxBackApp 個
			bool repeat = false;
			int trainingSize = trainingDMEventPoint.size();
			if (*testDMEventPoint.begin() == trainingDMEventPoint.at(trainingSize-1)) {
				testDMEventPoint.erase(testDMEventPoint.begin());
				repeat = true;
			}
			for (int it=0; it<=EXPERIMENT_maxBackApp; it++) {
				testDMEventPoint.insert(testDMEventPoint.begin(), trainingDMEventPoint.at(trainingSize -1 -it));
			}
			
	#ifdef EXPERIMENT_display_add_old_memory
			cout << "\n testDMEventPoint add ver. :" <<endl;
			for (auto temp =testDMEventPoint.begin(); temp!= testDMEventPoint.end(); temp++) {
				printf("%4d,", *temp);
			}
			cout <<endl;
	#endif
#endif
			
			//{ output
#ifdef EXPERIMENT_display_training_and_test_date
	#ifdef EXPERIMENT_display_training_and_test_date_simple_ver
			// training
			eventVec->at(screenPattern->at(trainingStart).first).thisDate->output_colon_ver();
			printf(" | ");
			eventVec->at(screenPattern->at(trainingEnd).second).thisDate->output_colon_ver();
			printf(" | %5lu | ", trainingDMEventPoint.size());
			// test
			eventVec->at(screenPattern->at(testStart).first).thisDate->output_colon_ver();
			printf(" | ");
			eventVec->at(screenPattern->at(testEnd).second).thisDate->output_colon_ver();
			printf(" | %5lu", testDMEventPoint.size());
	#else
			// training
			cout << "Training action data is between ";
			eventVec->at(screenPattern->at(trainingStart).first).thisDate->output_colon_ver();
			cout << " and\n                                ";
			eventVec->at(screenPattern->at(trainingEnd).second).thisDate->output_colon_ver();
			printf("\nTraining data mining event point have (%5lu) times\n", trainingDMEventPoint.size());
			// test
			cout << "Test action data is between ";
			eventVec->at(screenPattern->at(testStart).first).thisDate->output_colon_ver();
			cout << " and\n                            ";
			eventVec->at(screenPattern->at(testEnd).second).thisDate->output_colon_ver();
			printf("\nTest data mining event point have (%5lu) times\n", testDMEventPoint.size());
	#endif
#endif//}
		}
		
    /** 如果有遇到一次發現多的 app 執行的話
     *  就用 "oom_score" 來判斷先後 數字大的先
		 *  最後 刪除重複的資料
     *  DMEventPoint : Push data into DMEventPoint(Data Event Point).
     *  usePattern : The data is according to the Screen Changed.
     *  eventVec : Event data
     */
		void buildDMEventPoint(vector<elemType> *DMEventPoint, vector<pair<int, int> > *usePattern, vector<Event> *eventVec) {
      // 用 usePattern 來取出
      for (vector<pair<int, int> >::iterator useInterval = usePattern->begin();
			     useInterval != usePattern->end(); useInterval++) {
				// 順序是先
				// 1. check screen, if turn on, record it
				// 2. record app
				// 3. check screen, if turn off, record it
				
				// ----- 1. check screen, if turn on, record it
#ifdef EXPERIMENT_add_screen_status
				DMEventPoint->push_back(EPscreen_turn_on);
#endif

        //{ ----- 2. record app
        for (int EP=useInterval->first; EP<=useInterval->second; EP++) { // PS: EP = EventPoint
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
                cout << "(error) DataMining::buildDMEventPoint() record too much!!!" <<endl;
              }
              DMEventPoint->push_back(big_app_name);
              app_record[big_app_num] = true;
            }
            for (int j=0; j<caseVec->size(); j++) {
              if (!app_record[j]) {
								cout << "(error) DataMining::buildDMEventPoint() something don't record ???" <<endl;
							}
            }
          }//}
        }//}
				
				// ----- 3. check screen, if turn off, record it
#ifdef EXPERIMENT_add_screen_status
				DMEventPoint->push_back(EPscreen_turn_off);
#endif
      }
			
			// ----- final. remove the same action
			removeSameAction(DMEventPoint);
    }
		
		/** 將 training、test 中連續一樣的資料刪掉
		 *  以免後面出現使用連續兩個一樣的 APP
		 */
		void removeSameAction(vector<elemType> *DMEventPoint) {
			vector<elemType>::iterator lastIter = DMEventPoint->begin();
			vector<elemType>::iterator thisIter = DMEventPoint->begin();
			thisIter++;
			while (thisIter != DMEventPoint->end()) {
				if (*lastIter != *thisIter) {
					// 不一樣的話沒事
					lastIter++;
					thisIter++;
				} else {
					// 一樣的話要刪掉 thisIter
					thisIter = DMEventPoint->erase(thisIter);
				}
			}
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
		
		// 建立 DMEPtoEvent 並加入其他 point (ex: screen status)
		void buildDMEPtoEvent(vector<string> *allAppNameVec) {
			// 放 app name & screen status
			for (int i=0; i<allAppNameVec->size(); i++) {
				DMEPtoEvent.push_back(allAppNameVec->at(i));
			}
			
			// push screen on & off into DMEPtoEvent ("screen turn on", "screen turn off", "screen long off")
			DMEPtoEvent.push_back(screen_turn_on);
			EPscreen_turn_on = DMEPtoEvent.size()-1;
			DMEPtoEvent.push_back(screen_turn_off);
			EPscreen_turn_off = DMEPtoEvent.size()-1;
			DMEPtoEvent.push_back(screen_long_off);
			EPscreen_long_off = DMEPtoEvent.size()-1;
			
			// print Point AppName
#ifdef EXPERIMENT_debug_Print_Event
			for (int i=0; i<DMEPtoEvent.size(); i++)
				cout << i << "\t:" << DMEPtoEvent[i] <<endl;
#endif
		}
		
		/** 取得 screen 開關 Pattern
		 *  主要是取得 on -> off 也就是亮著的時間間隔
		 */
		vector<pair<int, int> > segmentScreenPattern(vector<Event> *eventVec) {
			vector<pair<int, int> > screenPattern;
      // 看螢幕"暗著"的時間間隔
      pair<int, int> onePattern;
      int pI = 0;
      // PI 有起頭之後就用 while 跑
      while (pI<eventVec->size()) {
				onePattern = make_pair(0,0);
        // 找接下來變"亮"的時候
        for (pI; pI<eventVec->size(); pI++) {
          if (eventVec->at(pI).isThisScreenOn) {
						onePattern.first = pI;
            break;
          }
        }
				
        // 找接下來變"暗"的時候，並登記前一個狀態 (亮的時刻)
        for (pI; pI<eventVec->size(); pI++) {
          if (!eventVec->at(pI).isThisScreenOn) {
            onePattern.second = pI-1;
            break;
          }
        }
				
        // 找到亮暗的區間
        // 先檢察是不是還在 pattern 範圍內，是的話就記錄，不是就 break
        if (pI < eventVec->size()) {
          screenPattern.push_back(onePattern);
        } else {
					break;
				}
      }
			
			return screenPattern;
		}//}
		
		//  ┌---------------------┐
		//  | Experiment function |
		/** └---------------------┘
		 *  1. GSP sequential patterns mining
		 *  2. Experiment 
		 *   | 1) GSP normal Algorithm
		 *   | 2) GSP const level Algorithm
		 *   | 3) GSP multiply of level Algorithm
		 *   | 4) GSP power of level Algorithm
		 *   | 5) LRU
		 *   | 6) MFU
		 *   | 7) RAM
		 *   | xxx) GSP normal & power compare
		 */ //{
		void experiment() {
      //{ ----- 1. GSP sequential patterns mining
			//int min_support = trainingDMEventPoint.size()/200; // (0.5%)
			int min_support = 2;
			//cout << "min_support: " << min_support <<endl;
      GSP gsp(trainingDMEventPoint, min_support);
      gsp.Mining();
			vector<elemType> filterVec;
			filterVec.push_back(EPscreen_turn_on);
			filterVec.push_back(EPscreen_turn_off);
			//filterVec.push_back(EPscreen_long_off);
			gsp.Filter(&filterVec);
			
			// (print) GSP statistics
			//gsp.OutputAll();
			//gsp.OutputAllTop();//}
			
			// ----- 2. Experiment 
			//{ initial
			GSP_Predict gspPredict(&gsp);
			const int maxBackApp = EXPERIMENT_maxBackApp;
			const int maxPredictApp = 15;
			int failedTimes;
			int successTimes[maxPredictApp];//}
			
			// 1) GSP normal Algorithm
#ifdef EXPERIMENT_GSP_normal_part
			{ cout << " ----- GSP normal predict:" <<endl;
				// initial
				Experiment experiment(maxBackApp, maxPredictApp, &testDMEventPoint);
				
				// predict
				do {
					Sequence shortSeq = experiment.buildShortSeq();
					PredictResult result = gspPredict.predictResult_normal(&shortSeq, maxPredictApp);
					experiment.updatePrediction(&result);
				} while (experiment.next());
				// output
				experiment.print();
				cout<<endl;
			}
#endif
			
			// 2) GSP const level Algorithm
#ifdef EXPERIMENT_GSP_const_level_part
			{ cout << " ----- GSP const level predict:" <<endl;
				cout << "peep | Total   |         |                            predict rate on difference Level                             |" <<endl;
				cout << " App | pattern |   MFU   | Level 1 | Level 2 | Level 3 | Level 4 | Level 5 | Level 6 | Level 7 | Level 8 | Level 9 |" <<endl;
				//{ initial
				failedTimes = 0;
				for (int i=0; i<maxPredictApp; i++) {
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
						double parameter = level;
						PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::ConstLevel, &parameter, &shortSeq, maxPredictApp);
						
						// 檢查確實有結果才繼續，否則跳出換下一個
						elemType predictApp = result.resultPairs.at(0).first;
						if (predictApp != PredictResult::NO_APP) {
							if (predictApp == reallyUseApp) {
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
					int successLevelTimes[maxBackApp];
					for (int i=0; i<maxBackApp; i++) {
						successLevelTimes[i] = 0;
					}
					vector<int> catchSeqVec; // 知道有哪些是有搜尋到的內容
					failedTimes = 0;//}
					
					//{ 先找自己這層有多少個 & 沒有則跳出
					for (int i=0; i<testDMEventPoint.size(); i++) {
						if (predictLevelMap[i][level] != EMPTY) {
							catchSeqVec.push_back(i);
							if (predictLevelMap[i][level] != PREDICT_FAIL) {
								successLevelTimes[level]++;
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
								successLevelTimes[downLevel]++;
							} else {
								failedTimes++;
							}
						}
					}//}
					
					//{ output
					int totalPattern = catchSeqVec.size();
					printf("%4d | %7d | ", level, totalPattern);
					for (int i = 0; i<=level; i++) {
						printf("%1.5f | ", (double)successLevelTimes[i]/totalPattern);
					}
					cout<<endl;//}
				}
				cout <<endl;//}
			}
#endif
			
			// 3) GSP multiply of level Algorithm
#ifdef EXPERIMENT_GSP_multiply_of_level_part
			{ cout << " ----- GSP multiply predict rate:" <<endl;
				cout << "base | multiply |     predict App Num & predict rate     " <<endl;
				cout << "     |      Num |  1 app  |  2 app  |  3 app  | .......  " <<endl;
				//{ parameter initial | method: weight = base + multiplyNum * level(0~n)
				double base = 0.00000001;
				double multiplyNum = 1;
				double multiplyNumMax = 1;
				double multiplyGrow =   1;
				Experiment lastPrint(maxBackApp, maxPredictApp);//}
				
				for (multiplyNum; multiplyNum <= multiplyNumMax; multiplyNum+=multiplyGrow) {
					// initial
					Experiment experiment(maxBackApp, maxPredictApp, &testDMEventPoint);
					
					//{ predict
					double parameter[2];
					parameter[0] = base;
					parameter[1] = multiplyNum;
					do {
						Sequence shortSeq = experiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_byMethod(GSP_Predict::multiplyOfLevle, parameter, &shortSeq, maxPredictApp);
						experiment.updatePrediction(&result);
					} while (experiment.next());//}
					
					//{ output
					char outChar[64];
					snprintf(outChar, 64, "%.2f | %.5f | ", base, multiplyNum);
					string str = string(outChar);
					lastPrint.printImportant(&experiment, &str);//}
				}
				lastPrint.finalPrint();
				cout<<endl;
			}
#endif
			
			// 4) GSP power of level Algorithm
#ifdef EXPERIMENT_GSP_power_of_level_part
			{ cout << " ----- GSP power predict rate:" <<endl;
				cout << "base |   power |     predict App Num & predict rate     " <<endl;
				cout << "     |     Num |  1 app  |  2 app  |  3 app  | .......  " <<endl;
				//{ parameter initial | method: weight = base + powerNum ^ level(0~n)
				double base = 0.00000001;
				double powerNum = 5;
				double powerNumMax = 5;
				double powerGrow = 0.05;
				Experiment lastPrint(maxBackApp, maxPredictApp);//}
				
				for (powerNum; powerNum <= powerNumMax; powerNum+=powerGrow) {
					// initial
					Experiment experiment(maxBackApp, maxPredictApp, &testDMEventPoint);
					
					//{ predict
					double parameter[2];
					parameter[0] = base;
					parameter[1] = powerNum;
					do {
						Sequence shortSeq = experiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_byMethod(
						                                    GSP_Predict::powerOfLevel,
																								parameter,
																								&shortSeq,
																								maxPredictApp);
						experiment.updatePrediction(&result);
					} while (experiment.next());//}
					
					//{ output
	#ifndef EXPERIMENT_parameter_part_print_change_result
					printf("%.2f | %.5f | ", base, powerNum);
					experiment.print(false);
	#else
					char outChar[64];
					snprintf(outChar, 64, "%.2f | %.5f | ", base, powerNum);
					string str = string(outChar);
					lastPrint.printImportant(&experiment, &str);
	#endif//}
				}
	#ifdef EXPERIMENT_parameter_part_print_change_result
				lastPrint.finalPrint();
	#endif
				cout<<endl;
			}
#endif
			
			// 5) LRU
#ifdef EXPERIMENT_LRU_part
			{ cout << " ----- LRU predict:" <<endl;
				// initial
				Experiment experiment(maxBackApp, maxPredictApp, &testDMEventPoint);
				
				// predict
				do {
					PredictResult result;
					// build LRU result
					for (int i=experiment.seqPoint-1; i>=0; i--) {
						if (testDMEventPoint.at(experiment.seqPoint) != testDMEventPoint.at(i) && !result.checkRepeat(testDMEventPoint.at(i))) {
							result.resultPairs.push_back(make_pair(testDMEventPoint.at(i), i));
							if (result.size() >= maxPredictApp) {
								break;
							}
						}
					}
					experiment.updatePrediction(&result);
				} while (experiment.next());
				
				// output
				experiment.print();
				cout<<endl;
			}
#endif
			
			// 6) MFU
#ifdef EXPERIMENT_MFU_part
			{ cout << " ----- MFU predict:" <<endl;
				// initial
				Experiment experiment(maxBackApp, maxPredictApp, &testDMEventPoint);
				
				// predict
				double parameter = 0;	// level = 0
				do {
					Sequence shortSeq = experiment.buildShortSeq();
					PredictResult result = gspPredict.predictResult_byMethod(
					                                    GSP_Predict::ConstLevel,
																							&parameter,
																							&shortSeq,
																							maxPredictApp);
					experiment.updatePrediction(&result);
				} while (experiment.next());
				
				// output
				experiment.print();
				cout<<endl;
			}
#endif
			
			// 7) RAM
#ifdef EXPERIMENT_ram_part
			{
				//{ tital output
#ifdef EXPERIMENT_ram_part__BEST
				cout << " ----- RAM experiment for BEST: ";
#endif
#ifdef EXPERIMENT_ram_part__LRU
				cout << " ----- RAM experiment for LRU: ";
#endif
#ifdef EXPERIMENT_ram_part__MFU
				cout << " ----- RAM experiment for MFU: ";
#endif
#ifdef EXPERIMENT_ram_part__normal
				cout << " ----- RAM experiment for normal: ";
#endif
#ifdef EXPERIMENT_ram_part__power
				cout << " ----- RAM experiment for power: ";
#endif
#ifdef EXPERIMENT_ram_part__forward_predict
				cout << " ----- RAM experiment for power_more_predict: FAN=" << FAN;
#endif
				cout <<endl;//}
				
				const int maxPredictAppForRam = 200;
				// 各種 ram 大小
				for (int ram = EXPERIMENT_ram_head; ram<=EXPERIMENT_ram_end; ram++) {
					// initial
					Experiment experiment(maxBackApp, maxPredictAppForRam, &testDMEventPoint, ram);
					// predict
#ifdef EXPERIMENT_ram_part__BEST
	#ifdef EXPERIMENT_ram_part_add_old_memory
					// ----- old Memory //mason
					Experiment oldexperiment(maxBackApp, maxPredictAppForRam, &trainingDMEventPoint, ram);
					do {
						// build bast result
						PredictResult result;
						int nowApp = trainingDMEventPoint.at(oldexperiment.seqPoint);
						for (int i=oldexperiment.seqPoint+1; i<trainingDMEventPoint.size(); i++) {
							int app = trainingDMEventPoint.at(i);
							if (nowApp != app && !result.checkRepeat(app)) {
								result.resultPairs.push_back(make_pair(app, 10000000-i));
							}
						}
						oldexperiment.updatePrediction(&result);
					} while (oldexperiment.next());
					// 從新設定
					experiment.memory = oldexperiment.memory;
					experiment.memory.successTimes = 0;
					experiment.memory.failTimes = 0;
					experiment.seqPoint = maxBackApp;
	#endif
					do {
						// build bast result
						PredictResult result;
						int nowApp = testDMEventPoint.at(experiment.seqPoint);
						for (int i=experiment.seqPoint+1; i<testDMEventPoint.size(); i++) {
							int app = testDMEventPoint.at(i);
							if (nowApp != app && !result.checkRepeat(app)) {
								result.resultPairs.push_back(make_pair(app, 10000000-i));
							}
						}
						experiment.updatePrediction(&result);
					} while (experiment.next());
#endif
#ifdef EXPERIMENT_ram_part__LRU
	#ifdef EXPERIMENT_ram_part_add_old_memory
					// ----- old Memory //mason
					Experiment oldexperiment(maxBackApp, maxPredictAppForRam, &trainingDMEventPoint, ram);
					do {
						// build LRU result
						PredictResult result;
						for (int i=oldexperiment.seqPoint-1; i>=0; i--) {
							if (trainingDMEventPoint.at(oldexperiment.seqPoint) != trainingDMEventPoint.at(i) && !result.checkRepeat(trainingDMEventPoint.at(i))) {
								result.resultPairs.push_back(make_pair(trainingDMEventPoint.at(i), i));
							}
						}
						oldexperiment.updatePrediction(&result);
					} while (oldexperiment.next());
					// 從新設定
					experiment.memory = oldexperiment.memory;
					experiment.memory.successTimes = 0;
					experiment.memory.failTimes = 0;
					experiment.seqPoint = maxBackApp;
	#endif
					do {
						// build LRU result
						PredictResult result;
						for (int i=experiment.seqPoint-1; i>=0; i--) {
							if (testDMEventPoint.at(experiment.seqPoint) != testDMEventPoint.at(i) && !result.checkRepeat(testDMEventPoint.at(i))) {
								result.resultPairs.push_back(make_pair(testDMEventPoint.at(i), i));
							}
						}
						experiment.updatePrediction(&result);
					} while (experiment.next());
#endif
#ifdef EXPERIMENT_ram_part__MFU
					double parameter = 0;	// level = 0
	#ifdef EXPERIMENT_ram_part_add_old_memory
					// ----- old Memory //mason
					Experiment oldexperiment(maxBackApp, maxPredictAppForRam, &trainingDMEventPoint, ram);					
					do {
						// build MFU result
						Sequence shortSeq = oldexperiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_byMethod(
						                                    GSP_Predict::ConstLevel,
																								&parameter,
																								&shortSeq,
																								maxPredictAppForRam);
						oldexperiment.updatePrediction(&result);
					} while (oldexperiment.next());
					
					// 從新設定
					experiment.memory = oldexperiment.memory;
					experiment.memory.successTimes = 0;
					experiment.memory.failTimes = 0;
					experiment.seqPoint = maxBackApp;
	#endif
					do {
						// build MFU result
						Sequence shortSeq = experiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_byMethod(
						                                    GSP_Predict::ConstLevel,
																								&parameter,
																								&shortSeq,
																								maxPredictAppForRam);
						experiment.updatePrediction(&result);
					} while (experiment.next());
#endif
#ifdef EXPERIMENT_ram_part__normal
					do {
						Sequence shortSeq = experiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_normal(&shortSeq, maxPredictApp);
						experiment.updatePrediction(&result);
					} while (experiment.next());
#endif
#ifdef EXPERIMENT_ram_part__power
					// parametor
					double base = EXPERIMENT_base;
					double power = EXPERIMENT_power;
					double parameter[2];
					parameter[0] = base;
					parameter[1] = power;
					do {
						// build power result
						Sequence shortSeq = experiment.buildShortSeq();
						PredictResult result = gspPredict.predictResult_byMethod(
																								GSP_Predict::powerOfLevel,
																								parameter,
																								&shortSeq,
																								maxPredictAppForRam);
						experiment.updatePrediction(&result);
					} while (experiment.next());
#endif

// two ver => fast ver. or add old memory ver.
#ifdef EXPERIMENT_ram_part__forward_predict
	#ifdef EXPERIMENT_ram_part__forward_predict_has_fast_ver
					// parametor
					double base = 0; // on below
					double power = EXPERIMENT_power;
					double parameter[2];
					parameter[0] = base;
					parameter[1] = power;
					double parameterForMFU = 0; // level = 0
		#ifdef EXPERIMENT_ram_part__forward_predict_fast_ver
					// 靜態宣告 才不會重跑就消失
					static map<int, PredictResult> allResult;
					if (ram==1) { // ram 為 1 的時候，需要重新設定
						allResult.clear();
					}
		#endif
					do {
						Sequence shortSeq = experiment.buildShortSeq();
		#ifndef EXPERIMENT_ram_part__forward_predict_fast_ver
						// MFU result
						PredictResult MFUresult = gspPredict.predictResult_byMethod(
																									GSP_Predict::ConstLevel,
																									&parameterForMFU,
																									&shortSeq,
																									maxPredictAppForRam);
						// build power result
						PredictResult result = gspPredict.predictResult_forwardPredict_byMethod(
																								FAN, // Forward App Number
																								GSP_Predict::powerOfLevel,
																								parameter,
																								&shortSeq,
																								maxPredictAppForRam);
						MFUresult.rateBase();
						MFUresult *= EXPERIMENT_base;
						result += MFUresult;
						result.sort();
						experiment.updatePrediction(&result);
		#else
						// 第一次跑的話要從新跑結果
						if (ram==1) {
							// MFU result
							PredictResult MFUresult = gspPredict.predictResult_byMethod(
																										GSP_Predict::ConstLevel,
																										&parameterForMFU,
																										&shortSeq,
																										maxPredictAppForRam);
							// build power result
							PredictResult result = gspPredict.predictResult_forwardPredict_byMethod(
																									FAN, // Forward App Number
																									GSP_Predict::powerOfLevel,
																									parameter,
																									&shortSeq,
																									maxPredictAppForRam);
							MFUresult.rateBase();
							MFUresult *= EXPERIMENT_base;
							result += MFUresult;
							result.sort();
							allResult[experiment.seqPoint] = result;
						}
						experiment.updatePrediction(&allResult[experiment.seqPoint]);
		#endif
					} while (experiment.next());
	#endif
	#ifdef EXPERIMENT_ram_part__forward_predict_add_old_memory_ver
					
					// parametor
					double base = 0; // on below
					double power = EXPERIMENT_power;
					double parameter[2];
					parameter[0] = base;
					parameter[1] = power;
					double parameterForMFU = 0; // level = 0
					
		#ifdef EXPERIMENT_ram_part_add_old_memory
					// ----- old Memory //mason
					Experiment oldexperiment(maxBackApp, maxPredictAppForRam, &trainingDMEventPoint, ram);
					// 靜態宣告 才不會重跑就消失
					static map<int, PredictResult> oldallResult;
					if (ram==1) { // ram 為 1 的時候，需要重新設定
						oldallResult.clear();
					}
					do {
						Sequence shortSeq = oldexperiment.buildShortSeq();
						// 第一次跑的話要從新跑結果
						if (ram==1) {
							// MFU result
							PredictResult MFUresult = gspPredict.predictResult_byMethod(
																										GSP_Predict::ConstLevel,
																										&parameterForMFU,
																										&shortSeq,
																										maxPredictAppForRam);
							// build power result
							PredictResult result = gspPredict.predictResult_forwardPredict_byMethod(
																									FAN, // Forward App Number
																									GSP_Predict::powerOfLevel,
																									parameter,
																									&shortSeq,
																									maxPredictAppForRam);
							MFUresult.rateBase();
							MFUresult *= EXPERIMENT_base;
							result += MFUresult;
							result.sort();
							oldallResult[oldexperiment.seqPoint] = result;
						}
						oldexperiment.updatePrediction(&oldallResult[oldexperiment.seqPoint]);
					} while (oldexperiment.next());
					
					// 從新設定
					//mason 看要不要裝一下 experiment 前九個
					experiment.memory = oldexperiment.memory;
					experiment.memory.successTimes = 0;
					experiment.memory.failTimes = 0;
					experiment.seqPoint=maxBackApp;
		#endif
					
					// 靜態宣告 才不會重跑就消失
					static map<int, PredictResult> allResult;
					if (ram==1) { // ram 為 1 的時候，需要重新設定
						allResult.clear();
					}
					do {
						Sequence shortSeq = experiment.buildShortSeq();
						// 第一次跑的話要從新跑結果
						if (ram==1) {
							// MFU result
							PredictResult MFUresult = gspPredict.predictResult_byMethod(
																										GSP_Predict::ConstLevel,
																										&parameterForMFU,
																										&shortSeq,
																										maxPredictAppForRam);
							// build power result
							PredictResult result = gspPredict.predictResult_forwardPredict_byMethod(
																									FAN, // Forward App Number
																									GSP_Predict::powerOfLevel,
																									parameter,
																									&shortSeq,
																									maxPredictAppForRam);
							MFUresult.rateBase();
							MFUresult *= EXPERIMENT_base;
							result += MFUresult;
							result.sort();
							allResult[experiment.seqPoint] = result;
						}
						experiment.updatePrediction(&allResult[experiment.seqPoint]);
					} while (experiment.next());
	#endif
#endif
					
					// output predict result
					printf("%2d|", ram);
					experiment.memoryPrint();
					cout<<endl;
				}
			}
#endif
			
			// xxx) GSP normal & power compare // A little bird told me that has some bug!!!!!
#ifdef EXPERIMENT_GSP_normal_and_power_compare
			{ cout << " ----- GSP - normal & power predict:" <<endl;
				//{ initial
				Experiment normalExp(maxBackApp, maxPredictApp, &testDMEventPoint);
				Experiment powerExp (maxBackApp, maxPredictApp, &testDMEventPoint);
				// parameter initial | method: weight = base + powerNum ^ level(0~n)
				double base = 1;
				double powerNum = 15;
				double parameter[2];
				parameter[0] = base;
				parameter[1] = powerNum;//}
				
				//{ predict
				do {
					Sequence norShortSeq = normalExp.buildShortSeq();
					Sequence powShortSeq = powerExp.buildShortSeq();
					PredictResult norResult = gspPredict.predictResult_normal(&norShortSeq, maxPredictApp);
					PredictResult powResult = gspPredict.predictResult_byMethod(GSP_Predict::powerOfLevel, parameter, &powShortSeq, maxPredictApp);
					int norPredict = normalExp.updatePrediction(&norResult);
					int powPredict = powerExp.updatePrediction(&powResult);
					
				} while (normalExp.next() && powerExp.next());//}
				
				//{ output
				normalExp.print();
				powerExp.print();
				cout<<endl;//}
			}
#endif
		}
		
		/** 建立 sequence 方便搜尋和預測
		 *  return : 順序是由舊->新
		 *  (bug) 不會比較 seqEnd 有沒有比 maxBackApp 還長，所以可能會往後挖過頭
		 *  (bug) 也不會比較 seqEnd 有沒有超過 DMEventPoint
		 */
		Sequence buildShortSequence(int seqEnd, int maxBackApp, const vector<elemType> *DMEventPoint) {
			Sequence shortSeq;
			// 整理 sequence
			for (int j=seqEnd-maxBackApp; j<=seqEnd; j++) {
				Itemset tmpItem;
				tmpItem.item.push_back(DMEventPoint->at(j));
				shortSeq.itemset.push_back(tmpItem);
			}
			
			return shortSeq;
		}//}
		
};

static void mainOfExperiment(vector<Event> *eventVec, vector<string> *allAppNameVec) {
#ifndef MAIN_experiment_LOOP // not define
	DataMining dataMining;
	dataMining.build(eventVec, allAppNameVec);
	dataMining.experiment();
#else
	while (true) {
		DataMining dataMining;
		dataMining.build(eventVec, allAppNameVec);
		dataMining.experiment();
		
	#ifdef EXPERIMENT_Interval_time
		TRAINING_INTERVAL_increase_time++;
	#endif
	#ifdef EXPERIMENT_increase_FAN
		if (++FAN > EXPERIMENT_increase_FAN_end) {
			break;
		}
	#endif
	}
#endif
}

#endif /* DATA_MINING_EXPERIMENT_HPP */