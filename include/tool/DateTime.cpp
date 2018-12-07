#include <string>

#include "DateTime.hpp"
#include "StringToNumber.hpp"

// 單純紀錄日期時間
void DateTime::initial() {
	year = 0;
	month = 0;
	day = 0;
	
	hour = 0;
	minute = 0;
	second = 0;
}

void DateTime::initial_hour(int hours) {
	initial();
	hour = hours;
	
	while (hour >= 24) {
		hour -= 24;
		day++;
		if (day > MonthDay()) {
			day = day - MonthDay();
			month++;
			if (month==12) {
				year++;
				month = 0;
			}
		}
	}
}

void DateTime::initial_Day(int days) {
	initial();
	day = days;
	
	while (day > MonthDay()) {
		day = day - MonthDay();
		month++;
		if (month==12) {
			year++;
			month = 0;
		}
	}
}

bool DateTime::setAllDateTime(std::string fileDate) {
	// 2017-12-16_03.04.28
	if (19 <= fileDate.size()) {
		// ----- 日期 -----
		if (!setDate(fileDate)) {
			return false;
		}
		// ----- 時間 -----
		// 檢查 12~13 個是不是數字
		int temp;
		if (StringToNumber(fileDate.substr(11,2), &temp)) {
			hour = temp;       // hour
		} else {
			return false;
		}
		// 檢查 15~16 個是不是數字
		if (StringToNumber(fileDate.substr(14,2), &temp)) {
			minute = temp; // minute
		} else {
			return false;
		}
		// 檢查 18~19 個是不是數字
		if (StringToNumber(fileDate.substr(17,2), &temp)) {
			second = temp;       // second
		} else {
			return false;
		}
		return true;
	} else {
		// 都不符合 return false
		return false;
	}
}

bool DateTime::setDate(std::string fileDate) {
	// ----- 日期 -----
	// 檢查 1~4 個是不是數字 year
	int temp;
	if (StringToNumber(fileDate.substr(0,4), &temp)) {
		year = temp;
	} else {
		return false;
	}
	
	// 檢查 6~7 個是不是數字 month
	if (StringToNumber(fileDate.substr(5,2), &temp)) {
		month = temp;
	} else {
		return false;
	}
	
	// 檢查 9~10 個是不是數字 day
	if (StringToNumber(fileDate.substr(8,2), &temp)) {
		day = temp;
	} else {
		return false;
	}
	return true;
}

void DateTime::output() {
	if (year==0 && month==0 && day==0) {
		printf("%2d-%2d-%2d", hour, minute, second);
	} else {
		printf("%4d/%2d/%2d %2d-%2d-%2d", year, month, day, hour, minute, second);
	}
}

void DateTime::output_colon_ver() {
	if (year==0 && month==0 && day==0) {
		printf("%2d:%2d:%2d", hour, minute, second);
	} else {
		printf("%4d/%2d/%2d %2d:%2d:%2d", year, month, day, hour, minute, second);
	}
}

// 取得保存用的字串
std::string DateTime::getSaveString() {
	// 2017-12-16_03.04.28
	std::string result;
	result = std::to_string(year);
	result += "-";
	if (month<=9) {result += "0";}
	result += std::to_string(month);
	result += "-";
	if (day<=9) {result += "0";}
	result += std::to_string(day);
	result += "_";
	if (hour<=9) {result += "0";}
	result += std::to_string(hour);
	result += ".";
	if (minute<=9) {result += "0";}
	result += std::to_string(minute);
	result += ".";
	if (second<=9) {result += "0";}
	result += std::to_string(second);
	return result;
}

// 比較的是時間的新舊，新的會大於舊的(1:2:3 會大於 1:2:4)
bool DateTime::operator > (const DateTime &otherDate)const {
	if (year == otherDate.year) {
		if (month == otherDate.month) {
			if (day == otherDate.day) {
				if (hour == otherDate.hour) {
					if (minute == otherDate.minute) {
						return second > otherDate.second;
					} else {
						return minute > otherDate.minute;
					}
				} else {
					return hour > otherDate.hour;
				}
			} else {
				return day > otherDate.day;
			}
		} else {
			return month > otherDate.month;
		}
	} else {
		return year > otherDate.year;  
	}
}
bool DateTime::operator < (const DateTime &otherDate)const {
	if (year == otherDate.year) {
		if (month == otherDate.month) {
			if (day == otherDate.day) {
				if (hour == otherDate.hour) {
					if (minute == otherDate.minute) {
						return second < otherDate.second;
					} else {
						return minute < otherDate.minute;
					}
				} else {
					return hour < otherDate.hour;
				}
			} else {
				return day < otherDate.day;
			}
		} else {
			return month < otherDate.month;
		}
	} else {
		return year < otherDate.year;  
	}
}

// 一定要大減小(近剪遠)！！！
DateTime DateTime::operator - (const DateTime &otherDate)const {
	bool isSubCarry = false;
	DateTime temp;
	temp.initial();
	// second
	temp.second = second - otherDate.second;
	if (temp.second<0) {
		isSubCarry = true;
		temp.second += 60;
	}
	// minute
	temp.minute = minute - otherDate.minute;
	if (isSubCarry) {
		temp.minute--;
		isSubCarry = false;
	}
	if (temp.minute<0) {
		isSubCarry = true;
		temp.minute += 60;
	}
	// hour
	temp.hour = hour - otherDate.hour;
	if (isSubCarry) {
		temp.hour--;
		isSubCarry = false;
	}
	if (temp.hour<0) {
		isSubCarry = true;
		temp.hour += 24;
	}
	// day
	temp.day = day - otherDate.day;
	if (isSubCarry) {
		temp.day--;
		isSubCarry = false;
	}
	if (temp.day<0) {
		isSubCarry = true;
		temp.year = year;		// 2 月會有所不同
		temp.month = month-1;
		if (temp.month == 0) {
			temp.month = 12;
			temp.year--;
		}
		temp.day += temp.MonthDay();
	}
	// month
	temp.month = month - otherDate.month;
	if (isSubCarry) {
		temp.month--;
		isSubCarry = false;
	}
	if (temp.month<0) {
		isSubCarry = true;
		temp.month += 12;
	}
	// year
	temp.year = year - otherDate.year;
	if (isSubCarry) {
		temp.year--;
	}
	
	// 檢察是不是有問題
	if (temp.year<0) {
		temp.year=0;
		temp.month=0;
		temp.day=0;
		temp.hour=0;
		temp.minute=0;
		temp.second=0;
		printf("read.cpp:DateTime::operator'-' error");
	}
	
	return temp;
}

// 此天星期幾
int DateTime::WeekDay() {
	// 利用基姆拉尔森计算日期公式
	// 0~6 -> 日一二三四五六
	return (day + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400 + 1)%7;
}

// 此月有幾天
int DateTime::MonthDay() {
	switch(month){
		case 0: case 1: case 3: case 5: case 7: case 8: case 10: case 12:
			return 31;
			break;
		case 4: case 6: case 9: case 11:
			return 30;
			break;
		case 2:
			if ((year%4==0&&year%100!=0)||year%400==0)
				return 28;
			else 
				return 29;
			break;
		default:
			printf("read.cpp:DateTime::MonthDay error");
			return -1;
			break;
	}
}
