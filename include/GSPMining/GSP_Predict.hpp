#ifndef GSP_PREDICT_HPP
#define GSP_PREDICT_HPP

#include <vector>
#include <iostream>
#include <complex>
#include <map>

#include "GSP.hpp"
#include "Sequence.hpp"
#include "../PredictResult.hpp"
using namespace std;

// ----- display part -----
//#define GSP_PREDICT_HPP_display_output_countAppMap // 輸出 countAppMap 排序結果
//#define GSP_PREDICT_HPP_display_similarPattern
//#define GSP_PREDICT_HPP_display_ProbabilityTable_of_each_prediction

// ----- parameter part -----
#define GSP_PREDICT_HPP_Probability_threshold 0.005

typedef int elemType;

class GSP_Predict {
	public :
		map<elemType, int> *appCountMap;	// 單個 element 出現次數
		multimap<int, elemType> countAppMap;	// 單個 element 出現次數，以數量來排序
		map<int, vector<Sequence> > *miningPatternsMap;	// 第一個是長度(沒有 1) 2, 3, 4, 5, ~ n.
		
		GSP_Predict(GSP *gsp) {
			appCountMap = &gsp->appCountMap;
			miningPatternsMap = &gsp->miningPatternsMap;
			// build countAppMap
			for (map<elemType, int>::iterator iter= appCountMap->begin();
					 iter!=appCountMap->end(); iter++)
			{ 
				countAppMap.insert(make_pair(iter->second, iter->first));
			}
			
			// 輸出 countAppMap 排序結果
#ifdef GSP_PREDICT_HPP_display_output_countAppMap
			for (map<elemType, int>::iterator iter= countAppMap.begin();
					 iter!=countAppMap.end(); iter++)
			{ 
				cout << iter->first << ":" << iter->second <<endl;
			}
#endif
		}
		
		//  ┌----------------┐
		//  |  basic method  |
		/** └----------------┘
		 *  在 stats 找尋相似的 pattern, 再回傳給 predict 做最後判斷
		 *  map->int : 往回看幾個
		 *  map->vector : 其中有多少個 app(in pair) 在紀錄裡面
		 *  map->vector->pair : <which app, count>
		 *  usePatt(use Pattern) : 目前使用的順序
		 *  return : similarPattern or NULL
		 */ //{
		map<int, vector<pair<elemType, int> > > findSimilarPattern(Sequence *usePatt) {
			map<int, vector<pair<elemType, int> > > similarPattern;
			for (int backApp = 1; true; backApp++) {
				// 先檢查是不是沒資料了
				if (miningPatternsMap->find(backApp+1) == miningPatternsMap->end()) {
					break;
				} else if (usePatt->itemset.size() < backApp) {
					break;
				}
				
				vector<Sequence> *miningData = &(*miningPatternsMap)[backApp+1];
				vector<pair<elemType, int> > similarData;
				for (vector<Sequence>::iterator onePatt = miningData->begin(); 
				     onePatt != miningData->end(); onePatt++) 
				{
					// 檢查使用順序是否相似
					if (isSimilar(backApp, usePatt, &(*onePatt))) {
						// 取出等等會用的 APP 和 統計次數
						similarData.push_back(make_pair(onePatt->itemset.at(backApp).item.at(0), onePatt->num));
					}
				}
				// 如果這裡完全沒有結果的話就可以停止了
				if (similarData.empty()) {
					break;
				} else {
					similarPattern[backApp] = similarData;
				}
			}
			
#ifdef GSP_PREDICT_HPP_display_similarPattern
			usePatt->Output();
			for (auto oneSPM = similarPattern.begin(); oneSPM!=similarPattern.end(); oneSPM++) {
				cout << " ----- Level : " << oneSPM->first <<endl;
				for (auto oneSP = oneSPM->second.begin(); oneSP != oneSPM->second.end(); oneSP++) {
					cout << oneSP->first << " : " << oneSP->second <<endl;
				}
			}
#endif
			
			return similarPattern;
		};
		
