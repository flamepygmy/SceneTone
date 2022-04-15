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

// INCLUDE FILES
#include "scenetonedocument.h"
#include "scenetoneapplication.h"

// ============================ MEMBER FUNCTIONS ===============================

// UID for the ap plication;
// this should correspond to the uid defined in the mmp file
const TUid KUidScenetoneApp = { 0xA0001186 };

// -----------------------------------------------------------------------------
// CScenetoneApplication::CreateDocumentL()
// Creates CApaDocument object
// -----------------------------------------------------------------------------
//
CApaDocument* CScenetoneApplication::CreateDocumentL()
    {
    // Create an Scenetone document, and return a pointer to it
    return (static_cast<CApaDocument*>
                    ( CScenetoneDocument::NewL( *this ) ) );
    }

// -----------------------------------------------------------------------------
// CScenetoneApplication::AppDllUid()
// Returns application UID
// -----------------------------------------------------------------------------
//
TUid CScenetoneApplication::AppDllUid() const
    {
    // Return the UID for the Scenetone application
    return KUidScenetoneApp;
    }
// End of File
