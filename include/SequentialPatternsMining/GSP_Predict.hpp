#ifndef GSP_PREDICT_HPP
#define GSP_PREDICT_HPP

#include <vector>
#include <iostream>
#include <complex>

#include "../Sequence.hpp"
#include "../PredictResult.hpp"
#include "GSP.hpp"

//#define GSP_PREDICT_HPP_debug_output_countAppMap // 輸出 countAppMap 排序結果

using namespace std;

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
#ifdef GSP_PREDICT_HPP_debug_output_countAppMap
			for (map<elemType, int>::iterator iter= countAppMap.begin();
					 iter!=countAppMap.end(); iter++)
			{ 
				cout << iter->first << ":" << iter->second <<endl;
			}
#endif
		}
		
		/** 在 stats 找尋相似的 pattern, 再回傳給 predict 做最後判斷
		 *  map->int : 往回看幾個
		 *  map->vector : 其中有多少個 app(in pair) 在紀錄裡面
		 *  map->vector->pair : <which app, count>
		 *  usePatn(use Pattern) : 目前使用的順序
		 *  return : similarPattern or NULL
		 */
		map<int, vector<pair<elemType, int> > > findSimilarPattern(Sequence *usePatn) {
			map<int, vector<pair<elemType, int> > > similarPattern;
			for (int backApp = 1; true; backApp++) {
				// 先檢查是不是沒資料了
				if (miningPatternsMap->find(backApp+1) == miningPatternsMap->end()) {
					break;
				} else if (usePatn->itemset.size() < backApp) {
					break;
				}
				
				vector<Sequence> *miningData = &(*miningPatternsMap)[backApp+1];
				vector<pair<elemType, int> > similarData;
				for (vector<Sequence>::iterator onePatn = miningData->begin(); 
				     onePatn != miningData->end(); onePatn++) 
				{
					// 檢查使用順序是否相似
					if (isSimilar(backApp, usePatn, &(*onePatn))) {
						// 取出等等會用的 APP 和 統計次數
						similarData.push_back(make_pair(onePatn->itemset.at(backApp).item.at(0), onePatn->num));
					}
				}
				// 如果這裡完全沒有結果的話就可以停止了
				if (similarData.empty()) {
					break;
				} else {
					similarPattern[backApp] = similarData;
				}
			}
			
			// output
			/*{
				for (map<int, vector<pair<elemType, int> > >::iterator mapIter = similarPattern.begin();
						 mapIter != similarPattern.end(); mapIter++)
				{
					cout << "-----" << mapIter->first << "-----" <<endl;
					for (vector<pair<elemType, int> >::iterator vecIter = mapIter->second.begin(); 
							 vecIter != mapIter->second.end(); vecIter++)
					{
						cout << vecIter->first << " : " << vecIter->second <<endl;
					}
				}
			}*/
			
			return similarPattern;
		};
		
		/** 比較兩個 pattern 是否相似
		 *  backApp : 往前看幾個 APP
		 *  usePatn(use Pattern) : 目前使用的順序
		 *  comparePatn(compare Pattern) : 要被比較的 Pattern
		 *  return : 是否相似
		 *  (bug) 不會比較 usePatn 的長度是否為 backApp
		 *  (bug) 不會比較 comparePatn 的長度是否為 backApp+1
		 */
		bool isSimilar(int backApp, Sequence *usePatn, Sequence *comparePatn) {
			vector<Itemset>::reverse_iterator useItem = usePatn->itemset.rbegin();
			vector<Itemset>::reverse_iterator compareItem = comparePatn->itemset.rbegin();
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
		
		/** 取得預測的 APPs
		 *  (normal) 取得最長且最多的那一個 app
		 *  usePatn(use Pattern) : 目前使用的 APP Pattern
		 *  maxPredictApp : 總共要預測幾個 APP
		 *  return : 預測的結果，不夠 maxPredictApp 數量的話依照 MFU 補足
		 */
		PredictResult predictResult_normal(Sequence *usePatn, int maxPredictApp) {
			// simlPatnsMap(similarPatternsMap) 取得相似的 Pattern
			map<int, vector<pair<elemType, int> > > simlPatnsMap = findSimilarPattern(usePatn);
			
			// 取得最長且最多的那一個 app，並取 maxPredictApp 個
			PredictResult result;
			if (!simlPatnsMap.empty()) {
				// while 直到有 maxPredictApp 個APP
				while (result.size() < maxPredictApp) {
					pair<int, elemType> LMApp = getLongAndMostApp(&simlPatnsMap); // LMApp : Long Most App
					// 檢查有沒有找到，沒有的話 break
					if (LMApp.first >= 0) {
						// 檢查是否重複
						if (!result.checkRepeat(LMApp.second)) {
							result.resultPairs.push_back(make_pair(LMApp.second, 0));
						}
						// 在結果中移除，以方便下次搜尋
						eraseApp(&simlPatnsMap, &LMApp);
					} else {
						break;
					}
				}
			}
			
			// 用 MFU (Most Frequently Used) 來補上剩下的
			// multimap<int, elemType> countAppMap;	// 單個 element 出現次數，以數量來排序
			multimap<int, elemType>::reverse_iterator MFUApp = countAppMap.rbegin();
			while (result.size() < maxPredictApp && MFUApp != countAppMap.rend()) {
				// 檢查是否重複
				if (!result.checkRepeat(MFUApp->second)) {
					result.resultPairs.push_back(make_pair(MFUApp->second, 0));
				}
				MFUApp++;
			}
			
			return result;
		};
		
		/** 取得預測的 APPs 根據不同方法
		 *  method : 計算的方法
		 *  parameter : 不同方法參數不一樣
		 *  usePatn(use Pattern) : 目前使用的 APP Pattern
		 *  maxPredictApp : 總共要預測幾個 APP
		 *  return : 預測的結果，不夠 maxPredictApp 數量的話用 (NO_APP, NO_WEIGHT) 補足 (ps:in PredictResult)
		 */
		const static int ConstLevel = 1;
		const static int multiplyOfLevle = 2;
		const static int powerOfLevel = 3;
		PredictResult predictResult_byMethod(int method, double *parameter, Sequence *usePatn, int maxPredictApp) {
			// simlPatnsMap(similarPatternsMap) 取得相似的 Pattern
			map<int, vector<pair<elemType, int> > > simlPatnsMap = findSimilarPattern(usePatn);
			map<int, double> weightMap;
			
			// 根據方法不同，取得不同的 weight
			if (method == ConstLevel) {
				weightMap = computeWeight_constLevel(appCountMap, &simlPatnsMap, parameter);
			} else if (method == multiplyOfLevle) {
				weightMap = computeWeight_multiplyOfLevel(appCountMap, &simlPatnsMap, parameter);
			} else if (method == powerOfLevel) {
				weightMap = computeWeight_powerOfLevel(appCountMap, &simlPatnsMap, parameter);
			} else {
				printf("GSP_Predict::predictResult_byMethod() No match any methodID, method: %d", method);
			}
			
			// 計算 Total Weight for each app
			multimap<double ,elemType> weightAppVec = computeTotalWeight_byLevel(appCountMap, &simlPatnsMap, &weightMap);
			
			// 依照 total weight 大小來排序，並寫入 PredictResult
			PredictResult result;
			if (!weightAppVec.empty()) {
				// while 直到有 maxPredictApp 個APP，或數量不夠
				multimap<double ,elemType>::reverse_iterator oneApp = weightAppVec.rbegin();
				while (result.size() < maxPredictApp && oneApp != weightAppVec.rend()) {
					// 檢查是否重複
					if (!result.checkRepeat(oneApp->second)) {
						result.resultPairs.push_back(make_pair(oneApp->second, oneApp->first));
					}
					oneApp++;
				}
			}
			
			// 剩下則給 (NO_APP, NO_WEIGHT) (ps: (app, count))
			while (result.size() < maxPredictApp) {
				result.resultPairs.push_back(make_pair(PredictResult::NO_APP, PredictResult::NO_WEIGHT));
			}
			return result;
		};
		
		/** 計算 level weight (Const Level) : 指定 level 給定 1 其他給 0
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : level : 指定的 level 層給 1
		 *  return : map<level, weight>
		 *           return a map which recode weight on const level (level)
		 */
		map<int, double> computeWeight_constLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPatnsMap, double *parameter) {
			map<int, double> weights;
			
			int level = (int) parameter[0];
			
			// 第 0 層
			weights[0] = (level == 0)? 1:0;
			
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPatnsMap->begin(); iter!=simlPatnsMap->end(); iter++) {
				weights[iter->first] = (iter->first != level)? 0:1;
			}
			return weights;
		}
		
		/** 計算 level weight (multiply Of Level) appCount 會隨著 level 提升
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : base : 常數
		 *  parameter[1] : multiplyNum : 要乘上的數字
		 *  return : map<level, weight>
		 *           return a map which recode weight on each level (level)
		 */
		map<int, double> computeWeight_multiplyOfLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPatnsMap, double *parameter) {
			map<int, double> weights;
			// weight = base + multiplyNum * level(0~n)
			double base = parameter[0];
			double multiplyNum = parameter[1];
			
			// 第 0 層
			weights[0] = base + multiplyNum * 0;
			
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPatnsMap->begin(); iter!=simlPatnsMap->end(); iter++) {
				weights[iter->first] = base + multiplyNum * iter->first;
			}
			
			return weights;
		}
		
		
		/** 計算 level weight (power Of Level) appCount 會隨著 level 提升
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  parameter[0] : base : 常數
		 *  parameter[1] : power : 要多次方的數字
		 *  return : map<level, weight>
		 *           return a map which recode weight on each level (level)
		 */
		map<int, double> computeWeight_powerOfLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPatnsMap, double *parameter) {
			map<int, double> weights;
			// weight = base + powerNum ^ level(0~n)
			double base = parameter[0];
			double powerNum = parameter[1];
			
			// 第 0 層 : (ps: powerNum^0 = 1，但這裡不合理所以給 base 就好)
			weights[0] = base;
			
			// 第 1 層以上
			map<int, vector<pair<elemType, int> > >::iterator iter;
			for (iter=simlPatnsMap->begin(); iter!=simlPatnsMap->end(); iter++) {
				//weights[iter->first] = base + powerNum ^ iter->first;
				weights[iter->first] = base + pow(powerNum, iter->first);
			}
			
			return weights;
		}
		
		/** 計算 total weight
		 *  (by Level) : 針對 level 做運算
		 *  appCountMap : 各個 APP 的統計數量
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  weightMap : 各層 level 的 weight 權重
		 *  return : multimap<TotalWeight ,app>
		 *           return a map which recode weight by level (level)
		 */
		multimap<double ,elemType> computeTotalWeight_byLevel(map<elemType, int> *appCountMap, map<int, vector<pair<elemType, int> > > *simlPatnsMap, map<int, double> *weightMap) {
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
			
			// 再運算第 1 層以上 (ps: map<int, vector<pair<elemType, int> > > *simlPatnsMap)
			if (!simlPatnsMap->empty()) {
				for (map<int, vector<pair<elemType, int> > >::iterator simlPatns = simlPatnsMap->begin();
						 simlPatns != simlPatnsMap->end(); simlPatns++)
				{
					// 在 weightMap 中搜尋是否有此 level(simlPatns->first)
					map<int, double>::iterator weight = weightMap->find(simlPatns->first);
					if (weight != weightMap->end()) {
						for (vector<pair<elemType, int> >::iterator oneApp = simlPatns->second.begin();
								 oneApp != simlPatns->second.end(); oneApp++)
						{
							double TotalWeight = oneApp->second * weight->second; // (ps: count * weight)
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
		}
		
		/** 取得最長，再來是最多的那一個 app
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  return : (level, which app)
		 *           return the app which is longest and most one.
		 *           or NULL (pair(-1, -1))
		 */
		pair<int, elemType> getLongAndMostApp(map<int, vector<pair<elemType, int> > > *simlPatnsMap) {
			int level = -1;
			int app = PredictResult::NO_APP;
			int count = 0;
			map<int, vector<pair<elemType, int> > >::reverse_iterator simlPatns = simlPatnsMap->rbegin();
			if (simlPatns != simlPatnsMap->rend()) {
				level = simlPatnsMap->size();
				app = simlPatns->second.at(0).second;
				for (int i=0; i<simlPatns->second.size(); i++) {
					if (count < simlPatns->second.at(i).second) {
						app = simlPatns->second.at(i).first;
						count = simlPatns->second.at(i).second;
					}
				}
			}
			return make_pair(level, app);
		}
		
		/** 取得此 level 最多的那一個 app
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  level : 指定的 level
		 *  return : (level, which app)
		 *           return the app which is most one in the level.
		 *           or NULL (pair(-1, PredictResult::NO_APP))
		 */
		pair<int, elemType> getMostApp(map<int, vector<pair<elemType, int> > > *simlPatnsMap, int level) {
			map<int, vector<pair<elemType, int> > >::iterator simlPatns = simlPatnsMap->find(level);
			if (simlPatns != simlPatnsMap->end()) {
				int app = simlPatns->second.at(0).first;
				int count = simlPatns->second.at(0).second;
				for (int i=0; i<simlPatns->second.size(); i++) {
					if (count < simlPatns->second.at(i).second) {
						app = simlPatns->second.at(i).first;
						count = simlPatns->second.at(i).second;
					}
				}
				return make_pair(level, app);
			} else {
				return make_pair(-1, PredictResult::NO_APP);
			}
		}
		
		/** 移除指定 APP
		 *  simlPatnsMap : (similarPatternsMap) 取得相似的 Pattern
		 *  rmApp : 要被刪掉的 APP
		 *  return : true : 正確刪掉
		 *           false : 沒有找到
		 */
		bool eraseApp(map<int, vector<pair<elemType, int> > > *simlPatnsMap, pair<int, elemType> *rmApp) {
			// 先取得那一層
			vector<pair<elemType, int> > *levelVec = &(simlPatnsMap->at(rmApp->first));
			// 在那一層中找到指定 APP
			for (vector<pair<elemType, int> >::iterator iter = levelVec->begin();
			     iter!=levelVec->end(); iter++)
			{
				if (iter->first == rmApp->second) {
					levelVec->erase(iter);
					// 如果此 vector 沒東西的話，map 那邊要記得刪掉
					if (levelVec->size() == 0) {
						simlPatnsMap->erase(rmApp->first);
					}
					return true;
				}
			}
			return false;
		}
};

#endif /* GSP_PREDICT_HPP */