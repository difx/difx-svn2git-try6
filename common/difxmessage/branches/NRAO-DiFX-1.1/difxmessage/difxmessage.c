#include <stdio.h>
#include "../difxmessage.h"

/* Note! Keep this in sync with enum Mk5Status in difxmessage.h */
const char Mk5StateStrings[][24] = 
{
	"Opening",
	"Open",
	"Close",
	"GetDirectory",
	"GotDirectory",
	"Play",
	"Idle",
	"Error",
	"Busy",
	"Initializing",
	"Resetting",
	"Rebooting",
	"PowerOff",
	"NoData",
	"NoMoreData",
	"PlayInvalid"
};

/* Note! Keep this in sync with enum DifxStatus in difxmessage.h */
const char DifxStateStrings[][24] =
{
	"Spawning",
	"Starting",
	"Running",
	"Ending",
	"Done",
	"Aborting",
	"Terminating",
	"Terminated",
	"MpiDone"
};

/* Note! Keep this in sync with enum DifxMessageType in difxmessage.h */
const char DifxMessageTypeStrings[][24] =
{
	"Unknown",
	"DifxLoadMessage",
	"DifxErrorMessage",
	"Mark5StatusMessage",
	"DifxStatusMessage",
	"DifxInfoMessage",
	"DifxDatastreamMessage",
	"DifxCommand"
};