		/** 比較兩個 pattern 是否相似
		 *  backApp : 往前看幾個 APP
		 *  usePatt(use Pattern) : 目前使用的順序
		 *  comparePatt(compare Pattern) : 要被比較的 Pattern
		 *  return : 是否相似
		 *  (bug) 不會比較 usePatt 的長度是否為 backApp
		 *  (bug) 不會比較 comparePatt 的長度是否為 backApp+1
		 */
		bool isSimilar(int backApp, Sequence *usePatt, Sequence *comparePatt) {
			vector<Itemset>::reverse_iterator useItem = usePatt->itemset.rbegin();
			vector<Itemset>::reverse_iterator compareItem = comparePatt->itemset.rbegin();
			compareItem++;
			for (int i=0; i<backApp; i++) {
				if (useItem->item.at(0) != compareItem->item.at(0)) {
					return false;
				} else {
					useItem++;
					compareItem++;
				}
			}
			return true;
		}
		
		/** 取得最長，再來是最多的那一個 app
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  return : (count*100 + level, which app)
		 *           return the app which is longest and most one.
		 *           or NULL (pair(-1, -1))
		 */
		pair<int, elemType> getLongAndMostApp(map<int, vector<pair<elemType, int> > > *simlPattsMap) {
			//{ initial
			int maxLevel = simlPattsMap->size();
			int maxLevelCount = 0;
			map<int, vector<pair<elemType, int> > >::iterator simlPatts;
			if (maxLevel == 0) { // 沒結果
				return make_pair(-1, PredictResult::NO_APP);
			}
			vector<elemType> sameCountApp;//}
			
			//{ 1. find maxLevelCount
			simlPatts = simlPattsMap->find(maxLevel);
			for (int i=0; i<simlPatts->second.size(); i++) {
				if (maxLevelCount < simlPatts->second.at(i).second) {
					maxLevelCount = simlPatts->second.at(i).second;
				}
			}//}
			
			//{ 2. build sameCountApp : on Max Level
			for (int i=0; i<simlPatts->second.size(); i++) {
				if (maxLevelCount == simlPatts->second.at(i).second) {
					sameCountApp.push_back(simlPatts->second.at(i).first); // push "app" into sameCountApp
				}
			}//}
			
			//{ 3. 還是兩個以上的話，繼續往下找
			int level = maxLevel;
			while (sameCountApp.size() > 1 && --level > 0) {
				vector<elemType> lastSameCountApp;
				int count = 0;
				
				//{ 3-1. find max Count on last Level, and include sameCountApp
				simlPatts = simlPattsMap->find(level);
				for (int i=0; i<simlPatts->second.size(); i++) {
					bool inSameCountApp = false; // 確認有在 sameCountApp 中
					for (vector<elemType>::iterator iter = sameCountApp.begin(); iter != sameCountApp.end(); iter++) {
						if (*iter == simlPatts->second.at(i).first) {
							inSameCountApp = true;
							break;
						}
					}
					if (inSameCountApp && count < simlPatts->second.at(i).second) {
						count = simlPatts->second.at(i).second;
					}
				}//}
				
				//{ 3-2. build lastSameCountApp : on last Level
				for (int i=0; i<simlPatts->second.size(); i++) {
					bool inSameCountApp = false; // 確認有在 sameCountApp 中
					for (vector<elemType>::iterator iter = sameCountApp.begin(); iter != sameCountApp.end(); iter++) {
						if (*iter == simlPatts->second.at(i).first) {
							inSameCountApp = true;
							break;
						}
					}
					if (inSameCountApp && count == simlPatts->second.at(i).second) {
						lastSameCountApp.push_back(simlPatts->second.at(i).first); // push "app" into lastSameCountApp
					}
				}//}
				
				// 3-3. sameCountApp <- lastSameCountApp
				sameCountApp = lastSameCountApp;
			}//}
			
			//{ 4. 還是兩個以上的話直接在 MFU 中比 (map<elemType, int> *appCountMap)
			if (sameCountApp.size() > 1) {
				vector<elemType> lastSameCountApp;
				int count = 0;
				
				//{ 4-1. find max Count on appCountMap, and include sameCountApp
				for (map<elemType, int>::iterator appCount=appCountMap->begin();
				     appCount!=appCountMap->end(); appCount++)
				{
					bool inSameCountApp = false; // 確認有在 sameCountApp 中
					for (vector<elemType>::iterator iter = sameCountApp.begin(); iter != sameCountApp.end(); iter++) {
						if (*iter == appCount->first) {
							inSameCountApp = true;
							break;
						}
					}
					if (inSameCountApp && count < appCount->second) {
						count = appCount->second;
					}
				}//}
				
				//{ 4-2. build lastSameCountApp : on appCountMap
				for (map<elemType, int>::iterator appCount=appCountMap->begin();
				     appCount!=appCountMap->end(); appCount++)
				{
					bool inSameCountApp = false; // 確認有在 sameCountApp 中
					for (vector<elemType>::iterator iter = sameCountApp.begin(); iter != sameCountApp.end(); iter++) {
						if (*iter == appCount->first) {
							inSameCountApp = true;
							break;
						}
					}
					if (inSameCountApp && count == appCount->second) {
						lastSameCountApp.push_back(appCount->first); // push "app" into lastSameCountApp
					}
				}//}
				
				// 4-3. sameCountApp <- lastSameCountApp
				sameCountApp = lastSameCountApp;
			}//}
			
			return make_pair(maxLevelCount*100 + maxLevel, sameCountApp[0]);
		}
		
