// defines for ipc_dsy
// IPC_OrgDest
#define IPC_DEST_MODEM		0
#define IPC_DEST_OMAP		1
#define IPC_DEST_DSY		2
#define IPC_DEST_ACCESS		3

// t_IPC_Group
#define IPC_GROUP_RAGBAG	2
#define IPC_GROUP_SIM		3
#define IPC_GROUP_BAT		4
#define IPC_GROUP_CCMON		5
#define IPC_GROUP_DEVMENU	6
#define	IPC_GROUP_RSSI		7
#define	IPC_GROUP_BTBD		8
#define IPC_GROUP_POWERUP	9
#define IPC_GROUP_POWEROFF	10

//  cmd  for IPC_GROUP_POWERUP
#define PWRUP_PING_REQ			0
#define PWRUP_PING_RES			1
#define PWRUP_RTC_CHECK			2
#define PWRUP_RTC_TRANSFER		3
#define PWRUP_STARTUP_REASON		4
#define PWRUP_SELFTEST			5
#define PWRUP_HIDDENRESET		6
#define PWRUP_POWERON			7
#define PWRUP_GETSWSTARTUPREASON 	8
#define PWRUP_RTCSETUP			9
#define PWRUP_SYNCREQ			10
#define PWRUP_INDICATIONOBSERVEROK	11
#define PWRUP_GETOFFLINEMODESTATE	12

//  cmd  for IPC_GROUP_POWEROFF
#define PWROFF_PREWARNING		0
#define PWROFF_SWITCHOFF		1
#define PWROFF_HIDDENRESET		2
#define PWROFF_SEXITSHUTDOWN		3
#define PWROFF_SETSWSTARTUPREASON	4
#define PWROFF_SETOFFLINEMODESTATE	5
#define PWROFF_TRANSITIONTONORMAL	6
#define PWROFF_TRANSITIONCOPROMODE	7
#define PWROFF_OMAPPARTIALMODE		8

// cmd for IPC_GROUP_RAGBAG
#define RAG_AudioLinkOpenReqRes		0
#define RAG_AudioLinkCloseReqRes	1
#define RAG_UsbSessionEnd		2
#define RAG_RestoreFactorySettings	3
#define RAG_SetDosAlarm			4
#define RAG_SetLightSensorSettingsRes	5
#define RAG_EmailMessageRes		6
#define RAG_FaxMessageRes		7
#define RAG_VoiceMailStatusRes		8
#define RAG_TempSensorValueRes		9
#define RAG_SetState			10
#define RAG_CallsForwardingStatusRes	11
#define RAG_GetDisplayConstrast		12
#define RAG_SetDisplayConstrast		13
#define RAG_ModeTransition		14
#define RAG_PanicTransfer		15
#define RAG_AudioLinkOpenResReq		16
#define RAG_AudioLinkCloseResReq	17
#define RAG_EgoldWakeUp			18
#define RAG_BacklightTempHandle		19
#define RAG_CarKitIgnitionRes		20
// egold fw15: there are messages to 0x16 = 21 and 22

// cmd for IPC_GROUP_SIM
#define SIM_StateRes			0
#define SIM_GetLanguage			1
#define SIM_SimChanged			2
#define SIM_CurrentSimOwnedStatusRes	3
#define SIM_ActivePendingLockType	4
#define SIM_LockStatusRes		5
#define SIM_GetDomesticLanguage		6
#define SIM_SetDomesticLanguage		7
#define SIM_GetSimlockStatus		8

// cmd for IPC_GROUP_BAT
#define BAT_ChargingStateRes		0
#define BAT_StatusRes			1
#define BAT_BarsRes			2
#define BAT_LowWarningRes		3

// cmd for IPC_GROUP_CCMON - is string number

// cmd for IPC_GROUP_DEVMENU
#define DEVMENU_Enabled			0
#define DEVMENU_MonitorString	1 // to 27
#define DEVMENU_ResetExits		28

// cmd for IPC_GROUP_RSSI
#define RSSI_NetworkBars		0

#define BTBD_DataFromEElite		0
