#pragma once

#include "tag.hpp"

namespace sprawl
{
	namespace detail
	{
		namespace time
		{
			static constexpr int64_t EraFromYear(int64_t year)
			{
				return (year >= 0 ? year : year - 399) / 400;
			}

			static constexpr int64_t YearOfEra(int64_t year, int64_t era)
			{
				return year - era * 400;
			}

			static constexpr int64_t DayOfYear(int64_t day, int64_t month)
			{
				return (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
			}

			static constexpr int64_t DayOfEra(int64_t yearOfEra, int64_t dayOfYear)
			{
				return yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
			}

			static constexpr int64_t DaysFromAdjustedCivil(int64_t year, int64_t month, int64_t day)
			{
				return EraFromYear(year) * 146097 + DayOfEra(YearOfEra(year, EraFromYear(year)), DayOfYear(day, month)) - 719468;
			}

			static constexpr int64_t DaysFromCivil(int64_t year, int64_t month, int64_t day)
			{
				return DaysFromAdjustedCivil(year - int64_t(month <= 2), month, day);
			}

			template<typename t_TagType> struct numericMonth_;
			template<> struct numericMonth_<SPRAWL_TAG("Jan")> { static constexpr int64_t value = 1; };
			template<> struct numericMonth_<SPRAWL_TAG("Feb")> { static constexpr int64_t value = 2; };
			template<> struct numericMonth_<SPRAWL_TAG("Mar")> { static constexpr int64_t value = 3; };
			template<> struct numericMonth_<SPRAWL_TAG("Apr")> { static constexpr int64_t value = 4; };
			template<> struct numericMonth_<SPRAWL_TAG("May")> { static constexpr int64_t value = 5; };
			template<> struct numericMonth_<SPRAWL_TAG("Jun")> { static constexpr int64_t value = 6; };
			template<> struct numericMonth_<SPRAWL_TAG("Jul")> { static constexpr int64_t value = 7; };
			template<> struct numericMonth_<SPRAWL_TAG("Aug")> { static constexpr int64_t value = 8; };
			template<> struct numericMonth_<SPRAWL_TAG("Sep")> { static constexpr int64_t value = 9; };
			template<> struct numericMonth_<SPRAWL_TAG("Oct")> { static constexpr int64_t value = 10; };
			template<> struct numericMonth_<SPRAWL_TAG("Nov")> { static constexpr int64_t value = 11; };
			template<> struct numericMonth_<SPRAWL_TAG("Dec")> { static constexpr int64_t value = 12; };
		}

		template<typename t_UnimplementedType>
		struct TagToDateTime;

		template<typename t_MonthTagType, typename t_DayTagType, typename t_YearTagType, typename t_HoursTagType, typename t_MinutesTagType, typename t_SecondsTagType>
		struct TagToDateTime<sprawl::TypeList<t_MonthTagType, t_DayTagType, t_YearTagType, t_HoursTagType, t_MinutesTagType, t_SecondsTagType>>
		{
			static constexpr int64_t value =
				t_SecondsTagType::template As<int64_t>()
				+ t_MinutesTagType::template As<int64_t>() * 60
				+ t_HoursTagType::template As<int64_t>() * 60 * 60
				+ time::DaysFromCivil(t_YearTagType::template As<int64_t>(), time::numericMonth_<t_MonthTagType>::value, t_DayTagType::template As<int64_t>()) * 24 * 60 * 60;
		};

		template<char t_FirstChar, char... t_MoreChars>
		struct TagToDateTime<sprawl::Tag<t_FirstChar, t_MoreChars...>>
		{
			typedef sprawl::Tag<t_FirstChar, t_MoreChars...> TagType;
			static constexpr int64_t value = TagToDateTime<typename TagType::template Replace<SPRAWL_TAG("  "), SPRAWL_TAG(" ")>::template Replace<SPRAWL_TAG(":"), SPRAWL_TAG(" ")>::template Split<SPRAWL_TAG(" ")>>::value;
		};
	}

	typedef SPRAWL_TAG(__DATE__ " " __TIME__) COMPILE_DATE_TIME_TAG;
	constexpr int64_t compileTimestamp = detail::TagToDateTime<COMPILE_DATE_TIME_TAG>::value;
}