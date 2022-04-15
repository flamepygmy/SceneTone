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
#include "scenetoneappui.h"
#include "scenetonedocument.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CScenetoneDocument::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CScenetoneDocument* CScenetoneDocument::NewL( CEikApplication&
                                                          aApp )
    {
    CScenetoneDocument* self = NewLC( aApp );
    CleanupStack::Pop( self );
    return self;
    }

// -----------------------------------------------------------------------------
// CScenetoneDocument::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CScenetoneDocument* CScenetoneDocument::NewLC( CEikApplication&
                                                           aApp )
    {
    CScenetoneDocument* self =
        new ( ELeave ) CScenetoneDocument( aApp );

    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// -----------------------------------------------------------------------------
// CScenetoneDocument::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CScenetoneDocument::ConstructL()
    {
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CScenetoneDocument::CScenetoneDocument()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CScenetoneDocument::CScenetoneDocument( CEikApplication& aApp )
    : CAknDocument( aApp )
    {
    // No implementation required
    }

// ---------------------------------------------------------------------------
// CScenetoneDocument::~CScenetoneDocument()
// Destructor.
// ---------------------------------------------------------------------------
//
CScenetoneDocument::~CScenetoneDocument()
    {
    // No implementation required
    }

// ---------------------------------------------------------------------------
// CScenetoneDocument::CreateAppUiL()
// Constructs CreateAppUi.
// ---------------------------------------------------------------------------
//
CEikAppUi* CScenetoneDocument::CreateAppUiL()
    {
    // Create the application user interface, and return a pointer to it;
    // the framework takes ownership of this object
    return ( static_cast <CEikAppUi*> ( new ( ELeave )
                                        CScenetoneAppUi ) );
    }

// End of File
