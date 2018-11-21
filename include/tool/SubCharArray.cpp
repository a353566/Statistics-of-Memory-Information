#include <string>

/** 剪取中間某一段資料
 *  charArray : 整段資料 (ex: name|15989|null|658|0|9|)
 *  size : charArray 長度 (ex: 25)
 *  subUnit : 要剪的字元 (ex: '|')
 *  whichData : 要剪的區段 (ex: 1 => return 15989)
 *  return : 0    1     2    3   4 5  (ps: 以上面為例)
 *           name|15989|null|658|0|9|
 */
std::string subCharArray(const char* charArray, int size, char subUnit, int whichData) {
	if (whichData<0)  // 防呆
		return std::string("");
	
	int head = 0;
	int end = 0;
	for (int i=0; i<size; i++) {
		if (charArray[i] == subUnit) { // 尋找相同字元
			whichData--;
			if (whichData == 0) { // 找到頭了
				head = i+1;
			} else if (whichData == -1) { // 確定找完了
				end = i;
				if (head < end)  // 怕裡面沒有資料
					return std::string(charArray + head, end - head);
				else
					return std::string("");
			}
		}
	}
	
	return std::string("");
}