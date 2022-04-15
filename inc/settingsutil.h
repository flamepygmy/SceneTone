//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006  Jani Vaarala <flame@pygmyprojects.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  --------------------------------------------------------------------------

#if !defined(__SETTINGSUTIL_H__)
#define __SETTINGSUTIL_H__

#include <e32std.h>
#include <e32cmn.h>
#include <badesca.h>

class CSettingsUtil : public CBase
{
  public:

	static CSettingsUtil* NewL();
	TInt GetSetting( const TDesC &aKey, TDes16 &aDestination );
	TInt SetSetting( const TDesC &aKey, const TDesC16 &aValue );
	TInt StoreSettings(const TDesC &aFilename);
	TInt ReadSettings(const TDesC &aFilename);

  private:
  	~CSettingsUtil();
  	 CSettingsUtil();

	CDesCArrayFlat *iSettingsKeys;
	CDesCArrayFlat *iSettingsValues;

	TBool iChanged;
};

#endif // __SETTINGSUTIL_H__
