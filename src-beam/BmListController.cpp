/*
	BmListController.cpp
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


#include <memory.h>

#include <Autolock.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <Window.h>

#include "CLVColumnLabelView.h"

#include "BmBasics.h"
#include "BmBusyView.h"
#include "BmCaption.h"
#include "BmDataModel.h"
#include "BmListController.h"
#include "BmLogHandler.h"
#include "BmResources.h"
#include "BmUtil.h"

#define BM_PULSE 'bmPL'
static BMessage pulseMsg(BM_PULSE);

/********************************************************************************\
	BmListViewItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::BmListViewItem( BString& key, BmListModelItem* modelItem,
										  bool hierarchical, BMessage* archive)
	:	inherited( 0, !modelItem->empty(), false, MAX( TheResources->FontLineHeight(), 18))
	,	mKey( key)
	,	mModelItem( modelItem)
{
	SetExpanded( archive ? archive->FindBool( MSG_EXPANDED) : false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem::~BmListViewItem() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewItem::Archive( BMessage* archive, bool deep) const {
	// first we archive the item's attributes into a separate message...
	BMessage msg;
	msg.AddBool( MSG_EXPANDED, IsExpanded());

	// ...now we add this message with a corresponding key to the archive:
	status_t ret = archive->AddMessage( MSG_CHILDREN, &msg)
						|| archive->AddString( MSG_CHILDNAMES, Key());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::UpdateView( BmUpdFlags flags) {
	BmListModelItem* item = ModelItem();
	if (flags & (UPD_EXPANDER)) {
		SetSuperItem( item->size() != 0);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewItem::SetTextCols( int16 firstTextCol, BmListColumn* columnVec,
											 bool truncate) {
	int16 offs = firstTextCol;
	for( const BmListColumn* p = columnVec; p->text != NULL; ++p) {
		SetColumnContent( offs++, p->text, truncate, p->rightJustified);
	}
}



/********************************************************************************\
	BmListViewController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmListViewController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmListViewController::BmListViewController( minimax minmax, BRect rect,
								 const char* Name, list_view_type Type, bool hierarchical, 
								 bool showLabelView, bool showCaption, bool showBusyView)
	:	inherited( minmax, rect, Name, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, 
					  Type, hierarchical, showLabelView)
	,	inheritedController( Name)
	,	mInitialStateInfo( NULL)
	,	mShowCaption( showCaption)
	,	mShowBusyView( showBusyView)
	,	mUseStateCache( true)
	,	mCurrHighlightItem( NULL)
	,	mUpdatePulseRunner( NULL)
	,	mCachedMessages( 20)
	,	mSittingOnExpander( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmListViewController()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmListViewController::~BmListViewController() {
	delete mUpdatePulseRunner;
	delete mInitialStateInfo;
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmListViewController::CreateContainer( bool horizontal, bool vertical, 
												  							bool scroll_view_corner, 
												  							border_style border, 
																		   uint32 ResizingMode, 
																		   uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView);
}

/*------------------------------------------------------------------------------*\
	AttachedToWindow()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AttachedToWindow() {
	inherited::AttachedToWindow();
	if (SetTarget( this) != B_OK)
		BM_THROW_RUNTIME( "ListViewController: Could not set invocation-target.");
	SetInvocationMessage( new BMessage( B_CONTROL_INVOKED));
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	override that automatically grabs the keyboard-focus after a click 
			inside the listview
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseDown(BPoint point) { 
	inherited::MouseDown( point); 
	BView::MakeFocus( true);
	// [zooey]: use the following line to draw blue frame indicating key-focus
	// MakeFocus( true);
}

/*------------------------------------------------------------------------------*\
	MouseUp( point)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseUp(BPoint point) { 
	inherited::MouseUp( point); 
}

/*------------------------------------------------------------------------------*\
	MouseMoved()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MouseMoved( BPoint point, uint32 transit, const BMessage *msg) {
	inherited::MouseMoved( point, transit, msg);
	if (msg && AcceptsDropOf( msg)) {
		if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
			int32 index = IndexOf( point);
			if (IndexOf( mCurrHighlightItem) != index) {
				if (mCurrHighlightItem) {
					// remove old highlight:
					mCurrHighlightItem->Highlight( false);
					InvalidateItem( IndexOf( mCurrHighlightItem));
				}
				if (index >= 0) {
					// highlight current destination:
					mCurrHighlightItem = dynamic_cast<BmListViewItem*>( ItemAt( index));
					mCurrHighlightItem->Highlight( true);
					InvalidateItem( index);
				} else
					mCurrHighlightItem = NULL;
			}
			if (Hierarchical() && mCurrHighlightItem 
			&& mCurrHighlightItem->ExpanderRectContains( point)) {
				if (!mSittingOnExpander) {
					if (!mCurrHighlightItem->IsExpanded()) {
						// expand superitem if mouse is over expander (so that user can 
						// navigate through the subitems that were previously hidden):
						Expand( mCurrHighlightItem);
					} else {
						// collapse superitem if mouse is over expander (so that user can 
						// hide subitems that were previously shown):
						Collapse( mCurrHighlightItem);
					}
					mSittingOnExpander = true;
				}
			} else 
				mSittingOnExpander = false;
		} else if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW) {
			if (mCurrHighlightItem) {
				mCurrHighlightItem->Highlight( false);
				InvalidateItem( IndexOf( mCurrHighlightItem));
				mCurrHighlightItem = NULL;
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::HandleDrop( const BMessage* msg) {
	// remove the drag-highlight, if neccessary:
	if (mCurrHighlightItem) {
		mCurrHighlightItem->Highlight( false);
		InvalidateItem( IndexOf( mCurrHighlightItem));
		mCurrHighlightItem = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MessageReceived( BMessage* msg) {
	try {
		BmDataModel* dataModel = DataModel();
		switch( msg->what) {
			case BM_LISTMODEL_ADD: {
				if (!IsMsgFromCurrentModel( msg)) break;
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				AddModelItem( item);
				item->RemoveRef();			// the msg is no longer referencing the item
				UpdateCaption();
				break;
			}
			case BM_LISTMODEL_REMOVE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				RemoveModelItem( item);
				item->RemoveRef();			// the msg is no longer referencing the item
				UpdateCaption();
				break;
			}
			case BM_LISTMODEL_UPDATE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (!item) break;
				BmUpdFlags flags = UPD_ALL;
				msg->FindInt32( BmListModel::MSG_UPD_FLAGS, (int32*)&flags);
				UpdateModelItem( item, flags);
				item->RemoveRef();			// the msg is no longer referencing the item
				break;
			}
			case BM_LISTVIEW_SHOW_COLUMN:
			case BM_LISTVIEW_HIDE_COLUMN: {
				ShowOrHideColumn( msg);
				break;
			}
			case BM_JOB_UPDATE_STATE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				UpdateModelState( msg);
				break;
			}
			case BM_JOB_DONE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			case B_CONTROL_INVOKED: {
				// an item has been double-clicked:
				ItemInvoked( FindMsgInt32( msg, "index"));
				break;
			}
			default:
				if (msg->WasDropped())
					HandleDrop( msg);
				else
					inherited::MessageReceived( msg);
		}
		if (MsgNeedsAck( msg) && dataModel)
			dataModel->ControllerAck( this);
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString(ControllerName()) << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	FindViewItemFor( modelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::FindViewItemFor( BmListModelItem* modelItem) {
	int32 count = FullListCountItems();
	for( int32 i=0; i<count; ++i) {
		BmListViewItem* viewItem = static_cast<BmListViewItem*>( FullListItemAt(i));
		if (viewItem->ModelItem() == modelItem)
			return viewItem;
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::AddAllModelItems() {
	BM_LOG2( BM_LogModelController, BString(ControllerName())<<": adding items to listview");
	BmAutolock lock( DataModel()->ModelLocker());
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( BString() << ControllerName() << ":AddAllModelItems(): Unable to lock model");
	MakeEmpty();
	BmListModel *model = DataModel();
	BList* tempList = NULL;
	if (!Hierarchical())
		tempList = new BList( model->size());
	SetDisconnectScrollView( true);
	SetInsertAtSortedPos( false);
	BmModelItemMap::const_iterator iter;
	int32 count=1;
	for( iter = model->begin(); iter != model->end(); ++iter, ++count) {
		BmListModelItem* modelItem = iter->second.Get();
		BmListViewItem* viewItem;
		if (Hierarchical()) {
			// add item and its subitems to the listview:
			doAddModelItem( NULL, modelItem);
		} else {
			viewItem = CreateListViewItem( modelItem);
			if (viewItem)
				tempList->AddItem( viewItem);
		}
		if (count%100==0) {
			ScrollView()->PulseBusyView();
			BString caption = BString()<<count<<" "<<ItemNameForCaption()<<(count>1?"s":"");
			UpdateCaption( caption.String());
		}
	}
	if (!Hierarchical()) {
		// add complete item-list for efficiency:
		AddList( tempList);
		delete tempList;
	}
	SortItems();
	SetInsertAtSortedPos( true);
	SetDisconnectScrollView( false);
	UpdateColumnSizesDataRectSizeScrollBars( true);
	UpdateCaption();
	BM_LOG3( BM_LogModelController, BString(ControllerName())<<": finished with adding items to listview");
}

/*------------------------------------------------------------------------------*\
	AddModelItem( item)
		-	Hook function that is called whenever a new item has been added to the 
			listmodel
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::AddModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, BString(ControllerName())<<": adding one item to listview");
	BmListViewItem* newItem;
	if (!Hierarchical()) {
		newItem = doAddModelItem( NULL, item);
	} else {
		BmListViewItem* parentItem = FindViewItemFor( item->Parent());
		newItem = doAddModelItem( parentItem, item);
	}
	BMessage msg( BM_NTFY_LISTCONTROLLER_MODIFIED);
	SendNotices( BM_NTFY_LISTCONTROLLER_MODIFIED, &msg);
	return newItem;
}

/*------------------------------------------------------------------------------*\
	doAddModelItem( BmListViewItem* parent, BmListModelItem)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmListViewController::doAddModelItem( BmListViewItem* parent, BmListModelItem* item) {
	auto_ptr<BMessage> archive( Hierarchical() ? GetArchiveForItemKey( item->Key()) : NULL);
	BmListViewItem* newItem = CreateListViewItem( item, archive.get());
	if (newItem) {
		if (Hierarchical() && !item->empty())
			newItem->SetSuperItem( true);
		if (parent)
			AddUnder( newItem, parent);
		else
			AddItem( newItem);
	}
	
	// add all sub-items of current item to the view as well:
	BmModelItemMap::const_iterator iter;
	for( iter = item->begin(); iter != item->end(); ++iter) {
		BmListModelItem* subItem = iter->second.Get();
		doAddModelItem( newItem, subItem);
	}
	return newItem;
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	Hook function that is called whenever an item has been deleted from the
			listmodel
		-	this implementation informs the model that we have noticed the removal 
			of the item. This is neccessary since the model is waiting for all 
			controllers to signal that they have understood that the deleted item
			is no longer valid. Otherwise, a controller might still access
			an item that has already been deleted by the model.
\*------------------------------------------------------------------------------*/
void BmListViewController::RemoveModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, BString(ControllerName())<<": removing one item from listview");
	if (item) {
		BmListViewItem* viewItem = FindViewItemFor( item);
		if (viewItem) {
			RemoveItem( viewItem);
			delete viewItem;
		}
		UpdateCaption();
		BMessage msg( BM_NTFY_LISTCONTROLLER_MODIFIED);
		SendNotices( BM_NTFY_LISTCONTROLLER_MODIFIED, &msg);
	}
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( msg)
		-	Hook function that is called whenever an item needs to be updated 
		-	default implementation does nothing
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateModelItem( BmListModelItem* item, BmUpdFlags updFlags) {
	BmListViewItem* viewItem = FindViewItemFor( item);
	if (viewItem) {
		viewItem->UpdateView( updFlags);
		int32 idx = IndexOf(viewItem);
		if (idx >= 0)
			InvalidateItem( idx);
	} else
		BM_LOG2( BM_LogModelController, BString(ControllerName())<<": requested to update an unknown item <"<<item->Key()<<">");
}

