/*
	BmPrefsSendMailView.cpp
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

#include <Alert.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <layout-all.h>

#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmPrefsSendMailView.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSendAccItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_SERVER,
	COL_AUTH_METHOD,
	COL_USER,
	COL_PWD,
	COL_DOMAIN,
	COL_PORT
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccItem::BmSendAccItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item, false)
{
	UpdateView( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccItem::~BmSendAccItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSendAccItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmSendAcc* acc = ModelItem();
	if (flags & UPD_ALL) {
		BmListColumn cols[] = {
			{ acc->Key().String(),						false },
			{ acc->SMTPServer().String(),				false },
			{ acc->AuthMethod().String(),				false },
			{ acc->Username().String(),				false },
			{ acc->PwdStoredOnDisk() ? "*****":"",	false },
			{ acc->DomainToAnnounce().String(),		false },
			{ acc->PortNrString().String(),			true  },
			{ NULL, false }
		};
		SetTextCols( 0, cols, !ThePrefs->GetBool("StripedListView"));
	}
}



/********************************************************************************\
	BmSendAccView
\********************************************************************************/

BmSendAccView* BmSendAccView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccView* BmSendAccView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmSendAccView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccView::BmSendAccView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_SendAccView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true, true, false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;

	Initialize( BRect( 0,0,width-1,height-1),
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "Account", 80.0, CLV_SORT_KEYABLE|flags, 50.0));
	AddColumn( new CLVColumn( "Server", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Auth-Method", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "User", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Pwd", 50.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Domain", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Port", 40.0, flags, 40.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccView::~BmSendAccView() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSendAccView::CreateListViewItem( BmListModelItem* item,
																		BMessage* archive) {
	return new BmSendAccItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmSendAccView::CreateContainer( bool horizontal, bool vertical, 
												  				  bool scroll_view_corner, 
												  				  border_style border, 
																  uint32 ResizingMode, 
																  uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView, 
											 be_plain_font->StringWidth(" 99 accounts "));
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSendAccView::AddModelItem( BmListModelItem* item) {
	BmListViewItem* viewItem = inherited::AddModelItem( item);
	if (viewItem) {
		Select( IndexOf(viewItem));
		ScrollToSelection();
	}
	return viewItem;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSendAccView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("SendAccView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsSendMailView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSendMailView::BmPrefsSendMailView() 
	:	inherited( "Sending Mail-Accounts (SMTP)")
{
	MView* view = 
		new VGroup(
			CreateAccListView( minimax(500,60,1E5,1E5), 500, 80),
			new HGroup(
				mAddButton = new MButton("Add Account", new BMessage(BM_ADD_ACCOUNT), this),
				mRemoveButton = new MButton("Remove Account", new BMessage( BM_REMOVE_ACCOUNT), this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Account Info",
					new VGroup(
						mAccountControl = new BmTextControl( "Account name:", false, 0, 35),
						new Space( minimax(0,5,0,5)),
						mServerControl = new BmTextControl( "Server:"),
						mPortControl = new BmTextControl( "Port:"),
						new Space( minimax(0,5,0,5)),
						mAuthControl = new BmMenuControl( "Auth-method:", new BPopUpMenu("")),
						mLoginControl = new BmTextControl( "Login:"),
						mPwdControl = new BmTextControl( "Password:"),
						new Space( minimax(0,5,0,5)),
						mDomainControl = new BmTextControl( "Domain to announce:"),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Options",
						new VGroup(
							mStorePwdControl = new BmCheckControl( "Store password on disk (UNSAFE!)", 
																				new BMessage(BM_PWD_STORED_CHANGED), 
																				this),
							0
						)
					),
					new Space(),
					0
				),
				0
			),
			new Space(minimax(0,0,1E5,1E5,0.5)),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	mPwdControl->TextView()->HideTyping( true);

	float divider = mAccountControl->Divider();
	divider = MAX( divider, mDomainControl->Divider());
	divider = MAX( divider, mLoginControl->Divider());
	divider = MAX( divider, mPortControl->Divider());
	divider = MAX( divider, mPwdControl->Divider());
	divider = MAX( divider, mServerControl->Divider());
	divider = MAX( divider, mAuthControl->Divider());
	mAccountControl->SetDivider( divider);
	mDomainControl->SetDivider( divider);
	mLoginControl->SetDivider( divider);
	mPortControl->SetDivider( divider);
	mPwdControl->SetDivider( divider);
	mServerControl->SetDivider( divider);
	mAuthControl->SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSendMailView::~BmPrefsSendMailView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::Initialize() {
	inherited::Initialize();

	mAccountControl->SetTarget( this);
	mDomainControl->SetTarget( this);
	mLoginControl->SetTarget( this);
	mPortControl->SetTarget( this);
	mPwdControl->SetTarget( this);
	mServerControl->SetTarget( this);
	mStorePwdControl->SetTarget( this);

	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( "", new BMessage(BM_AUTH_SELECTED)), this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_SMTP_AFTER_POP, new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_PLAIN, new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_LOGIN, new BMessage(BM_AUTH_SELECTED)), 
						this);

	mAccListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mAccListView->SetTarget( this);
	mAccListView->StartJob( TheSmtpAccountList.Get());
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::Activated() {
	inherited::Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::WriteStateInfo() {
	mAccListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::SaveData() {
	TheSmtpAccountList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::UndoChanges() {
	TheSmtpAccountList->Cleanup();
	TheSmtpAccountList->StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mAccListView->CurrentSelection( 0);
				ShowAccount( index);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				if (mCurrAcc) {
					BView* srcView = NULL;
					msg->FindPointer( "source", (void**)&srcView);
					BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
					if ( source == mAccountControl) {
						if (!TheSmtpAccountList->RenameItem( mCurrAcc->Name(), mAccountControl->Text())) {
							for( int32 i = 1;
								  !TheSmtpAccountList->RenameItem( mCurrAcc->Name(), BString(mAccountControl->Text())<<"_"<<i);
								  ++i) {
							}
						}
					} else if ( source == mDomainControl)
						mCurrAcc->DomainToAnnounce( mDomainControl->Text());
					else if ( source == mLoginControl)
						mCurrAcc->Username( mLoginControl->Text());
					else if ( source == mPortControl)
						mCurrAcc->PortNr( atoi(mPortControl->Text()));
					else if ( source == mPwdControl)
						mCurrAcc->Password( mPwdControl->Text());
					else if ( source == mServerControl)
						mCurrAcc->SMTPServer( mServerControl->Text());
				}
				break;
			}
			case BM_PWD_STORED_CHANGED: {
				bool val = mStorePwdControl->Value();
				mPwdControl->SetEnabled( val);
				if (!val)
					mPwdControl->SetText("");
				if (mCurrAcc)
					mCurrAcc->PwdStoredOnDisk( val);
				break;
			}
			case BM_AUTH_SELECTED: {
				BMenuItem* item = mAuthControl->Menu()->FindMarked();
				if (item)
					mCurrAcc->AuthMethod( item->Label());
				else
					mCurrAcc->AuthMethod( "");
				break;
			}
			case BM_ADD_ACCOUNT: {
				BString key( "new account");
				for( int32 i=1; TheSmtpAccountList->FindItemByKey( key); ++i) {
					key = BString("new account_")<<i;
				}
				TheSmtpAccountList->AddItemToList( new BmSmtpAccount( key.String(), TheSmtpAccountList.Get()));
				break;
			}
			case BM_REMOVE_ACCOUNT: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Mail-Account", 
														 (BString("Are you sure about removing the account <") << mCurrAcc->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_ACCOUNT), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheSmtpAccountList->RemoveItemFromList( mCurrAcc.Get());
						mCurrAcc = NULL;
					}
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::ShowAccount( int32 selection) {
	bool enabled = (selection != -1);
	mAccountControl->SetEnabled( enabled);
	mDomainControl->SetEnabled( enabled);
	mLoginControl->SetEnabled( enabled);
	mPortControl->SetEnabled( enabled);
	mServerControl->SetEnabled( enabled);
	mAuthControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	mStorePwdControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrAcc = NULL;
		mAccountControl->SetTextSilently( "");
		mDomainControl->SetTextSilently( "");
		mLoginControl->SetTextSilently( "");
		mPortControl->SetTextSilently( "");
		mPwdControl->SetTextSilently( "");
		mServerControl->SetTextSilently( "");
		mAuthControl->ClearMark();
		mStorePwdControl->SetValue( 0);
		mPwdControl->SetEnabled( false);
	} else {
		BmSendAccItem* accItem = dynamic_cast<BmSendAccItem*>(mAccListView->ItemAt( selection));
		if (accItem) {
			mCurrAcc = dynamic_cast<BmSmtpAccount*>(accItem->ModelItem());
			if (mCurrAcc) {
				mAccountControl->SetTextSilently( mCurrAcc->Name().String());
				mDomainControl->SetTextSilently( mCurrAcc->DomainToAnnounce().String());
				mLoginControl->SetTextSilently( mCurrAcc->Username().String());
				mPortControl->SetTextSilently( mCurrAcc->PortNrString().String());
				mPwdControl->SetTextSilently( mCurrAcc->Password().String());
				mServerControl->SetTextSilently( mCurrAcc->SMTPServer().String());
				mAuthControl->MarkItem( mCurrAcc->AuthMethod().String());
				mStorePwdControl->SetValue( mCurrAcc->PwdStoredOnDisk());
				mPwdControl->SetEnabled( mCurrAcc->PwdStoredOnDisk());
			}
		} else
			mCurrAcc = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsSendMailView::CreateAccListView( minimax minmax, int32 width, int32 height) {
	mAccListView = BmSendAccView::CreateInstance( minmax, width, height);
	return mAccListView->ContainerView();
}
