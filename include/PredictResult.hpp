#ifndef PREDICT_RESULT_HPP
#define PREDICT_RESULT_HPP

#include <vector>
#include <iostream>

using namespace std;

typedef int elemType;

class PredictResult {
	public :
	  const static int NO_APP = -1000;
	  const static int NO_WEIGHT = -1001;
		vector<pair<elemType, double> > resultPairs;	// <哪個 app ,count or total weight>
		
		/** 檢查是否重複
		 *  checkApp : 要檢查的 App
		 *  return : true , false
		 */
		bool checkRepeat(elemType checkApp) {
			for (vector<pair<elemType, double> >::iterator oneApp = resultPairs.begin();
					 oneApp != resultPairs.end(); oneApp++)
			{
				if (oneApp->first == checkApp) {
					return true;
				}
			}
			return false;
		}
		
		/** 回傳 第幾個才預測成功 (1~maxPredictApp or PREDICT_MISS)
		 *  reallyUseApp : 要比對的 APP (真正下一個用的 APP)
		 *  maxPredictApp : 最多預測到幾個 APP
		 *  return : 1 ~ maxPredictApp or
		 *           PREDICT_MISS
		 */
		const static int PREDICT_MISS = -1002;
		int predict(int reallyUseApp, int maxPredictApp) {
			for (int i=0; i<maxPredictApp; i++) {
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
		
		// output
		void output() {
			cout << "a result: ";
			for (vector<pair<elemType, double> >::iterator oneApp = resultPairs.begin();
					 oneApp != resultPairs.end(); oneApp++)
			{
				//cout << "(" << oneApp->first << ") ";
				cout << "(" << oneApp->first << " ," << oneApp->second << ") ";
			}
			cout <<endl;
		}
};

#endif /* PREDICT_RESULT_HPP */