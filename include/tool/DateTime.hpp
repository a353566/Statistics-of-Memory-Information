#ifndef DATETIME_HPP
#define DATETIME_HPP

#include <string>

//#include "StringToNumber.hpp"

class DateTime {
  // 單純紀錄日期時間
  public :
    int year;
    int month;
    int day;
    
    int hour;
    int minute;
    int second;
    
    void initial();
    void initial_hour(int hours);
		void initial_Day(int days);
		
    bool setAllDateTime(std::string fileDate);
    
    bool setDate(std::string fileDate);
    
    void output();
    
		// 取得保存用的字串
		std::string getSaveString();
		
    // 此天星期幾
    int WeekDay();
    
    // 此月有幾天
    int MonthDay();
    // 比較的是時間的新舊，新的會大於舊的(1:2:3 會大於 1:2:4)
    bool operator > (const DateTime &otherDate)const;
    bool operator < (const DateTime &otherDate)const;
		
    // 一定要大減小(近剪遠)！！！
    DateTime operator - (const DateTime &otherDate)const;
};

#endif /* DATETIME_HPP */