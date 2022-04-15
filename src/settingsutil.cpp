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

#include "settingsutil.h"
#include "f32file.h"
#include "s32file.h"

CSettingsUtil::~CSettingsUtil()
{
	// todo: bring down the descriptor array properly...
}

CSettingsUtil::CSettingsUtil()
{
	iChanged = EFalse;
}


CSettingsUtil* CSettingsUtil::NewL()
{
	CSettingsUtil *self = new (ELeave) CSettingsUtil();
	return self;	
}

TInt CSettingsUtil::GetSetting( const TDesC &aKey, TDes16 &aDestination )
{
	TInt i;
	for(i=0;i<iSettingsKeys->MdcaCount();i++)
	{
		if(	(*iSettingsKeys)[i] == aKey )
		{
			aDestination = (*iSettingsValues)[i];
			return KErrNone;
		}
	}
	return KErrNotFound;
}

TInt CSettingsUtil::SetSetting( const TDesC &aKey, const TDesC16 &aValue )
{
	// First scan through if we have a setting by that name already
	for(TInt i=0;i<iSettingsKeys->MdcaCount();i++)
	{
		if( (*iSettingsKeys)[i] == aKey )
		{
			// yes.. delete existing one
			iSettingsValues->Delete(i);
			iSettingsKeys->Delete(i);
			break;		
		}
	}
		
	iSettingsKeys->AppendL(aKey);
	iSettingsValues->AppendL(aValue);

	iChanged = ETrue;
}

TInt CSettingsUtil::StoreSettings(const TDesC &aFilename)
{
	// This is not quite right, as the filename might be different from what it was
	// for ReadSettings -> change filename to be part of constructor...
	if(iChanged)
	{
		RFs session;
		session.Connect();
	
		RFile file;
		TInt err = file.Replace(session, aFilename, EFileWrite);
		if(err) return err;
	
		RFileWriteStream stream(file);
	
		for(TInt i=0 ; i < iSettingsKeys->MdcaCount() ; i++)
		{
			stream.WriteUint32L(((*iSettingsKeys)[i]).Length());
			stream.WriteL((*iSettingsKeys)[i]);

			stream.WriteUint32L(((*iSettingsValues)[i]).Length());
			stream.WriteL((*iSettingsValues)[i]);
		}

		stream.Close();
		file.Close();
	}
}

TInt CSettingsUtil::ReadSettings(const TDesC &aFilename)
{
	RFs session;
	session.Connect();
	
	RFile file;
	TInt err = file.Open(session, aFilename, EFileRead);
	if(err) return err;
	
	RFileReadStream stream(file);

	iSettingsKeys   = new (ELeave) CDesCArrayFlat(8);
	iSettingsValues = new (ELeave) CDesCArrayFlat(8);
	 
	while(1)
	{
		TInt len;

		TRAP(err, len = stream.ReadUint32L());
		if(err) break;

		HBufC16 *hbuf = HBufC16::NewL(len);
		TPtr16 dessi1(hbuf->Des());
		TRAP(err, stream.ReadL(dessi1, len));
		iSettingsKeys->AppendL(dessi1);

		TRAP(err, len = stream.ReadUint32L());
		// TODO: handle this error case...
		hbuf = HBufC16::NewL(len);
	    TPtr16 dessi2(hbuf->Des());
		TRAP(err,stream.ReadL(dessi2, len));
		iSettingsValues->AppendL(dessi2);
		}

	stream.Close();
	file.Close();
	return KErrNone;
}


//	CDesCArrayFlat *iSettingsKeys;
//	CDesCArrayFlat *iSettingsValues;
//};