#ifndef GET_DIRECTORY_HPP
#define GET_DIRECTORY_HPP

#include <dirent.h>
#include <stdio.h>
using namespace std;

// ----- hpp
int getdir(string dir, vector<string> &files);  // 取得資料夾中檔案的方法

// ----- cpp
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
#endif // GET_DIRECTORY_HPP