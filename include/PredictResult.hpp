#ifndef PREDICT_RESULT_HPP
#define PREDICT_RESULT_HPP

#include <vector>
#include <iostream>

using namespace std;

typedef int elemType;

class PredictResult {
	public :
	  static const int NO_APP;
	  static const int NO_WEIGHT;
		vector<pair<elemType, double> > resultPairs;	// <哪個 app ,count or total weight>
		
		/** 檢查是否重複
		 *  checkApp : 要檢查的 App
		 *  return : true , false
		 */
		bool checkRepeat(elemType checkApp) {
			return findAppIndex(checkApp) != NO_APP;
		}
		
		/** 回傳 第幾個才預測成功 (1~maxPredictApp or PREDICT_MISS)
		 *  reallyUseApp : 要比對的 APP (真正下一個用的 APP)
		 *  maxPredictApp : 最多預測到幾個 APP
		 *  return : 1 ~ maxPredictApp or
		 *           PREDICT_MISS
		 */
		const static int PREDICT_MISS;
		int predict(elemType reallyUseApp) {
			return predict(reallyUseApp, size());
		}
		int predict(elemType reallyUseApp, int maxPredictApp) {
			for (int i=0; i<maxPredictApp && i<size(); i++) {
				if (resultPairs.at(i).first == reallyUseApp) {
					return i+1;
				}
			}
			return PREDICT_MISS;
		}
		
		// 回傳結果的長度
		int size() {
			return resultPairs.size();
		}
		
		// 找此 app 在 vec 第幾個
		int findAppIndex(elemType findApp) {
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
		}
		
		// 取得 weight 的總和
		double totalWeight() {
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
		
		// output
		void output() {
			printf("\"%5lf\":", totalWeight());
			for (vector<pair<elemType, double> >::iterator oneApp = resultPairs.begin();
					 oneApp != resultPairs.end(); oneApp++)
			{
				printf("(%3d, %8lf) ", oneApp->first, oneApp->second);
			}
			printf("\n");
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
		}
};

const int PredictResult::NO_APP = -1000;
const int PredictResult::NO_WEIGHT = -1001;
const int PredictResult::PREDICT_MISS = 100002;
#endif /* PREDICT_RESULT_HPP */