		/** 移除指定 APP
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  rmApp : 要被刪掉的 APP (count*100 + level, which app)
		 *  return : true : 正確刪掉
		 *           false : 沒有找到
		 */
		bool eraseApp(map<int, vector<pair<elemType, int> > > *simlPattsMap, pair<int, elemType> *rmApp) {
			// 先取得那一層
			int level = rmApp->first%100; // (ps: count*100 + level)
			vector<pair<elemType, int> > *levelVec = &(simlPattsMap->at(level));
			// 在那一層中找到指定 APP
			for (vector<pair<elemType, int> >::iterator iter = levelVec->begin();
			     iter!=levelVec->end(); iter++)
			{
				if (iter->first == rmApp->second) {
					levelVec->erase(iter);
					// 如果此 vector 沒東西的話，map 那邊要記得刪掉
					if (levelVec->size() == 0) {
						simlPattsMap->erase(level);
					}
					return true;
				}
			}
			return false;
		}

		// 檢查是否現在在用此 app
		bool checkNowUse(Sequence *usePatt, elemType app) {
			return usePatt->itemset.rbegin()->item[0] == app;
		}//}
		
		//  ┌------------------┐
		//  |  predict method  |
		/** └------------------┘
		 * 取得預測的 APPs (normal)
		 *  (normal) 取得最長且最多的那一個 app
		 *           不夠的話用 MFU 補上
		 *  usePatt(use Pattern) : 目前使用的 APP Pattern
		 *  maxPredictApp : 總共要預測幾個 APP
		 *  return : 預測的結果，不夠 maxPredictApp 數量的話依照 MFU 補足
		 */ //{
		PredictResult predictResult_normal(Sequence *usePatt, int maxPredictApp) {
			// simlPattsMap(similarPatternsMap) 取得相似的 Pattern
			map<int, vector<pair<elemType, int> > > simlPattsMap = findSimilarPattern(usePatt);
			
			// 取得最長且最多的那一個 app，並取 maxPredictApp 個
			PredictResult result;
			if (!simlPattsMap.empty()) {
				// while 直到有 maxPredictApp 個APP
				while (result.size() < maxPredictApp) {
					pair<int, elemType> LMApp = getLongAndMostApp(&simlPattsMap); // LMApp : Long Most App
					// 檢查有沒有找到，沒有的話 break
					if (LMApp.first >= 0) {
						// 檢查是否重複
						if (!result.checkRepeat(LMApp.second)) {
							result += make_pair(LMApp.second, LMApp.first);
						}
						// 在結果中移除，以方便下次搜尋
						eraseApp(&simlPattsMap, &LMApp);
					} else {
						break;
					}
				}
			}
			
			// 用 MFU (Most Frequently Used) 來補上剩下的
			// multimap<int, elemType> countAppMap;	// 單個 element 出現次數，以數量來排序
			multimap<int, elemType>::reverse_iterator MFUApp = countAppMap.rbegin();
			while (result.size() < maxPredictApp && MFUApp != countAppMap.rend()) {
				// 檢查是否重複 & 是否現在在用
				if (!result.checkRepeat(MFUApp->second) && !checkNowUse(usePatt, MFUApp->second)) {
					result += make_pair(MFUApp->second, 0);
				}
				MFUApp++;
			}
			
			return result;
		};
		
