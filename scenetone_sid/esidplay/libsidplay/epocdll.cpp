/**
 * @file epocdll This file is a extra interface if libsidplay is built as a DLL.
 *
 * Copyright (c) 1999-2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 */

#include <e32std.h>

GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}

// EOF - EPOCDLL.CPP
