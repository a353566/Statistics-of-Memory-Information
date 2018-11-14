#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>

//#define MAIN_debug_File_name

// ----- 時間間隔實驗 (hour)
//#define MAIN_Interval_time

// ----- 取消所有多餘細節的顯示
#define CANCEL_MORE_DETAIL_OUTPUT

#include "include/CollectionFile.hpp"
#include "include/MergeFile.hpp"
#include "include/DataMiningExperiment.hpp"
using namespace std;

int getdir(string dir, vector<string> &files);   // 取得資料夾中檔案的方法

int main(int argc, char** argv) {
	//{ ----- files initial
  vector<string> files;
  // 取出檔案，並判斷有沒有問題
  string folder = (argc == 2) ? string(argv[1]):string(".");	// 資料夾路徑(絕對位址or相對位址) 拿參數給予的目的地
  if (getdir(folder, files) == -1) {
    cout << "Error opening" << folder << endl;
    return -1;
  }//}

  // 整理所有檔案內容
  vector<CollectionFile> collectFileVec; // 所有檔案的 vector
  for(int i=0; i<files.size(); i++) {
    CollectionFile oneFile(folder, files[i]);
    //{ ----- check file name
		// 檢查檔案看是不是需要的檔案，像 2017-12-24_03.20.27 就是正確的檔名
	  if (!oneFile.setAllDateTime(files[i])) {
      continue;
    }//}
		
    // 確定沒問題後打開檔案
    if (oneFile.openFileAndRead()) {
      // 檔案資料正確後，放入檔案的 linked list 中
      // 順便排序
      int ins = 0;
      for (ins=0; ins<collectFileVec.size(); ins++) {
        if (collectFileVec[ins] > oneFile)
          break;
      }
      collectFileVec.insert(collectFileVec.begin()+ins, oneFile);
    } else {
      continue;
    }
  }
  // 檔名依序輸出
#ifdef MAIN_debug_File_name
  for (int i=0; i<collectFileVec.size(); i++)
    collectFileVec[i].date.output();
#endif
	
  // 輸出數量
#ifndef CANCEL_MORE_DETAIL_OUTPUT
  cout << "收集了多少" << collectFileVec.size() << "檔案" <<endl;
#endif  
	
  // 將檔案給 mergeFile 整理成 dataMining 可以讀的資料
  MergeFile mergeFile;      // 整理所有資料
  mergeFile.merge(&collectFileVec);
  mergeFile.buildEventVec();
	
  // 將 mergeFile 中的 allEventVec appNameVec 給 dataMining 去整理
	// 並且開始時實驗
#ifdef MAIN_Interval_time
	while (true) {
		DataMining dataMining;
		dataMining.build(&mergeFile.allEventVec, &mergeFile.allAppNameVec);
		dataMining.experiment();
	}
#else
	DataMining dataMining;
	dataMining.build(&mergeFile.allEventVec, &mergeFile.allAppNameVec);
	dataMining.experiment();
#endif

#ifndef CANCEL_MORE_DETAIL_OUTPUT
  cout << " ┌------┐\n" <<
	        " | over |\n" <<
					" └------┘"   <<endl;
#endif
  return 0;
}

int getdir(string folder, vector<string> &files) {
  // 創立資料夾指標
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(folder.c_str())) == NULL) {
    return -1;
  }
  // 如果dirent指標非空
  while((dirp = readdir(dp)) != NULL) {
    // 將資料夾和檔案名放入vector
    files.push_back(string(dirp->d_name));
  }
  // 關閉資料夾指標
  closedir(dp);
  return 0;
}
