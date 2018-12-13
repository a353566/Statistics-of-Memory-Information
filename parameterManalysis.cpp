#include <dirent.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "include/tool/StringToNumber.hpp"
#include "include/tool/SubCharArray.hpp"

using namespace std;
int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

int main(int argc, char** argv) {
	//{ ----- parameter initial
	string inputFolder("./M/");
	vector<string> fileVec;
	for (int i=1; i<argc; i++) {
		char *temp;
		// input
		temp = strstr(argv[i], "input=");
		if (temp != NULL) {
			inputFolder = string(temp+6);
			cout << "input folder is \"" << inputFolder << "\"" <<endl;
			continue;
		}
		// file
		/*temp = strstr(argv[i], "file=");
		if (temp != NULL) {
			file = string(temp+5);
			cout << "file is \"" << file << "\"\n" <<endl;
			continue;
		}*/
	}//}
	
	/*if (argc >= 5) {
		StringToNumber(string(argv[3]), &START_SPACE_TIME);
		StringToNumber(string(argv[4]), &TRAINING_INTERVAL_DAY);
		StringToNumber(string(argv[5]), &TEST_INTERVAL_DAY);
		cout << START_SPACE_TIME << "\n" << TRAINING_INTERVAL_DAY << "\n" << TEST_INTERVAL_DAY <<endl;
	} else {
		cout << "(error) the parameters is not enough" <<endl;
		return 0;
	}*/
	
	//{ ----- files initial
  // 取出檔案，並判斷有沒有問題
  if (getdir(inputFolder, fileVec) == -1) {
    cout << "Error opening" << inputFolder << endl;
    return -1;
  }//}
	
	bool first = true;
	double left[300][2];
	double table[300][15];
	int totalFile=0;
	for (int i=0; i<300; i++) {
		for (int j=0; j<2; j++) {
			left[i][j]=0;
		}
	}
	for (int i=0; i<300; i++) {
		for (int j=0; j<15; j++) {
			table[i][j]=0;
		}
	}
	
	for (auto onefile =fileVec.begin(); onefile!=fileVec.end(); onefile++) {
		//{ 開檔部分
		FILE *file;
		string path = inputFolder + *onefile;
		file = fopen(path.c_str(),"r"); // "r" 讀檔而已
		if(file == NULL) {
			cout << "open \"" << *onefile << "\" File fail" << endl ;
			return false;
		}//}
		
		//{ initial
		int line=-1;
		int getLineSize = 4096;
		char getLine[getLineSize]; //}
		
		// 跳過 9 行
		for (int i=0; i<9; i++) {
			if (fgets(getLine, getLineSize, file) == NULL) {
				fclose(file);
				break;
			}
		}
		
		cout << *onefile << " ";
		for (int i=0; i<300; i++) {
			if (fgets(getLine, getLineSize, file) != NULL) {  // 讀一行
				line++;
				string temp;
				// left
				temp = string(getLine,4);
				left[line][0] += atof(temp.c_str());
				temp = string(getLine+7,7);
				left[line][1] += atof(temp.c_str());
				
				// table
				for (int j=0; j<15; j++) {
					temp = string(getLine+17+10*j,7);
					table[line][j] += atof(temp.c_str());
				}
			} else {
				cout << "(error)" << *onefile <<endl;
				fclose(file);
				return false;
			}
		}
		totalFile++;
		fclose(file);
	}
	
	// output
	cout << "totalFile:" << totalFile <<endl;
	for (int i=0; i<300; i++) {
		cout << left[i][0]/totalFile << "|" << left[i][1]/fileVec.size();
		// table
		for (int j=0; j<15; j++) {
			cout << "|" << table[i][j]/totalFile;
		}
		cout<<endl;
	}
	
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
		if (string(dirp->d_name) == ".") {
			continue;
		} else if (string(dirp->d_name) == "..") {
			continue;
		}
    // 將資料夾和檔案名放入vector
    files.push_back(string(dirp->d_name));
  }
  // 關閉資料夾指標
  closedir(dp);
  return 0;
}
