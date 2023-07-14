
// DlgProxy.h: header file
//

#pragma once

class CLaserBurningConfigDlg;


// CLaserBurningConfigDlgAutoProxy command target

class CLaserBurningConfigDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CLaserBurningConfigDlgAutoProxy)

	CLaserBurningConfigDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CLaserBurningConfigDlg* m_pDialog;

// Operations
public:

// Overrides
	public:
	virtual void OnFinalRelease();

// Implementation
protected:
	virtual ~CLaserBurningConfigDlgAutoProxy();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CLaserBurningConfigDlgAutoProxy)

	// Generated OLE dispatch map functions

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

