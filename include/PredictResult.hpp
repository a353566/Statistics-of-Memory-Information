#ifndef PREDICT_RESULT_HPP
#define PREDICT_RESULT_HPP

#include <vector>
#include <iostream>

using namespace std;

typedef int elemType;

class PredictResult {
	private :
		vector<pair<elemType, double> > resultPairs;	// <哪個 app ,count or total weight>
	public :
		static const pair<elemType, double> emptyPair;
	  static const elemType NO_APP;
	  static const double NO_WEIGHT;
		
		PredictResult() {};
		
		/** 檢查是否重複
		 *  checkApp : 要檢查的 App
		 *  return : true , false
		 */
		bool checkRepeat(elemType checkApp) const {
			return findAppIndex(checkApp) != NO_APP;
		}
		
		/** 回傳 第幾個才預測成功 (from 1 to size()-1 or PREDICT_MISS)
		 *  reallyUseApp : 要比對的 APP (真正下一個用的 APP)
		 *  return : from 1 to size()-1 or
		 *           PREDICT_MISS
		 */
		const static int PREDICT_MISS;
		int getRank(const elemType &reallyUseApp) const {
			for (int i=0; i<size(); i++) {
				if (resultPairs.at(i).first == reallyUseApp) {
					return i+1;
				}
			}
			return PREDICT_MISS;
		}
		int getRank(const elemType &reallyUseApp, const elemType &filterUseApp) const {
			int reallyRank = getRank(reallyUseApp);
			int filterRank = getRank(filterUseApp);
			if (reallyRank > filterRank) {
				return reallyRank-1;
			} else if (reallyRank < filterRank) {
				return reallyRank;
			} else if (reallyRank == PREDICT_MISS) {
				return PREDICT_MISS;
			} else {
				printf("(error) PredictResult::getRank(e, e): e=e=%d", reallyRank);
				return reallyRank;
			}
		}
		
		// 回傳結果的長度
		int size() const {
			return resultPairs.size();
		}
		
		// 找此 app 在 vec 第幾個
		int findAppIndex(elemType findApp) const {
			if (findApp == NO_APP) {
				cout << "(error) PredictResult::findAppIndex() : find NO_APP" <<endl;
				return NO_APP;
			}
			for (int i=0; i<size(); i++) {
				if (resultPairs[i].first == findApp) {
					return i;
				}
			}
			return NO_APP;
		};
		
		// 從 index 回傳 pair
		const pair<elemType, double>& getPairWithIndex(int index) const {
			if (size() == 0 || index >= size()) {
				return emptyPair;
				//return NULL;
			} else {
				return resultPairs[index];
			}
		}
		
		bool removePairWithIndex(int index) {
			if (size() == 0 || index < size()) {
				return false;
			} else {
				resultPairs.erase(resultPairs.begin() + index);
				return true;
			}
		}
		
		// 加入一筆 pair，重複的話就更新
		void updatePair(const pair<elemType, double> &updatePair) {
			if (!checkRepeat(updatePair.first)) {
				bool isAdd = false;
				for (auto onePair =resultPairs.begin(); onePair !=resultPairs.end(); onePair++) {
					if (updatePair.second > onePair->second) {
						resultPairs.insert(onePair, updatePair);
						isAdd = true;
						return;
					}
				}
				if (!isAdd) {
					resultPairs.push_back(updatePair);
					return;
				}
			} else {
				// App is already in PredictResult, so update this, and sort.
				resultPairs[findAppIndex(updatePair.first)].second = updatePair.second;
				sort();
				return;
			}
		}
		
		// 取得 weight 的總和
		double totalWeight() const {
			double total = 0;
			for (auto oneApp = resultPairs.begin(); oneApp != resultPairs.end(); oneApp++) {
				if (oneApp->first != NO_APP) {
					total += oneApp->second;
				}
			}
			return total;
		}
		
		// 排序
		void sort() {
			for (int i=0; i+1<resultPairs.size(); i++) {
				if (resultPairs[i].second < resultPairs[i+1].second) {
					for (int j=i; j>=0; j--) {
						if (resultPairs[j].second < resultPairs[j+1].second) {
							elemType tempApp = resultPairs[j].first;
							resultPairs[j].first = resultPairs[j+1].first;
							resultPairs[j+1].first = tempApp;
							double tempWeight = resultPairs[j].second;
							resultPairs[j].second = resultPairs[j+1].second;
							resultPairs[j+1].second = tempWeight;
						} else {
							break;
						}
					}
				}
			}
		}
		
		// 將 count 改成機率形式
		void rateBase() {
			double base = 1/totalWeight();
			*this *= base;
		}
		
		void clear() {
			resultPairs.clear();
		}
		
		// output
		void output() const {
			printf("\"%5lf\":", totalWeight());
			for (auto oneApp = resultPairs.begin(); oneApp != resultPairs.end(); oneApp++)
			{
				printf("(%3d, %8lf) ", oneApp->first, oneApp->second);
			}
		}
		
		// resultPairs 中 weight 的數值 * rate
		void operator *= (const double &rate) {
			for (auto oneApp = resultPairs.begin(); oneApp != resultPairs.end(); oneApp++) {
				if (oneApp->first != NO_APP) {
					oneApp->second *= rate;
				}
			}
		}
		
		// 將兩個結果相加
		void operator += (const PredictResult &addResult) {
			for (auto addApp = addResult.resultPairs.begin(); addApp != addResult.resultPairs.end(); addApp++) {
				if (addApp->first != NO_APP) {
					int appIndex = findAppIndex(addApp->first);
					if (appIndex!=NO_APP) {
						resultPairs[appIndex].second += addApp->second;
					} else {
						resultPairs.push_back(*addApp);
					}
				}
			}
			sort();
		}
		
		// 加入一筆 pair，重複的話就更新
		void operator += (const pair<elemType, double> &addPair) {
			updatePair(addPair);
		}
};

const int PredictResult::NO_APP = -1000;
const pair<elemType, double> PredictResult::emptyPair = make_pair(NO_APP, NO_WEIGHT);
const double PredictResult::NO_WEIGHT = -100110;
const int PredictResult::PREDICT_MISS = 100002;
#endif /* PREDICT_RESULT_HPP */