#pragma once

/*************************
 *     Sprawl::Format    *
 *************************
 *      Main Module      *
 *      Header File      *
 *************************/

#include <vector>
#include <sstream>
#include <iostream>

namespace sprawl
{
	namespace format
	{
		std::string format2(const char *str, std::vector<std::string> &args);

		std::string format2(const char *str, std::vector<std::string> &args, std::string &arg1);

		template<typename T>
		inline std::string format2(const char *str, std::vector<std::string> &args, T &arg1)
		{
			std::stringstream s;
			s.str("");
			s << arg1;
			args.push_back(s.str());
			return format2(str, args);
		}

		template<typename T, typename... Params>
		inline std::string format2(const char *str, std::vector<std::string> &args, T &arg1, Params&... params)
		{
			std::stringstream s;
			s.str("");
			s << arg1;
			args.push_back(s.str());
			return format2(str, args, params...);
		}

		template<typename... Params>
		inline std::string format2(const char *str, std::vector<std::string> &args, std::string &arg1, Params&... params)
		{
			args.push_back(arg1);
			return format2(str, args, params...);
		}

		std::string format(std::string str);

		std::string format(const char* str);

		std::string format(const char* str, std::string &arg1);

		template<typename T>
		inline std::string format(const char* str, T arg1)
		{
			std::vector<std::string> args;
			std::stringstream s;
			s.str("");
			s << arg1;
			args.push_back(s.str());
			return format2(str, args);
		}

		template<typename... Params>
		inline std::string format(const char *str, std::string &arg1, Params... params)
		{
			std::vector<std::string> args;
			args.push_back(arg1);
			return format2(str, args, params...);
		}

		template<typename T, typename... Params>
		inline std::string format(const char *str, T arg1, Params... params)
		{
			std::vector<std::string> args;
			std::stringstream s;
			s.str("");
			s << arg1;
			args.push_back(s.str());
			return format2(str, args, params...);
		}

		template<typename T>
		inline std::string format(std::string &str, T arg1)
		{
			return format(str.c_str(), arg1);
		}

		template<typename T, typename... Params>
		inline std::string format(std::string &str, Params... params)
		{
			return format(str.c_str(), params...);
		}
	}
}
