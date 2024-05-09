#include"date.h"
#include<string>
#include"stdio.h"
#include "common/log/log.h"
bool check_date(int year, int month, int day)
{
  static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  LOG_WARN("check_date: year %d,month %d,day %d");
  bool leap = (year % 400 == 0 || (year % 100 && year % 4 == 0));
  if (year > 0 && (month > 0) && (month <= 12) && (day > 0) && (day <= ((month == 2 && leap) ? 1 : 0) + mon[month]))
    return true;
  else
    return false;
}

int string_to_date(const std::string &str,int32_t & date)
{
    int y,m,d;
    sscanf(str.c_str(), "%d-%d-%d", &y, &m, &d);//not check return value eq 3, lex guarantee
    bool b = check_date(y,m,d);
    if(!b) return -1;
    date = y*10000+m*100+d;
    return 0;
}
std::string date_to_string(int32_t date)
{
    std::string ans = "YYYY-MM-DD";
    std::string tmp = std::to_string(date);
    int tmp_index = 7;
    for(int i = 9 ; i >=0 ;i--)
    {
        if(i == 7|| i == 4)
        {
            ans[i] = '-';
        }
        else
        {
            ans[i] = tmp[tmp_index--];
        }
    }
    return ans;
}