/************
 ** 	from http://www.beatjapan.org/mirror/www.be.com/beware/Utilities/uptime.html
 **	archive ftp://ftp.halcyon.com/pub/users/jrashmun/uptime.zip
 **     License : Freeware
*************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <SupportDefs.h>
#include <OS.h>

void Usage( );

void Usage( )
{
   printf( "\nUsage:  uptime [-u]\n" );
   printf( "Prints the current date and time, as well\n" );
   printf( "as the time elapsed since the system was started.\n" );
   printf( "The optional argument omits the current date and time.\n\n" );
}

int main( int argc, char* argv[] )
{
   char            buf[255];
   const char*     day_strings[] = { "day", "days" };
   const char*     hour_strings[] = { "hour", "hours" };
   const char*     minute_strings[] = { "minute", "minutes" };
   const char*     day_string = 0;
   const char*     hour_string = 0;
   const char*     minute_string = 0;
   int             print_uptime_only = 0;
   time_t          current_time = 0;
   const time_t    secs_per_day = 86400;
   const time_t    secs_per_hour = 3600;
   const time_t    secs_per_minute = 60;
   time_t          uptime_days = 0;
   time_t          uptime_hours = 0;
   time_t          uptime_minutes = 0;
   time_t          uptime_secs = 0;
   const bigtime_t usecs_per_sec = 1000000L;
   bigtime_t       uptime_usecs = 0;
   bigtime_t       usecs_since_sec = 0;

   if( argc > 2 )
   {
      Usage( );
      return 1;
   }

   if( argc == 2 )
   {
      if( !strcmp( argv[1], "-u" ) )
         print_uptime_only = 1;
      else
      {
         Usage( );
         return 2;
      }
   }

   uptime_usecs = system_time( );

   uptime_secs = uptime_usecs / usecs_per_sec;
   usecs_since_sec = uptime_usecs % usecs_per_sec;
   if( usecs_since_sec >= (usecs_per_sec / 2) )
      uptime_secs++;

   uptime_days = uptime_secs / secs_per_day;
   uptime_hours = (uptime_secs % secs_per_day) / secs_per_hour;
   uptime_minutes = (uptime_secs % secs_per_hour) / secs_per_minute;

   current_time = time( 0 );

   if( !print_uptime_only )
   {
      strcpy( buf, ctime( &current_time ) );

      buf[strlen( buf ) - 1] = '\0';
   }

   day_string = (uptime_days == 1) ? day_strings[0] : day_strings[1];
   hour_string = (uptime_hours == 1) ? hour_strings[0] : hour_strings[1];
   minute_string = (uptime_minutes == 1) ? minute_strings[0] : minute_strings[1];

   if( uptime_days && uptime_hours && uptime_minutes )
   {
      if( !print_uptime_only )
         printf( "%s, up %ld %s, %ld %s, %ld %s\n", buf,
                 uptime_days, day_string,
                 uptime_hours, hour_string,
                 uptime_minutes, minute_string );
      else
         printf( "up %ld %s, %ld %s, %ld %s\n",
                 uptime_days, day_string,
                 uptime_hours, hour_string,
                 uptime_minutes, minute_string );
   }
   else if( !uptime_days )
   {
      if( uptime_hours && uptime_minutes )
      {
         if( !print_uptime_only )
            printf( "%s, up %ld %s, %ld %s\n", buf,
                    uptime_hours, hour_string,
                    uptime_minutes, minute_string );
         else
            printf( "up %ld %s, %ld %s\n",
                    uptime_hours, hour_string,
                    uptime_minutes, minute_string );
      }
      else if( uptime_hours )
      {
         if( !print_uptime_only )
            printf( "%s, up %ld %s\n", buf,
                    uptime_hours, hour_string );
         else
            printf( "up %ld %s\n",
                    uptime_hours, hour_string );
      }
      else
      {
         if( !print_uptime_only )
            printf( "%s, up %ld %s\n", buf,
                    uptime_minutes, minute_string );
         else
            printf( "up %ld %s\n",
                    uptime_minutes, minute_string );
      }
   }
   else if( uptime_hours )
   {
      if( !print_uptime_only )
         printf( "%s, up %ld %s, %ld %s\n", buf,
                 uptime_days, day_string,
                 uptime_hours, hour_string );
      else
         printf( "up %ld %s, %ld %s\n",
                 uptime_days, day_string,
                 uptime_hours, hour_string );
   }
   else if( uptime_minutes )
   {
      if( !print_uptime_only )
         printf( "%s, up %ld %s, %ld %s\n", buf,
                 uptime_days, day_string,
                 uptime_minutes, minute_string );
      else
         printf( "up %ld %s, %ld %s\n",
                 uptime_days, day_string,
                 uptime_minutes, minute_string );
   }
   else
   {
      if( !print_uptime_only )
         printf( "%s, up %ld %s\n", buf,
                 uptime_days, day_string );
      else
         printf( "up %ld %s\n",
                 uptime_days, day_string );
   }

   return 0;
}

