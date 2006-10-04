//header file for hstring class

#ifndef __HSTRING_H
#define __HSTRING_H

#include <string>

typedef std::string hstring;

/*  working class definition
class hstring : public std::string
{
public:

	hstring(void);
	//hstring(const char* inChar);

	//operators

	//pointering this class should have us return the char string version of the stored string.
	const char* operator*() const
	{
		return c_str();
	}

	bool operator!()
	{
		return (size()==0);
	}

	//used to blank out string in g_navigator.cpp (std::string)
	//hstring& operator=(int a); 

	//(std::string)
	//hstring& operator=(char* a);
};
*/

#endif