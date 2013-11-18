#pragma once

/*
 * This module is included as a part of libSprawl
 *
 * Copyright (C) 2013 Jaedyn K. Draper
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
