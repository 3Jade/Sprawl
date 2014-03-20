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


#include <sstream>
#include <iostream>
#include <string.h>
#include "format.hpp"

namespace sprawl
{
	namespace format
	{
		inline int toInt(char* s)
		{
			if(*(s+1) == '\0')
				return *s-48;
			else
				return ((*s-48)*10)+toInt(s+1);
		}

		std::string format2(const char* str, std::vector<std::string>& args)
		{
			char s[32768] = "";
			char argnum[32768] = "";
			char* c;
			unsigned int arg = 0;
			unsigned int pos = 0;
			unsigned int argpos = 0;
			for( ; *str != '\0'; str++)
			{
				if(*str=='{')
				{
					if(*(str+1) == '{')
					{
						s[pos++] = '{';
						str++;
					}
					else
					{
						str++;
						while(*str != '}')
						{
							//If we find a non-digit, we have an invalid arg number. Print what we've currently got for argnum, plus the current character.
							if(!isdigit(*str))
							{
								s[pos++] = '{';

								c = argnum;
								for( ; *c != '\0'; c++)
									s[pos++] = *c;

								s[pos++] = *str;

								argpos = 0;

								break;
							}
							argnum[argpos++] = *str;
							str++;
						}
						argnum[argpos] = '\0';
						if(argpos != 0) //If we have found a valid arg number
						{
							arg = toInt(argnum);
							if(arg >= args.size())
							{
								s[pos++] = '{';

								c = argnum;
								for( ; *c != '\0'; c++)
									s[pos++] = *c;

								s[pos++] = '}';
							}
							else
							{
								const char* cc = args[arg].c_str();
								for( ; *cc != '\0'; cc++)
									s[pos++] = *cc;
							}
							argpos = 0;
						}
					}
				}
				else
					s[pos++] = *str;
			}
			s[pos] = '\0';
			return s;
		}

		std::string format(std::string& str)
		{
			return str;
		}

		std::string format(const char* str)
		{
			return str;
		}

		std::string format2(const char* str, std::vector<std::string>& args, std::string& arg1)
		{
			args.push_back(arg1);
			return format2(str, args);
		}

		std::string format(const char* str, std::string& arg1)
		{
			std::vector<std::string> args;
			args.clear();
			args.push_back(arg1);
			return format2(str, args);
		}
	}
}
