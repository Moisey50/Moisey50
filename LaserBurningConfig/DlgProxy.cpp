
// DlgProxy.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "LaserBurningConfig.h"
#include "DlgProxy.h"
#include "LaserBurningConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLaserBurningConfigDlgAutoProxy

IMPLEMENT_DYNCREATE(CLaserBurningConfigDlgAutoProxy, CCmdTarget)

CLaserBurningConfigDlgAutoProxy::CLaserBurningConfigDlgAutoProxy()
{
	EnableAutomation();

	// To keep the application running as long as an automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(CLaserBurningConfigDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(CLaserBurningConfigDlg)))
		{
			m_pDialog = reinterpret_cast<CLaserBurningConfigDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

CLaserBurningConfigDlgAutoProxy::~CLaserBurningConfigDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != nullptr)
		m_pDialog->m_pAutoProxy = nullptr;
	AfxOleUnlockApp();
}

void CLaserBurningConfigDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CLaserBurningConfigDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CLaserBurningConfigDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// Note: we add support for IID_ILaserBurningConfig to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {672393cc-7137-44e0-9d03-c3505134ae19}
static const IID IID_ILaserBurningConfig =
{0x672393cc,0x7137,0x44e0,{0x9d,0x03,0xc3,0x50,0x51,0x34,0xae,0x19}};

BEGIN_INTERFACE_MAP(CLaserBurningConfigDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CLaserBurningConfigDlgAutoProxy, IID_ILaserBurningConfig, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in pch.h of this project
// {68f04c6c-e999-4aa6-87eb-f5975a88027e}
IMPLEMENT_OLECREATE2(CLaserBurningConfigDlgAutoProxy, "LaserBurningConfig.Application", 0x68f04c6c,0xe999,0x4aa6,0x87,0xeb,0xf5,0x97,0x5a,0x88,0x02,0x7e)


// CLaserBurningConfigDlgAutoProxy message handlers
