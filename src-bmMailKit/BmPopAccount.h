/*
	BmPopAccount.h

		$Id$
*/

#ifndef _BmPopAccount_h
#define _BmPopAccount_h

#include <stdexcept>
#include <vector>

#include <Archivable.h>
#include <List.h>
#include <String.h>
#include <NetAddress.h>

#include "BmDataModel.h"

class BmPopAccountList;
/*------------------------------------------------------------------------------*\
	BmPopAccount 
		-	holds information about one specific POP3-account
		- 	derived from BArchivable, so it can be read from and
			written to a file
\*------------------------------------------------------------------------------*/
class BmPopAccount : public BmListModelItem {
	typedef BmListModelItem inherited;
	// archivable components:
	static const char* const MSG_NAME = 			"bm:name";
	static const char* const MSG_USERNAME = 		"bm:username";
	static const char* const MSG_PASSWORD = 		"bm:password";
	static const char* const MSG_POP_SERVER = 	"bm:popserver";
	static const char* const MSG_SMTP_ACCOUNT = 	"bm:smtpacc";
	static const char* const MSG_REAL_NAME = 		"bm:realname";
	static const char* const MSG_REPLY_TO = 		"bm:replyto";
	static const char* const MSG_SIGNATURE_NAME = "bm:signaturename";
	static const char* const MSG_CHECK_MAIL = 	"bm:checkmail";
	static const char* const MSG_DELETE_MAIL = 	"bm:deletemail";
	static const char* const MSG_PORT_NR = 		"bm:portnr";
	static const char* const MSG_UID = 				"bm:uid";
public:
	BmPopAccount( const char* name, BmPopAccountList* model);
	BmPopAccount( BMessage* archive, BmPopAccountList* model);
	virtual ~BmPopAccount() 				{}
	
	// native methods:
	bool IsUIDDownloaded( BString uid);
	void MarkUIDAsDownloaded( BString uid);
	BString GetFromAddress() const;

	// stuff needed for BArchivable:
	static BArchivable* Instantiate( BMessage* archive);
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const BString &Name() const 			{ return Key(); }
	const BString &Username() const 		{ return mUsername; }
	const BString &Password() const 		{ return mPassword; }
	const BString &POPServer() const		{ return mPOPServer; }
	const BString &SMTPAccount() const	{ return mSMTPAccount; }
	const BString &RealName() const 		{ return mRealName; }
	const BString &ReplyTo() const 		{ return mReplyTo; }
	const BString &SignatureName() const	 { return mSignatureName; }
	bool CheckMail() const 					{ return mCheckMail; }
	bool DeleteMailFromServer() const	{ return mDeleteMailFromServer; }
	int16 PortNr() const 					{ return mPortNr; }

	// setters:
	void Username( const BString &s) 	{ mUsername = s; }
	void Password( const BString &s) 	{ mPassword = s; }
	void POPServer( const BString &s)	{ mPOPServer = s; }
	void SMTPAccount( const BString &s)	{ mSMTPAccount = s; }
	void RealName( const BString &s) 	{ mRealName = s; }
	void ReplyTo( const BString &s) 		{ mReplyTo = s; }
	void SignatureName( const BString &s)	 { mSignatureName = s; }
	void CheckMail( bool b) 				{ mCheckMail = b; }
	void DeleteMailFromServer( bool b)	{ mDeleteMailFromServer = b; }
	void PortNr( int16 i) 					{ mPortNr = i; }

	BNetAddress POPAddress() const;
private:
	BmPopAccount();					// hide default constructor

	//BString mName;					// name is stored in key (base-class)
	BString mUsername;
	BString mPassword;
	BString mPOPServer;
	BString mSMTPAccount;			// name of BmSmtpAccount to use when sending 
											// mail "from" this POP-account
	BString mRealName;
	BString mReplyTo;
	BString mSignatureName;			// name&path of signature file
	bool mCheckMail;					// include this account in global mail-check?
	bool mDeleteMailFromServer;	// delete mails upon receive?
	int16 mPortNr;						// usually 110

	vector<BString> mUIDs;			// list of UIDs seen in this account
};


/*------------------------------------------------------------------------------*\
	BmPopAccountList 
		-	holds list of all Pop-Accounts
		-	includes functionality for checking some/all POP-servers for new mail
\*------------------------------------------------------------------------------*/
class BmPopAccountList : public BmListModel {
	typedef BmListModel inherited;
public:
	// creator-func, c'tors and d'tor:
	static BmPopAccountList* CreateInstance();
	BmPopAccountList();
	~BmPopAccountList();
	
	// native methods:
	void CheckMail();
	void CheckMailFor( BString accName);
	
	// overrides of listmodel base:
	const BString SettingsFileName();
	void InstantiateItems( BMessage* archive);

	static BmRef<BmPopAccountList> theInstance;

private:
};

#define ThePopAccountList BmPopAccountList::theInstance

#endif
