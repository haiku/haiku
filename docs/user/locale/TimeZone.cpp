/*!
\class BTimeZone
\ingroup locale
\brief Class holding information for a time zone.

*/

/*!
\fn BTimeZone::BTimeZone(const char* zoneCode)
\brief Construct a timezone from its code.

The constructor only allows you to construct a timezone if you already know its
code. If you don't know the code, you can instead go through the BCountry class
which can enumerate all timezones in a country, or use the BLocaleRoster, which
knows the timezone selected by the user.
*/

/*!
\fn const BString& BTimeZone::Code() const
\brief Returns the timezone code.

Note different time zones with different codes may have the same rules.
*/

/*!
\fn const BString& BTimeZone::Name() const
\brief Returns the localized name of the time zone

Use this for displaying information to the user.
*/

/*!
\fn const BString& BTimeZone::DaylightSavingName() const
\brief Return the name of the daylight savings rules used in this timezone.
*/

/*!
\fn const BString& BTimeZone::ShortName() const
\brief Return the short name of the timezone, in the user's locale.
*/

/*!
\fn const BString& BTimeZone::DaylightSavingName() const
\brief Return the short name of the daylight savings rules used in this
timezone.
*/

/*!
\fn int BTimeZone::OffsetFromGMT() const
\brief Return the offset from GMT.

The offset is a number of seconds, positive or negative.
*/

/*!
\fn bool BTimeZone::SupportsDaylightSaving() const
\brief Return true if the time zone has daylight saving rules
*/

/*!
\fn status_t BTimeZone::InitCheck() const
\brief Return false if there was an error creating the timezone (you called the
constructor or SetTo with an invalid code).
*/

/*!
\fn status_t BTimeZone::SetTo(const char* zoneCode)
\brief Set the timezone to another code.

\returns false if there was an error (likely you given an invalid code)
*/