		/** 取得預測的 APPs 根據不同方法
		 *  1. get Weight for each level : 根據方法不同，取得不同的 weight
		 *  2. calculate Total Weight for each app
		 *  3. sort app with Total Weight : 依照 total weight 大小來排序，並寫入 PredictResult
		 *  4. fill it : 剩下則給 (NO_APP, NO_WEIGHT)
		 *  method : 計算的方法
		 *  parameter : 不同方法參數不一樣
		 *  usePatt(use Pattern) : 目前使用的 APP Pattern
		 *  maxPredictApp : 總共要預測幾個 APP
		 *  return : 預測的結果，不夠 maxPredictApp 數量的話用 (NO_APP, NO_WEIGHT) 補足 (ps:in PredictResult)
		 */
		const static int ConstLevel;
		const static int multiplyOfLevle;
		const static int powerOfLevel;
		PredictResult predictResult_byMethod(int method, double *parameter,
		                                     Sequence *usePatt, int maxPredictApp) {
			// simlPattsMap(similarPatternsMap) 取得相似的 Pattern
			map<int, vector<pair<elemType, int> > > simlPattsMap = findSimilarPattern(usePatt);
			map<int, double> weightMap;
			
			//{ ----- 1. get Weight for each level : 根據方法不同，取得不同的 weight
			if (method == ConstLevel) {
				weightMap = computeWeight_constLevel(appCountMap, &simlPattsMap, parameter);
			} else if (method == multiplyOfLevle) {
				weightMap = computeWeight_multiplyOfLevel(appCountMap, &simlPattsMap, parameter);
			} else if (method == powerOfLevel) {
				weightMap = computeWeight_powerOfLevel(appCountMap, &simlPattsMap, parameter);
			} else {
				printf("GSP_Predict::predictResult_byMethod() No match any methodID, method: %d", method);
			}//}
			
			//  ----- 2. calculate Total Weight for each app
			multimap<double ,elemType> weightAppVec = computeTotalWeight_byLevel(appCountMap, &simlPattsMap, &weightMap);
			
			//{ ----- 3. sort app with Total Weight : 依照 total weight 大小來排序，並寫入 PredictResult
			PredictResult result;
			if (!weightAppVec.empty()) {
				// while 直到有 maxPredictApp 個APP，或數量不夠
				multimap<double ,elemType>::reverse_iterator oneApp = weightAppVec.rbegin();
				while (result.size() < maxPredictApp && oneApp != weightAppVec.rend()) {
					// 檢查是否重複
#ifdef EXPERIMENT_RAM_part
					result.resultPairs.push_back(make_pair(oneApp->second, oneApp->first));
#else
					if (!result.checkRepeat(oneApp->second) && !checkNowUse(usePatt, oneApp->second)) {
						result += make_pair(oneApp->second, oneApp->first);
					}
#endif
					oneApp++;
				}
			}//}
			
			// ----- 4. fill it : 剩下則給 (NO_APP, NO_WEIGHT) (ps: (app, count))
			/*while (result.size() < maxPredictApp) {
				result.resultPairs.push_back(make_pair(PredictResult::NO_APP, PredictResult::NO_WEIGHT));
			}*/
			
			return result;
		};
		
