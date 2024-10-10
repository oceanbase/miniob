#include "date.h"
#include <sstream>

Date Date::from_string(std::string const &date_str) {
    Date date;
    char delimiter1, delimiter2;
    int month_int, day_int;

    // Use std::istringstream to parse the input
    std::istringstream ss(date_str);

    // Parse year, delimiter, month, second delimiter, and day
    if (!(ss >> date.year >> delimiter1 >> month_int >> delimiter2 >> day_int) ||
        delimiter1 != '-' || delimiter2 != '-') {
        throw std::invalid_argument("Invalid date format");
    }

    // Safely cast month and day to uint8_t after validation
    date.month = static_cast<uint8_t>(month_int);
    date.day = static_cast<uint8_t>(day_int);

    if (!date.isValid()) {
        throw std::invalid_argument("Invalid date format");
    }

    return date;
}
