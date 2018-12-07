#ifndef APP_INFO_HPP
#define APP_INFO_HPP

#include <vector>
#include <string.h>
#include <iostream>
#include "tool/SubCharArray.hpp"
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
		
		// 存檔 取得保存用的字串
		string getSaveString() {
			// name|pid|TotalPss|oom_score|ground|oom_adj|
			std::string result;
			result = std::to_string(namePoint);
			result += "_";
			result += std::to_string(pid);
			result += "_";
			result += std::to_string(totalPss);
			result += "_";
			result += std::to_string(oom_score);
			result += "_";
			result += std::to_string(ground);
			result += "_";
			result += std::to_string(oom_adj);
			result += "_";
			return result;
		}
		
		// 讀檔
		bool setData(string data) {
			const char* charArray = data.c_str();
			int size = data.size();
			string temp;
			int value;
			
			temp = subCharArray(charArray, size, '_', 0);
			if (StringToNumber(temp, &value)) { namePoint = value; } else { return false; }
			
			temp = subCharArray(charArray, size, '_', 1);
			if (StringToNumber(temp, &value)) { pid = value; } else { return false; }
			
			temp = subCharArray(charArray, size, '_', 2);
			if (StringToNumber(temp, &value)) { totalPss = value; } else { return false; }
			
			temp = subCharArray(charArray, size, '_', 3);
			if (StringToNumber(temp, &value)) { oom_score = value; } else { return false; }
			
			temp = subCharArray(charArray, size, '_', 4);
			if (StringToNumber(temp, &value)) { ground = value; } else { return false; }
			
			temp = subCharArray(charArray, size, '_', 5);
			if (StringToNumber(temp, &value)) { oom_adj = value; } else { return false; }
			
			return true;
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