#ifndef APP_INFO_HPP
#define APP_INFO_HPP

#include <vector>
#include <string.h>
#include <iostream>
using namespace std;

/** class App
 * 單一 app 的所有資訊 
 * 其中名字是用 point 的方式指向 appNameVec 其中一個真正的名字
 * name|pid|TotalPss|oom_score|ground|oom_adj
 * TotalPss|oom_score|ground|oom_adj
 */
class AppInfo {
	public :
		const static int NULL_DATA = -10000; // 下面參數 6 個沒東西的話是 NULL_DATA
		int namePoint;
		int pid;
		int totalPss;
		int oom_score; // oom_score 是 NULL_DATA 的話代表沒有資料
		int ground;    // ground 是 NULL_DATA 的話代表沒有資料
		int oom_adj;   // oom_adj 是 NULL_DATA 的話代表沒有資料
		
		AppInfo() {
			namePoint = NULL_DATA;
			pid = NULL_DATA;
			totalPss = NULL_DATA;
			oom_score = NULL_DATA;
			ground = NULL_DATA;
			oom_adj = NULL_DATA;
		}
		
		void output() {
			cout << "namePoint:" << namePoint
				<< "\tpid:" << pid
				<< "\ttotalPss:" << totalPss
				<< "\toom_score:" << oom_score
				<< "\tground:" << ground
				<< "\toom_adj:" << oom_adj <<endl;
		};
		void output(const vector<string> *appNameVec) {
			cout << "name:" << (*appNameVec)[namePoint] << '\n'
				<< "pid:" << pid
				<< "\ttotalPss:" << totalPss
				<< "\toom_score:" << oom_score
				<< "\tground:" << ground
				<< "\toom_adj:" << oom_adj <<endl;
		}
};

#endif /* APP_INFO_HPP */