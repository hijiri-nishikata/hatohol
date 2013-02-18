/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SQLWhereParser_h
#define SQLWhereParser_h

#include "SQLFormulaParser.h"

class SQLWhereParser : public SQLFormulaParser
{
public:
	static void init(void);

	SQLWhereParser(void);
	virtual ~SQLWhereParser();
	virtual bool add(string& word, string &wordLower);

protected:
	//
	// general sub routines
	//
	void clearContext(void);
	bool createBetweenElement(void);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbEqual(const char separator,
	                              SQLWhereParser *whereParser);
	void separatorCbEqual(const char separator);

	static void _separatorCbGreaterThan(const char separator,
	                                    SQLWhereParser *whereParser);
	void separatorCbGreaterThan(const char separator);

	//
	// Keyword handlers
	//
	bool kwHandlerBetween(void);

private:
	static KeywordHandlerMap m_keywordHandlerMap;

	struct PrivateContext;
	PrivateContext          *m_ctx;
};

#endif // SQLWhereParser_h


