#ifndef SUNRISE_SUNSET_H
#define SUNRISE_SUNSET_H

#include <cstdint>

class SunriseSunset {
public:
    struct TimeHM {
        int hour = 0;
        int minute = 0;

        int toMinutes() const { return hour * 60 + minute; }
    };

    struct Result {
        bool hasSunrise = false;
        bool hasSunset = false;

        TimeHM sunriseUtc;
        TimeHM sunsetUtc;

        TimeHM sunriseLocal;
        TimeHM sunsetLocal;
    };

    SunriseSunset() = default;

    void setLocation(double latitude, double longitude);
    double getLatitude() const { return latitude_; }
    double getLongitude() const { return longitude_; }

    void setTimezoneOffsetMinutes(int offsetMinutes);
    int getTimezoneOffsetMinutes() const { return timezoneOffsetMinutes_; }

    Result calculate(int year, int month, int day) const;

    // Utility helpers exposed for unit testing
    static bool isValidLocation(double latitude, double longitude);
    static int dayOfYear(int year, int month, int day);
    static TimeHM minutesToTime(int minutes);
    static int wrapMinutes(int minutes);

private:
    double latitude_ = 0.0;
    double longitude_ = 0.0;
    int timezoneOffsetMinutes_ = 0;

    static bool isLeapYear(int year);

    // NOAA sunrise equation
    static bool calculateUtcMinutes(double latitude, double longitude, int dayOfYear, bool sunrise, int& outMinutesUtc);
};

#endif // SUNRISE_SUNSET_H