		/** 計算 total weight
		 *  (by Level) : 針對 level 做運算
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  weightMap : 各層 level 的 weight 權重
		 *  return : multimap<TotalWeight ,app>
		 *           return a map which recode weight by level (level)
		 */
		multimap<double ,elemType> computeTotalWeight_byLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPattsMap, map<int, double> *weightMap) {
			map<elemType ,double> appWeightMap;
			
			// 先運算第 0 層 (ps: map<elemType, int> *appCountMap)
			if (!appCountMap->empty()) {
				// 在 weightMap 中搜尋是否有此 level(0)
				map<int, double>::iterator weight = weightMap->find(0);
				if (weight != weightMap->end()) {
					for (map<elemType, int>::iterator oneApp = appCountMap->begin();
							 oneApp != appCountMap->end(); oneApp++)
					{
						double TotalWeight = oneApp->second * weight->second; // (ps: count * weight)
						if (TotalWeight != 0) { // 不是 0 的話就加入到 appWeightMap
							appWeightMap[oneApp->first] += TotalWeight;
						}
					}
				}
			}
			
			// 再運算第 1 層以上 (ps: map<int, vector<pair<elemType, int> > > *simlPattsMap)
			if (!simlPattsMap->empty()) {
				for (map<int, vector<pair<elemType, int> > >::iterator simlPatts = simlPattsMap->begin();
						 simlPatts != simlPattsMap->end(); simlPatts++)
				{
					// 在 weightMap 中搜尋是否有此 level(simlPatts->first)
					map<int, double>::iterator weightIter = weightMap->find(simlPatts->first);
					if (weightIter != weightMap->end()) {
						double weight = weightIter->second;
						for (vector<pair<elemType, int> >::iterator oneApp = simlPatts->second.begin();
								 oneApp != simlPatts->second.end(); oneApp++)
						{
							double TotalWeight = oneApp->second * weight; // (ps: count * weight)
							if (TotalWeight != 0) { // 不是 0 的話就加入到 appWeightMap
								appWeightMap[oneApp->first] += TotalWeight;
							}
						}
					}
				}
			}
			
			// 最後放到 weightAppMap & sort
			multimap<double ,elemType> weightAppMap;
			for (map<elemType ,double>::iterator oneApp = appWeightMap.begin();
					 oneApp != appWeightMap.end(); oneApp++)
			{
				weightAppMap.insert(make_pair(oneApp->second, oneApp->first));
			}
			return weightAppMap;
		}//}
		
		//  ┌----------------┐
		//  |  level weight  |
		/** └----------------┘
		 * 計算 level weight (Const Level) : 指定 level 給定 1 其他給 0 (包括 MFU, level=0)
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : level : 指定的 level 層給 1
		 *  return : map<level, weight>
		 *           return a map which recode weight on const level (level)
		 */ //{
		map<int, double> computeWeight_constLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPattsMap, double *parameter) {
			map<int, double> weights;
			
			int level = (int) parameter[0];
			
			// 第 0 層
			weights[0] = (level == 0)? 1:0;
			
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPattsMap->begin(); iter!=simlPattsMap->end(); iter++) {
				weights[iter->first] = (iter->first != level)? 0:1;
			}
			return weights;
		}
		
		/** 計算 level weight (multiply Of Level) appCount 會隨著 level 提升 (包括 MFU, level=0)
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : base : 常數
		 *  parameter[1] : multiplyNum : 要乘上的數字
		 *  return : map<level, weight>
		 *           return a map which recode weight on each level (level)
		 */
		map<int, double> computeWeight_multiplyOfLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPattsMap, double *parameter) {
			map<int, double> weights;
			// weight = base + multiplyNum * level(0~n)
			double base = parameter[0];
			double multiplyNum = parameter[1];
			
			// 第 0 層
			weights[0] = base + multiplyNum * 0;
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPattsMap->begin(); iter!=simlPattsMap->end(); iter++) {
				weights[iter->first] = base + multiplyNum * iter->first;
			}
			return weights;
		}
		
		/** 計算 level weight (power Of Level) appCount 會隨著 level 提升 (包括 MFU, level=0)
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPattsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : base : 常數
		 *  parameter[1] : power : 要多次方的數字
		 *  return : map<level, weight>
		 *           return a map which recode weight on each level (level)
		 */
		map<int, double> computeWeight_powerOfLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPattsMap, double *parameter) {
			map<int, double> weights;
			// weight = base + powerNum ^ level(0~n)
			double base = parameter[0];
			double powerNum = parameter[1];
			
			// 第 0 層 : (ps: powerNum^0 = 1，但這裡不合理所以給 base 就好)
			weights[0] = base;
			
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPattsMap->begin(); iter!=simlPattsMap->end(); iter++) {
				//weights[iter->first] = base + powerNum ^ iter->first;
				weights[iter->first] = base + pow(powerNum, iter->first);
			}
			
			return weights;
		}//}
		
		//  ┌-------------------┐
		//  |  Forward Predict  |
		/** └-------------------┘
		 *  往前預測
		 */ //{
		/**  呼叫用的 function，會負責使用下面的 class 還得到 probability table
		 *  smaxStep : 設定要往前預測多少步
		 *  method : 計算的方法 (與 predictResult_byMethod 共用)
		 *  parameter : 不同方法參數不一樣
		 *  usePatt(use Pattern) : 目前使用的 APP Pattern
		 *  maxPredictApp : 總共要預測幾個 APP
		 *  return : Table 預測的結果
		 */
		PredictResult predictResult_forwardPredict_byMethod(int maxStep, int method, double *parameter, Sequence *usePatt, int maxPredictApp) {
			ForwardPredict forwardPredict(maxStep, method, parameter, maxPredictApp, this);
			return forwardPredict.getProbabilityTable(usePatt);
		}
		
		class ForwardPredict {
			public :
				const double P_threshold = GSP_PREDICT_HPP_Probability_threshold; // 多少發生機率以下不計算
				GSP_Predict *mGSPP;		// 傳自己進去，主要是要用它的資料
				int maxStep;					// 設定要往前預測多少步
				int method;						// 計算的方法 (與 predictResult_byMethod 共用)
				double *parameter;		// 不同方法參數不一樣
				int maxPredictApp;		// 總共要預測幾個 APP
				PredictResult *Table; // 1~maxStep
				
				ForwardPredict(int maxStep, int method, double *parameter, int maxPredictApp, GSP_Predict *mGSPP) {
					this->maxStep = maxStep;
					this->method = method;
					this->parameter = parameter;
					this->maxPredictApp = maxPredictApp;
					this->mGSPP = mGSPP;
					// Table initial
					Table = new PredictResult[maxStep+1];
				}
				
				/** 算和取得 Probability Table
				 *  usePatt(use Pattern) : 目前使用的 APP Pattern
				 *  return : 預測的結果，不夠 maxPredictApp 數量的話用 (NO_APP, NO_WEIGHT) 補足 (ps:in PredictResult)
				 */
				PredictResult getProbabilityTable(Sequence *usePatt) {
					predictNextTable(1, 1, usePatt);
					PredictResult finalResult;
					for (int step=1; step<=maxStep; step++) {
						finalResult += Table[step];
					}
					finalResult.sort();
					
#ifdef GSP_PREDICT_HPP_display_ProbabilityTable_of_each_prediction
					static int count = 1;
					printf("%4d  1:", count++);
					Table[1].output();
					for (int step=2; step<=maxStep; step++) {
						printf("     %2d:", step);
						Table[step].output();
					}
					printf(" All  :");
					finalResult.output();
#endif
					
					return finalResult;
				}
				
				/** 往後預測表格
				 *  impactRate : 因為前面往後傳導的影響率
				 *  step : 往前幾次
				 *  usePatt(use Pattern) : 目前使用的 APP Pattern
				 */
				void predictNextTable(double impactRate, int step, Sequence *usePatt) {
					// 先取得目前的
					PredictResult Result = mGSPP->predictResult_byMethod(method, parameter, usePatt, maxPredictApp);
					
					// 存到 Table 中
					Result.rateBase(); // 機率化
					Result *= impactRate;
					Table[step] += Result;
					
					if (step <= maxStep-1) { // 繼續往前
						for (int i=0; i<Result.size(); i++) {
							const pair<elemType, double> &oneApp = Result.getPairWithIndex(i);
							if (oneApp.first != PredictResult::NO_APP) {
								// 新的 result 重要性
								if (oneApp.second < P_threshold) {
									break;
								}
								
								Sequence forwardPatt = *usePatt;
								forwardPatt += oneApp.first;
								predictNextTable(oneApp.second, step+1, &forwardPatt);
							}
						}
					}
				}
		}; //}
		
};

const int GSP_Predict::ConstLevel = 101010;
const int GSP_Predict::multiplyOfLevle = 202020;
const int GSP_Predict::powerOfLevel = 303030;
#endif /* GSP_PREDICT_HPP */