/*
	BmFilterAddon.h

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#ifndef _BmFilterAddon_h
#define _BmFilterAddon_h

#include <Message.h>

#include "BmBase.h"
#include "BmString.h"

#define BM_FILL_MENU_FROM_LIST	'bmFM'

#define MSG_LIST_NAME 				"bm:lstnm"
#define MSG_MENU_POINTER			"bm:mptr"
#define MSG_MENU_TARGET				"bm:trgt"
#define MSG_MSG_TEMPLATE			"bm:tmpl"

#define BM_FOLDERLIST_NAME			"FolderList"
#define BM_STATUSLIST_NAME			"StatusList"
#define BM_IDENTITYLIST_NAME		"IdentityList"

/*------------------------------------------------------------------------------*\
	BmFilterAddon 
		-	base class for all filter-addons, this is used as filter-addon-API
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmFilterAddon {

public:
	BmFilterAddon();
	virtual ~BmFilterAddon();
	
	// native methods:
	virtual bool Execute( void* msgContext) = 0;
	virtual bool SanityCheck( BmString& complaint, BmString& fieldName) = 0;
	virtual status_t Archive( BMessage* archive, bool deep = true) const = 0;
	virtual BmString ErrorString() const = 0;

private:

	// Hide copy-constructor:
	BmFilterAddon( const BmFilterAddon&);
};

typedef BmFilterAddon* (*BmInstantiateFilterFunc)( const BmString& name, 
																	const BMessage* archive,
																	const BmString& kind);



/*------------------------------------------------------------------------------*\
	BmMsgContext
		-	
\*------------------------------------------------------------------------------*/
struct IMPEXPBMBASE BmHeaderInfo {
	BmString fieldName;
	const char** values;
};
	
struct IMPEXPBMBASE BmMsgContext {
	BmMsgContext( const BmString& mtxt, const BmString& mid);
	~BmMsgContext();

	const BmString& rawMsgText;
	const BmString& mailId;
	
	int32 headerInfoCount;
	BmHeaderInfo *headerInfos;
	
	BmString folderName;
	BmString status;
	BmString identity;
	bool moveToTrash;
	bool stopProcessing;
};

#endif