#ifndef SUBCHAR_HPP
#define SUBCHAR_HPP

#include <string>

/** 剪取中間某一段資料
 *  charArray : 整段資料 (ex: name|15989|null|658|0|9|)
 *  size : charArray 長度 (ex: 25)
 *  subUnit : 要剪的字元 (ex: '|')
 *  whichData : 要剪的區段 (ex: 1 => return 15989)
 *  return : 0    1     2    3   4 5  (ps: 以上面為例)
 *           name|15989|null|658|0|9|
 */
std::string subCharArray(const char* charArray, int size, char subUnit, int whichData);

#endif /* SUBCHAR_HPP */