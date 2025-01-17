/* -*-c++-*- OpenSceneGraph - Ephemeris Model Copyright (C) 2005 Don Burns
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSG_EPHEMERIS_DATE_TIME_DEF
#define OSG_EPHEMERIS_DATE_TIME_DEF

#include <string>

namespace avSky {

/**\class DateTime
   \brief Data and methods to store, query and set the current date and time.
   */

class DateTime 
{
    public:
        /** Default Constructor */
        DateTime();

        /** Copy Constructor */
        DateTime( const DateTime &dt );

        /** Constructor with parameters 
          \param year  - Actual year, (not since 1900), e.g. 2006
          \param month - Months numbered from 1 (January) to 12 (December)
          \param day   - Day of the month ranges 1 to 31 
          \param hour  - Hour of the day, ranges 0 - 23
          \param minute - minute of the hour, ranges 0 - 59
          \param second - second of the minute, ranges 0 - 59
         */
        DateTime( 
             int year,          // Actual year.  e.g. 2006
             int month,         // Month January = 1, December = 12
             int day,           // 1 - 31
             int hour=0,        // 0 - 23
             int minute = 0,    // 0 - 59
             int second = 0 );  // 0 - 59


        /** Constructor with parameter
            \param struct tm - A filled in struct tm (time.h) structure.
            */
        DateTime( const struct tm & );

        /**
          Set the current date and time to NOW.
          */
        void now();

        /**
          get the Modified Julian Date based on GMT, from the current date and time.
          */
        double      getModifiedJulianDate() const; 

        /**
          Get the Greenwhich Mean Time from the current date and time.
          */
        DateTime    getGMT() const;

        /**
          Set the current month
          \param year - Actual year, e.g. 2006.
          */
        void setYear( int year);
        /**
          Get the current year.
          */
        int getYear() const;

        /** 
          Set the month.
          \param month - An integer ranging from 1= January to 12 = December.
          */
        void setMonth( int month ); 
        /**
          Get the current month.
          */
        int getMonth() const;
        /** 
          Get a std::string representing the name of the current month.
          */
        std::string getMonthString() const;
        /**
          Static method to get the name of a month, based on a parameter.
          \param month - An integer ranging from 1= January to 12 = December.
          */
        static std::string getMonthString(int month);

        /**
          Set the day of the month. 
          \param day - An integer ranging from 1 to 31.
          */
        void setDayOfMonth( int day);

        /**
          Get the day of the month
          */
        int getDayOfMonth() const;

        /**
          Get the day of the year, range 1 - 366 based on the current date.
          */
        int getDayOfYear() const;
        /**
          Get the day of the week range 1 - 7 base on the current date.
          */
        int getDayOfWeek() const;
        /** 
          Get an std::string with the name of the day of the week based on the current date.
          */
        std::string getDayOfWeekString() const;
        /**
          Static method to get the name of day of the week with a parameter.
          \param weekday - An integer range 1 - 7.
          */
        static std::string getDayOfWeekString( int weekday );

        /**
          Set the current hour of the day
          \param hr - An integer ranging 0 - 23.
          */
        void setHour( int hr );
        /**
          Get the current hour.  Returns an integer ranging 0 - 23.
          */
        int getHour() const;

        /**
          Set the current minute of the hour
          \param min - An integer ranging 0 - 59.
          */
        void setMinute( int  min);
        /**
          Get the current minute of the hour.  Returns an integer ranging 0 - 59.
          */
        int getMinute() const;

        /**
          Set the current second of the minute
          \param sec - An integer ranging 0 - 59.
          */
        void setSecond( int sec );
        /**
          Get the current second of the minute.  Returns an integer ranging 0 - 59.
          */
        int getSecond() const;

        /**
          Return a boolean value indicating whether the currently set date falls during 
          daylight savings time.
          */
        bool isDaylightSavingsTime() const;

    private:
        struct tm _tm;
        static char *weekDayNames[7];
        static char *monthNames[12];
};


inline std::ostream& operator << (std::ostream& output, const DateTime& dt)
{
    output << dt.getDayOfWeekString() << " " << dt.getMonthString() << " " <<
              dt.getDayOfMonth() << " " <<
              dt.getHour() << ":" << dt.getMinute() << ":" << dt.getSecond() << 
              " " << dt.getYear(); 

    return output;     // to enable cascading
}



}

#endif
