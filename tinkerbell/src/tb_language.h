// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#ifndef TB_LANGUAGE_H
#define TB_LANGUAGE_H

#include "tinkerbell.h"
#include "tb_hashtable.h"

namespace tinkerbell {

/** TBLanguage is a basic language string manager.
	Strings read into it can be looked up from a TBID, so either by a number
	or the hash number from a string (done by TBID).

	Ex: GetString(10)      (Get the string with id 10)
	Ex: GetString("new")   (Get the string with id new)

	In UI resources, you can refer to strings from language lookup by preceding a string with @.

	Ex: TBButton: text: @close   (Create a button with text from lookup of "close")
*/
	
class TBLanguage
{
public:
	~TBLanguage();

	/** Load a file into this language manager.
		Note: This *adds* strings read from the file, without clearing any existing
		strings first. */
	bool Load(const char *filename);

	/** Clear the list of strings. */
	void Clear();

	const char *GetString(const TBID &id);
private:
	TBHashTableOf<TBStr> strings;
};

};

#endif // TB_LANGUAGE_H