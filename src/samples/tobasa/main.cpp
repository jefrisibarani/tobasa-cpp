#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/format.h>
#include <tobasa/util_date.h>

int main()
{
   try
   {
      if (! tbs::DateTime::initTimezoneData())
         return 1;
         
      tbs::DateTime dt;
      auto dts = tbsfmt::format("Current time is {}", dt.isoDateTimeString());
#if defined(TOBASA_USE_STD_DATE)
      std::cout << "Hello World\n" << dts;
#else
      tbsfmt::print("Hello World\n{}\n", dts );
#endif

      dt.parseTime("05:23:42", "%H:%M:%S");
      auto dateStr = dt.isoDateString();
      auto timeStr = dt.isoTimeString();


      // simple test
      auto currentTime     = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
      auto nextYear        = currentTime + tbsdate::years{ 1 };
      auto currentTimeStr  = tbs::util::formatDate("{:%Y-%m-%d %H:%M:%S}", currentTime);
      auto nextYearStr     = tbs::util::formatDate("{:%Y-%m-%d %H:%M:%S}", nextYear);
      auto currentTimeStr2 = tbs::util::formatDateNow("{:%Y-%m-%d %H:%M:%S}");


      // Null date test
      tbs::DateTime dt2;
      dt2.setToNullDateTime();
      // should null
      std::cout << "Null date value    : " << dt2.isoDateString()     << std::endl;
      //should null
      std::cout << "Null datetime value: " << dt2.isoDateTimeString() << std::endl;

      // Failed parse() test
      tbs::DateTime dt3;
      dt3.parse("05X23R42S");
      // should print null
      std::cout << "Failed parse() value: " << dt3.isoDateString() << std::endl;
      //should print null
      std::cout << "Failed parse() value: " << dt3.isoDateTimeString() << std::endl;

      tbs::DateTime dt4;
      dt4.parse("00-00-00 00:00:00");
      std::cout << "parsing 00-00-00 00:00:00 value: " << dt4.isoDateString() << std::endl;
      // should null
      std::cout << "parsing 00-00-00 00:00:00 is null date     RESULT: " << dt4.isoDateString() << std::endl;

      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }

}