/*------------------------------------------------------------------------------*\
	UpdateModelState( msg)
		-	Hook function that is called whenever the model as a whole (the list)
			has changed state
		-	default implementation does nothing
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateModelState( BMessage* msg) {
}

/*------------------------------------------------------------------------------*\
	UpdateCaption()
		-
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateCaption( const char* text) {
	if (mShowCaption && fScrollView) {
		if (text) {
			ScrollView()->SetCaptionText( text);
		} else {
			int32 numItems = FullListCountItems();
			BString caption;
			if (!numItems)
				caption = BString("no ")<<ItemNameForCaption()<<"s";
			else
				caption = BString("")<<numItems<<" "<<ItemNameForCaption()<<(numItems>1 ? "s" : "");
			ScrollView()->SetCaptionText( caption.String());
		}
		Window()->UpdateIfNeeded();
	}
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ItemInvoked( int32 index) {
}

/*------------------------------------------------------------------------------*\
	ShowOrHideColumn( msg)
		-	Hook function that is called whenever the user asked to show or hide a
			specific listview-column
\*------------------------------------------------------------------------------*/
void BmListViewController::ShowOrHideColumn( BMessage* msg) {
	if (msg->what == BM_LISTVIEW_SHOW_COLUMN) {
		ShowColumn( msg->FindInt32( MSG_COLUMN_NO));
	} else {
		HideColumn( msg->FindInt32( MSG_COLUMN_NO));
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ShowLabelViewMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "LabelViewMenu", false, false);

	int32 numCols = fColumnList.CountItems();
	int32 numVisibleCols = fColumnDisplayList.CountItems();
	for( int32 i=0; i<numCols; ++i) {
		CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( i);
		if (!column) continue;
		uint32 flags = column->Flags();
		if (flags & (CLV_EXPANDER | CLV_LOCK_WITH_RIGHT | CLV_MERGE_WITH_RIGHT | CLV_NOT_MOVABLE)) continue;
		bool shown = fColumnDisplayList.HasItem( column);
		BMessage* msg = new BMessage( shown ? BM_LISTVIEW_HIDE_COLUMN : BM_LISTVIEW_SHOW_COLUMN);
		msg->AddInt32( MSG_COLUMN_NO, i);
		BString label = column->GetLabelName();
		BMenuItem* item = new BMenuItem( label.String(), msg);
		item->SetMarked( shown);
		if (shown && numVisibleCols == 1) {
			// don't allow user to remove last visible column:
			item->SetEnabled( false);
		}
		item->SetTarget( this);
		theMenu->AddItem( item);
	}

	if (theMenu->CountItems() > 0) {
	   ColumnLabelView()->ConvertToScreen(&point);
		BRect openRect;
		openRect.top = point.y - 5;
		openRect.bottom = point.y + 5;
		openRect.left = point.x - 5;
		openRect.right = point.x + 5;
  		theMenu->Go( point, true, false, openRect);
	}
  	delete theMenu;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ExpansionChanged( CLVListItem* _item, bool expanded) {
	BmListViewItem* item = dynamic_cast< BmListViewItem*>( _item);
	UpdateItem( item, UPD_EXPANDER);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::UpdateItem( BmListViewItem* item, BmUpdFlags flags) {
	if (!item)
		return;
	LockLooper();
	item->UpdateView( flags);
	InvalidateItem( IndexOf( item));
	UnlockLooper();
}

/*------------------------------------------------------------------------------*\
	AttachModel()
		-	reads appropriate state-info after attaching to data-model
\*------------------------------------------------------------------------------*/
void BmListViewController::AttachModel( BmDataModel* model) {
	inheritedController::AttachModel( model);
	ReadStateInfo();
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	writes state-info for current data-model before detaching
		-	clears listview after detach
\*------------------------------------------------------------------------------*/
void BmListViewController::DetachModel() {
	WriteStateInfo();
	inheritedController::DetachModel();
	ScrollView()->UnsetBusy();
	MakeEmpty();
	UpdateCaption();
}

/*------------------------------------------------------------------------------*\
	MakeEmpty()
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::MakeEmpty() {
	BList tempList;
	int32 count = FullListCountItems();
	for( int i=0; i<count; ++i)
		tempList.AddItem( FullListItemAt( i));
	inherited::MakeEmpty();					// clear display
	while( !tempList.IsEmpty()) {
		BmListViewItem* subItem = static_cast<BmListViewItem*>(tempList.RemoveItem( (int32)0));
		delete subItem;
	}
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::StartJob( BmJobModel* model, bool startInNewThread) {
	AttachModel( model);
	ScrollView()->SetBusy();
	UpdateCaption( "tracking...");
	inheritedController::StartJob( model, startInNewThread);
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	Hook function that is called whenever the jobmodel associated to this 
			controller indicates that it is done (meaning: the list has been fetched
			and is now ready to be displayed).
\*------------------------------------------------------------------------------*/
void BmListViewController::JobIsDone( bool completed) {
	if (completed) {
		AddAllModelItems();
	} else {
		UpdateCaption( "");
	}
	ScrollView()->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	WriteStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::WriteStateInfo() {
	status_t err;
	BString stateInfoFilename;
	BFile stateInfoFile;
	BMessage archive;
	
	if (!DataModel() || !mUseStateCache)
		return;

	try {
		stateInfoFilename = StateInfoBasename() << "_" << ModelName();
		this->Archive( &archive, Hierarchical()) == B_OK
													|| BM_THROW_RUNTIME( BString("Unable to archive State-Info for ")<<Name());
		(err = stateInfoFile.SetTo( TheResources->StateInfoFolder(), stateInfoFilename.String(), 
											 B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create state-info file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &stateInfoFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store state-info into file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	Archive()
		-	
\*------------------------------------------------------------------------------*/
status_t BmListViewController::Archive(BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (deep && ret == B_OK) {
		int32 numItems = FullListCountItems();
		for( int32 i=0; i<numItems && ret==B_OK; ++i) {
			BmListViewItem* item = static_cast< BmListViewItem*>( FullListItemAt( i));
			ret = item->Archive( archive, deep);
		}
	}
	if (ret != B_OK) {
		ShowAlert( BString("Could not archive State-Info for ") << ModelName() << "\n\tError: "<< strerror( ret));
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetArchiveForItemKey( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmListViewController::GetArchiveForItemKey( BString key, BMessage* msg=NULL) {
	if (mInitialStateInfo) {
		status_t ret = B_OK;
		for( int i=0; ret==B_OK; ++i) {
			const char* name;
			ret = mInitialStateInfo->FindString( BmListViewItem::MSG_CHILDNAMES, i, &name);
			if (key == name) {
				return FindMsgMsg( mInitialStateInfo, BmListViewItem::MSG_CHILDREN, msg, i);
			}
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	ReadStateInfo( )
		-	
\*------------------------------------------------------------------------------*/
void BmListViewController::ReadStateInfo() {
	status_t err;
	BString stateInfoFilename;
	BFile stateInfoFile;

	// try to open state-info-file...
	stateInfoFilename = StateInfoBasename() << "_" << ModelName();
	if (mUseStateCache 
	&& (err = stateInfoFile.SetTo( TheResources->StateInfoFolder(), 
											 stateInfoFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, file found, we fetch our state(s) from it:
		try {
			delete mInitialStateInfo;
			mInitialStateInfo = new BMessage;
			(err = mInitialStateInfo->Unflatten( &stateInfoFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch state-info from file\n\t<") << stateInfoFilename << ">\n\n Result: " << strerror(err));
			inherited::Unarchive( mInitialStateInfo);
		} catch (exception &e) {
			delete mInitialStateInfo;
			mInitialStateInfo = NULL;
			BM_SHOWERR( e.what());
		}
	} else 
		inherited::Unarchive( DefaultLayout());
}



/********************************************************************************\
	BmCLVContainerView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCLVContainerView::BmCLVContainerView( minimax minmax, ColumnListView* target, 
													 uint32 resizingMode, uint32 flags, 
													 bool horizontal, bool vertical,
													 bool scroll_view_corner, border_style border, 
													 bool showCaption, bool showBusyView,
													 float captionWidth)
	:	inherited( minmax, target, resizingMode, flags, horizontal, vertical,
					  scroll_view_corner, border)
	,	mCaption( NULL)
	,	mCaptionWidth( captionWidth)
	,	mBusyView( NULL)
{
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR));
	BRect frame;
	BPoint LT;
	BScrollBar* hScroller = horizontal ? ScrollBar( B_HORIZONTAL) : NULL;
	if (hScroller) {
		frame = hScroller->Frame();
		if (showCaption && !mCaptionWidth) {
//			mCaptionWidth = frame.Width();
			hScroller->Hide();
		}
	} else {
		frame = Bounds();
		frame.left += 2;
		frame.right -= 2;
		frame.top = frame.bottom - 1 - B_H_SCROLL_BAR_HEIGHT;
	}
	if (showBusyView) {
		LT = frame.LeftTop();
		float bvSize = frame.Height();
		if (hScroller) {
			// a horizontal scrollbar exists, we shrink it to make room for the busyview:
			hScroller->ResizeBy( -bvSize, 0.0);
			hScroller->MoveBy( bvSize, 0.0);
		}
		mBusyView = new BmBusyView( BRect( LT.x, LT.y, LT.x+bvSize, LT.y+bvSize));
		AddChild( mBusyView);
		frame.left += bvSize;
	}
	if (showCaption) {
		LT = frame.LeftTop();
		if (hScroller) {
			// a horizontal scrollbar exists, we shrink it to make room for the caption:
			hScroller->ResizeBy( -mCaptionWidth, 0.0);
			hScroller->MoveBy( mCaptionWidth, 0.0);
		} else {
			// no horizontal scrollbar, so the caption occupies all the remaining space:
			mCaptionWidth = frame.Width();
		}
		mCaption = new BmCaption( BRect( LT.x, LT.y, LT.x+mCaptionWidth, LT.y+frame.Height()), "");
		AddChild( mCaption);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCLVContainerView::~BmCLVContainerView() {
}
	
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::SetCaptionText( const char* text) {
	if (mCaption)
		mCaption->SetText( text);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::SetBusy() {
	if (mBusyView) mBusyView->SetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::UnsetBusy() {
	if (mBusyView) mBusyView->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCLVContainerView::PulseBusyView() {
	if (mBusyView) mBusyView->Pulse();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmCLVContainerView::layout( BRect rect) {
	BRect r = inherited::layout( rect);
	float fullCaptionWidth = r.Width();
	fullCaptionWidth -= 2.0;
	if (mBusyView) {
		BRect bvFrame = mBusyView->Frame();
		mBusyView->MoveTo( bvFrame.left, rect.bottom-1-bvFrame.Height());
		fullCaptionWidth -= bvFrame.Width();
	}
	if (mCaption) {
		BRect cpFrame = mCaption->Frame();
		mCaption->MoveTo( cpFrame.left, rect.bottom-1-cpFrame.Height());
		BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
		if (!mCaptionWidth && (!hScroller || hScroller->IsHidden())) {
			if (ScrollBar( B_VERTICAL))
				fullCaptionWidth -= B_V_SCROLL_BAR_WIDTH + 1;
			mCaption->ResizeTo( fullCaptionWidth, cpFrame.Height());
			mCaption->Invalidate();
		}
	}
	return r;
}
