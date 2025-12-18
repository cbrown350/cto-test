#include "SunriseSunset.h"

#include <algorithm>
#include <cmath>

namespace {
static const double kZenithDegrees = 90.833; // official sunrise/sunset

double degToRad(double deg) {
    return deg * M_PI / 180.0;
}

double radToDeg(double rad) {
    return rad * 180.0 / M_PI;
}

double normalizeDegrees(double deg) {
    while (deg < 0.0) deg += 360.0;
    while (deg >= 360.0) deg -= 360.0;
    return deg;
}

double normalizeHours(double hours) {
    while (hours < 0.0) hours += 24.0;
    while (hours >= 24.0) hours -= 24.0;
    return hours;
}
} // namespace

void SunriseSunset::setLocation(double latitude, double longitude) {
    latitude_ = latitude;
    longitude_ = longitude;
}

void SunriseSunset::setTimezoneOffsetMinutes(int offsetMinutes) {
    timezoneOffsetMinutes_ = offsetMinutes;
}

bool SunriseSunset::isValidLocation(double latitude, double longitude) {
    return latitude >= -90.0 && latitude <= 90.0 && longitude >= -180.0 && longitude <= 180.0;
}

bool SunriseSunset::isLeapYear(int year) {
    if ((year % 400) == 0) return true;
    if ((year % 100) == 0) return false;
    return (year % 4) == 0;
}

int SunriseSunset::dayOfYear(int year, int month, int day) {
    static const int kDaysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;

    int doy = 0;
    for (int m = 1; m < month; ++m) {
        doy += kDaysInMonth[m - 1];
        if (m == 2 && isLeapYear(year)) {
            doy += 1;
        }
    }
    doy += day;
    return doy;
}

int SunriseSunset::wrapMinutes(int minutes) {
    int m = minutes % (24 * 60);
    if (m < 0) m += 24 * 60;
    return m;
}

SunriseSunset::TimeHM SunriseSunset::minutesToTime(int minutes) {
    minutes = wrapMinutes(minutes);
    TimeHM t;
    t.hour = minutes / 60;
    t.minute = minutes % 60;
    return t;
}

bool SunriseSunset::calculateUtcMinutes(double latitude, double longitude, int dayOfYearValue, bool sunrise, int& outMinutesUtc) {
    // NOAA sunrise equation (approx 1 minute accuracy for typical latitudes)
    const double lngHour = longitude / 15.0;

    const double t = dayOfYearValue + ((sunrise ? 6.0 : 18.0) - lngHour) / 24.0;

    const double M = (0.9856 * t) - 3.289;

    double L = M + (1.916 * std::sin(degToRad(M))) + (0.020 * std::sin(2.0 * degToRad(M))) + 282.634;
    L = normalizeDegrees(L);

    double RA = radToDeg(std::atan(0.91764 * std::tan(degToRad(L))));
    RA = normalizeDegrees(RA);

    const double Lquadrant = std::floor(L / 90.0) * 90.0;
    const double RAquadrant = std::floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);
    RA /= 15.0;

    const double sinDec = 0.39782 * std::sin(degToRad(L));
    const double cosDec = std::cos(std::asin(sinDec));

    const double cosH = (
        std::cos(degToRad(kZenithDegrees)) - (sinDec * std::sin(degToRad(latitude)))
    ) / (cosDec * std::cos(degToRad(latitude)));

    if (cosH > 1.0) {
        // Sun never rises
        return false;
    }
    if (cosH < -1.0) {
        // Sun never sets
        return false;
    }

    double H = sunrise ? (360.0 - radToDeg(std::acos(cosH))) : radToDeg(std::acos(cosH));
    H /= 15.0;

    const double T = H + RA - (0.06571 * t) - 6.622;
    const double UT = normalizeHours(T - lngHour);

    outMinutesUtc = static_cast<int>(std::round(UT * 60.0));
    outMinutesUtc = wrapMinutes(outMinutesUtc);
    return true;
}

SunriseSunset::Result SunriseSunset::calculate(int year, int month, int day) const {
    Result result;

    if (!isValidLocation(latitude_, longitude_)) {
        return result;
    }

    int doy = dayOfYear(year, month, day);
    if (doy <= 0 || doy > 366) {
        return result;
    }

    int sunriseUtcMinutes = 0;
    int sunsetUtcMinutes = 0;

    result.hasSunrise = calculateUtcMinutes(latitude_, longitude_, doy, true, sunriseUtcMinutes);
    result.hasSunset = calculateUtcMinutes(latitude_, longitude_, doy, false, sunsetUtcMinutes);

    if (result.hasSunrise) {
        result.sunriseUtc = minutesToTime(sunriseUtcMinutes);
        result.sunriseLocal = minutesToTime(sunriseUtcMinutes + timezoneOffsetMinutes_);
    }

    if (result.hasSunset) {
        result.sunsetUtc = minutesToTime(sunsetUtcMinutes);
        result.sunsetLocal = minutesToTime(sunsetUtcMinutes + timezoneOffsetMinutes_);
    }

    return result;
}
