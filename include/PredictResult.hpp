#ifndef PREDICT_RESULT_HPP
#define PREDICT_RESULT_HPP

#include <vector>
#include <iostream>

#include "Sequence.hpp"
using namespace std;

typedef int elemType;

class PredictResult {
	public :
	  const static int NO_APP = -1000;
	  const static int NO_WEIGHT = -1001;
		vector<pair<elemType, double> > resultPairs;	// <哪個 app ,count or total weight>
		
		// 檢查是否重複
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
				cout << "(" << oneApp->first << " ," << oneApp->second << ") ";
			}
			cout <<endl;
		}
};

#endif /* PREDICT_RESULT_HPP */