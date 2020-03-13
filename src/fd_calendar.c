#include "fd_calendar.h"

/* Nonzero if "y" is a leap year, else zero. */
int leap(int32_t y) {
    return (((y % 4) == 0) && ((y % 100) != 0)) || ((y % 400) == 0);
}

/* Number of leap years from 1970 to "y" (not including "y" itself). */
int nleap(int32_t y) {
    return (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400;
}

/* Additional leapday in February of leap years. */
int leapday(int m, int32_t y) {
    return (m == 1) && leap(y);
}

/* Accumulated number of days from 01-Jan up to start of current month. */
int32_t ydays(int m) {
    switch (m) {
        case 0: return 0;
        case 1: return 31;
        case 2: return 59;
        case 3: return 90;
        case 4: return 120;
        case 5: return 151;
        case 6: return 181;
        case 7: return 212;
        case 8: return 243;
        case 9: return 273;
        case 10: return 304;
        case 11: return 334;
        case 12: return 365;
    }
    return 0;
}

int32_t fd_calendar_to_time(fd_calendar_t calendar) {
    int32_t years;
    int32_t days;
    int32_t hour;
    int32_t min;
    int32_t sec;
    int32_t time;

    years = calendar.year + 1900;
    days = calendar.mday - 1; // 1..31 -> 0..30
    days += ydays(calendar.month);
    if ((calendar.month > 1) && leap(years)) {
        ++days;
    }
    days = days + 365 * (years - 1970) + nleap(years);
    time = 86400 * days;
    hour = calendar.hour;
    time += 3600 * hour;
    min = calendar.min;
    time += 60 * min;
    sec = calendar.sec;
    time +=    sec;
    return time;
}

int32_t yearsize(int32_t year) {
    int32_t days;

    if (leap(year)) {
        days = 366;
    } else {
        days = 365;
    }
    return days;
}

static int32_t ymdays(int ym) {
    switch (ym) {
        // non-leap year
        case 0: return 31;
        case 1: return 28;
        case 2: return 31;
        case 3: return 30;
        case 4: return 31;
        case 5: return 30;
        case 6: return 31;
        case 7: return 31;
        case 8: return 30;
        case 9: return 31;
        case 10: return 30;
        case 11: return 31;

        // leap year
        case 12: return 31;
        case 13: return 29;
        case 14: return 31;
        case 15: return 30;
        case 16: return 31;
        case 17: return 30;
        case 18: return 31;
        case 19: return 31;
        case 20: return 30;
        case 21: return 31;
        case 22: return 30;
        case 23: return 31;
    }

    return 31; // should never reach here
}

fd_calendar_t fd_calendar_from_time(int32_t time) {
    fd_calendar_t calendar;
    int32_t dayclock;
    int32_t dayno;
    int32_t year;
    int month;
    int offset;
    int32_t ys;
    int32_t sec;

    dayclock = time % 86400;
    sec = dayclock % 60;
    calendar.sec = (int) (sec);
    calendar.min = (int) ((dayclock % 3600) / 60);
    calendar.hour = (int) (dayclock / 3600);
    dayno = time / 86400;
    calendar.wday = (int) ((dayno + 4) % 7); // day 0 was a thursday
    year = 1970;
    while (dayno >= (ys = yearsize(year))) {
        dayno -= ys;
        year++;
    }
    calendar.year = year - 1900;
    offset = leap(year) ? 12 : 0;
    month = 0;
    while (dayno >= ymdays(offset + month)) {
        dayno -= ymdays(offset + month);
        month++;
    }
    calendar.month = month;
    calendar.mday = (int) (dayno + 1);
    return calendar;
}
