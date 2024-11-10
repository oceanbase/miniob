// 日期类型的数据结构
struct Date {
    int year;
    int month;
    int day;
};

// 检查日期的合法性
bool isDateValid(Date date) {
    // 检查年份范围
    if (date.year < 1970 || date.year > 2038) {
        return false;
    }
    // 检查月份范围
    if (date.month < 1 || date.month > 12) {
        return false;
    }
    // 检查日期范围，考虑闰年
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (date.year % 4 == 0 && (date.year % 100!= 0 || date.year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    return date.day >= 1 && date.day <= daysInMonth[date.month - 1];
}

// 将日期转换为整数表示（从 1970 年 1 月 1 日开始的天数）
int dateToInt(Date date) {
    int days = 0;
    for (int y = 1970; y < date.year; y++) {
        days += isLeapYear(y)? 366 : 365;
    }
    for (int m = 1; m < date.month; m++) {
        days += daysInMonth[m - 1];
        if (m == 2 && isLeapYear(date.year)) {
            days++;
        }
    }
    return days + date.day - 1;
}

// 将整数表示转换为日期
Date intToDate(int days) {
    int year = 1970;
    while (days >= (isLeapYear(year)? 366 : 365)) {
        days -= isLeapYear(year)? 366 : 365;
        year++;
    }
    int month = 1;
    while (days >= daysInMonth[month - 1]) {
        days -= daysInMonth[month - 1];
        if (month == 2 && isLeapYear(year)) {
            days--;
        }
        month++;
    }
    return {year, month, days + 1};
}
