//===========================================================================
// SVN properties (DO NOT CHANGE)
//
// $Id$
// $HeadURL$
// $LastChangedRevision$
// $Author$
// $LastChangedDate$
//
//============================================================================
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include "../difxmessage.h"
#include "difxmessageinternal.h"

const int MIN_SEND_GAP=20;

/* Function that replaces illegal XML string characters with the
 * official "entity" replacements.  
 *
 * Returns:
 *   Increse of string size on success, or -1 on error.
 */
int expandEntityRefrences(char *dest, const char *src, int maxLength)
{
	int i, j;

	for(i = j = 0; src[i]; i++)
	{
		if(j >= maxLength-7)
		{
			return -1;
		}

		if(src[i] == '>')
		{
			strcpy(dest+j, "&gt;");
			j += 4;
		}
		else if(src[i] == '<')
		{
			strcpy(dest+j, "&lt;");
			j += 4;
		}
		else if(src[i] == '&')
		{
			strcpy(dest+j, "&amp;");
			j += 5;
		}
		else if(src[i] == '"')
		{
			strcpy(dest+j, "&quot;");
			j += 6;
		}
		else if(src[i] == '\'')
		{
			strcpy(dest+j, "&apos;");
			j += 6;
		}
		else if(src[i] < 32)	/* ascii chars < 32 are not allowed */
		{
			sprintf(dest+j, "[[%3d]]", src[i]);
			j += 7;
		}
		else
		{
			dest[j] = src[i];
			j++;
		}
	}

	dest[j] = 0;

	return j - i;
}

int difxMessageSend(const char *message)
{
	static int first = 1;
	static struct timeval tv0;

	struct timeval tv;
	int dt;

	if(difxMessagePort < 0)
	{
		return -1;
	}

	if(first)
	{
		first = 0;
		gettimeofday(&tv0, 0);
	}
	else
	{
		gettimeofday(&tv, 0);
		dt = 1000000*(tv.tv_sec - tv0.tv_sec) + (tv.tv_usec - tv0.tv_usec);
		if(dt < MIN_SEND_GAP && dt > 0)
		{
			/* The minimum gap prevents two messages from being sent too soon 
			 * after each other -- a condition that apparently can lead to lost
			 * messages 
			 */
			usleep(MIN_SEND_GAP-dt);
		}
		tv0 = tv;
	}

	return MulticastSend(difxMessageGroup, difxMessagePort, message, strlen(message));
}

int difxMessageSendProcessState(const char *state)
{
	char message[DIFX_MESSAGE_LENGTH];

	if(difxMessagePort < 0)
	{
		return -1;
	}

	snprintf(message, DIFX_MESSAGE_LENGTH, "%s %s", difxMessageIdentifier, state);

	return difxMessageSend(message);
}

int difxMessageSendLoad(const DifxMessageLoad *load)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxLoad>"
		  "<cpuLoad>%4.2f</cpuLoad>"
		  "<totalMemory>%d</totalMemory>"
		  "<usedMemory>%d</usedMemory>"
		  "<netRXRate>%d</netRXRate>"
		  "<netTXRate>%d</netTXRate>"
		"</difxLoad>",

		load->cpuLoad,
		load->totalMemory,
		load->usedMemory,
		load->netRXRate,
		load->netTXRate);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendLoad: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat, 
		DifxMessageTypeStrings[DIFX_MESSAGE_LOAD],
		difxMessageSequenceNumber++, body);
	
	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendLoad: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

int difxMessageSendDifxAlert(const char *alertMessage, int severity)
{
	char message[DIFX_MESSAGE_LENGTH];
	char alertMessageExpanded[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	if(difxMessagePort < 0)
	{
		/* send to stderr or stdout if no port is defined */
		if(severity < DIFX_ALERT_LEVEL_WARNING)
		{
			fprintf(stderr, "[%s %d] %7s %s\n", difxMessageHostname, difxMessageMpiProcessId, difxMessageAlertString[severity], alertMessage);
		}
		else
		{
			printf("[%s %d] %7s %s\n", difxMessageHostname, difxMessageMpiProcessId, difxMessageAlertString[severity], alertMessage);
		}
	}
	else
	{
		v = expandEntityRefrences(alertMessageExpanded, alertMessage, DIFX_MESSAGE_LENGTH);

		if(v < 0)
		{
			fprintf(stderr, "difxMessageSendDifxAlert: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);

			return -1;
		}

		v = snprintf(body, DIFX_MESSAGE_LENGTH,
			
			"<difxAlert>"
			  "<alertMessage>%s</alertMessage>"
			  "<severity>%d</severity>"
			"</difxAlert>",

			alertMessageExpanded,
			severity);
		
		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendDifxAlert: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);
			
			return -1;
		}

		v = snprintf(message, DIFX_MESSAGE_LENGTH,
			difxMessageXMLFormat, 
			DifxMessageTypeStrings[DIFX_MESSAGE_ALERT],
			difxMessageSequenceNumber++, body);
		
		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendDifxAlert: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}

		difxMessageSend(message);

		/* Make sure all fatal errors go to the console */
		if(severity == DIFX_ALERT_LEVEL_FATAL)
		{
			fprintf(stderr, "[%s %d] %7s %s\n", difxMessageHostname, difxMessageMpiProcessId, difxMessageAlertString[severity], alertMessage);
		}
	}
	
	return 0;
}

int difxMessageSendCondition(const DifxMessageCondition *cond)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char bins[DIFX_MESSAGE_LENGTH];
	int i;
	int b = 0;
	int v;

	bins[0] = 0;

	fprintf(stderr, "Warning: sending deprecated message type `condition'.\n");
	fprintf(stderr, "Please upgrade the sending code to use the `disk stat' type.\n");

	if(difxMessagePort < 0)
	{
		for(i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
		{
			b += snprintf(bins+b, DIFX_MESSAGE_LENGTH-b, " %d", cond->bin[i]);
			
			if(b >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendCondition: message overflow (%d >= %d)\n", b, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}
		printf("%s[%d] = %s %s\n", 
			cond->moduleVSN, 
			cond->moduleSlot,
			cond->serialNumber,
			bins);
	}
	else
	{
		for(i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
		{
			b += snprintf(bins+b, DIFX_MESSAGE_LENGTH-b,
				"<bin%d>%d</bin%d>", i, cond->bin[i], i);
			if(b >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendCondition: message overflow (%d >= %d)\n", b, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}

		v = snprintf(body, DIFX_MESSAGE_LENGTH,
			
			"<difxCondition>"
			  "<serialNumber>%s</serialNumber>"
			  "<modelNumber>%s</modelNumber>"
			  "<size>%d</size>"
			  "<moduleVSN>%s</moduleVSN>"
			  "<moduleSlot>%d</moduleSlot>"
			  "<startMJD>%7.5f</startMJD>"
			  "<stopMJD>%7.5f</stopMJD>"
			  "<type>%s</type>"
			  "%s"
			"</difxCondition>",

			cond->serialNumber,
			cond->modelNumber,
			cond->diskSize,
			cond->moduleVSN, 
			cond->moduleSlot,
			cond->startMJD,
			cond->stopMJD,
			DriveStatsTypeStrings[DRIVE_STATS_TYPE_CONDITION],
			bins);

		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendCondition: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}

		v = snprintf(message, DIFX_MESSAGE_LENGTH,
			difxMessageXMLFormat, 
			DifxMessageTypeStrings[DIFX_MESSAGE_CONDITION],
			difxMessageSequenceNumber++, body);
		
		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendCondition: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}

		difxMessageSend(message);
	}

	return 0;
}

int difxMessageSendDriveStats(const DifxMessageDriveStats *driveStats)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char bins[DIFX_MESSAGE_LENGTH];
	char startByteString[48];
	int i;
	int b = 0;
	int v;

	bins[0] = 0;

	if(difxMessagePort < 0)
	{
		for(i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
		{
			b += snprintf(bins+b, DIFX_MESSAGE_LENGTH-b, " %d", driveStats->bin[i]);
			
			if(b >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendDriveStats: message overflow (%d >= %d)\n", b, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}
		printf("%s[%d] = %s %s\n", 
			driveStats->moduleVSN, 
			driveStats->moduleSlot,
			driveStats->serialNumber,
			bins);
	}
	else
	{
		for(i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
		{
			b += snprintf(bins+b, DIFX_MESSAGE_LENGTH-b,
				"<bin%d>%d</bin%d>", i, driveStats->bin[i], i);
			if(b >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendDriveStats: message overflow (%d >= %d)\n", b, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}

		if(driveStats->startByte != 0LL)
		{
			sprintf(startByteString, "<startByte>%Ld</startByte>", driveStats->startByte);
		}
		else
		{
			startByteString[0] = 0;
		}

		v = snprintf(body, DIFX_MESSAGE_LENGTH,
			
			"<difxDriveStats>"
			  "<serialNumber>%s</serialNumber>"
			  "<modelNumber>%s</modelNumber>"
			  "<size>%d</size>"
			  "<moduleVSN>%s</moduleVSN>"
			  "<moduleSlot>%d</moduleSlot>"
			  "<startMJD>%7.5f</startMJD>"
			  "<stopMJD>%7.5f</stopMJD>"
			  "<type>%s</type>"
			  "%s"
			  "%s"
			"</difxDriveStats>",

			driveStats->serialNumber,
			driveStats->modelNumber,
			driveStats->diskSize,
			driveStats->moduleVSN, 
			driveStats->moduleSlot,
			driveStats->startMJD,
			driveStats->stopMJD,
			startByteString,
			DriveStatsTypeStrings[driveStats->type],
			bins);

		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendCondition: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}

		v = snprintf(message, DIFX_MESSAGE_LENGTH,
			difxMessageXMLFormat, 
			DifxMessageTypeStrings[DIFX_MESSAGE_CONDITION],
			difxMessageSequenceNumber++, body);
		
		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendCondition: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}

		difxMessageSend(message);
	}

	return 0;
}

int difxMessageSendMark5Status(const DifxMessageMk5Status *mk5status)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char scanName[DIFX_MESSAGE_MAX_SCANNAME_LEN];
	char vsnA[10], vsnB[10];
	char bank;
	int i, v;

	if(strlen(mk5status->vsnA) != 8)
	{
		strcpy(vsnA, "none");
	}
	else
	{
		for(i = 0; i < 9; i++)
		{
			vsnA[i] = toupper(mk5status->vsnA[i]);
		}
	}
	if(strlen(mk5status->vsnB) != 8)
	{
		strcpy(vsnB, "none");
	}
	else
	{
		for(i = 0; i < 9; i++)
		{
			vsnB[i] = toupper(mk5status->vsnB[i]);
		}
	}
	if(!isalpha(mk5status->activeBank))
	{
		bank = ' ';
	}
	else
	{
		bank = toupper(mk5status->activeBank);
	}

	v = snprintf(scanName, DIFX_MESSAGE_MAX_SCANNAME_LEN,
		"%s", mk5status->scanName);
	if(v >= DIFX_MESSAGE_MAX_SCANNAME_LEN)
	{
		fprintf(stderr, "difxMessageSendMark5Status: scanName too long (%d >= %d)\n", v, DIFX_MESSAGE_MAX_SCANNAME_LEN);

		return -1;
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
	
		"<mark5Status>"
		  "<bankAVSN>%s</bankAVSN>"
		  "<bankBVSN>%s</bankBVSN>"
		  "<statusWord>0x%08x</statusWord>"
		  "<activeBank>%c</activeBank>"
		  "<state>%s</state>"
		  "<scanNumber>%d</scanNumber>"
		  "<scanName>%s</scanName>"
		  "<position>%lld</position>"
		  "<playRate>%7.3f</playRate>"
		  "<dataMJD>%13.7f</dataMJD>"
		"</mark5Status>",

		vsnA,
		vsnB,
		mk5status->status,
		bank,
		Mk5StateStrings[mk5status->state],
		mk5status->scanNumber,
		scanName,
		mk5status->position,
		mk5status->rate,
		mk5status->dataMJD);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendMark5Status: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat, 
		DifxMessageTypeStrings[DIFX_MESSAGE_MARK5STATUS],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendMark5Status: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendMk5Version(const DifxMessageMk5Version *mk5version)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char dbInfo[DIFX_MESSAGE_LENGTH] = "";
	int v;

	if(mk5version->DB_PCBVersion[0] != 0)
	{
		v = snprintf(dbInfo, DIFX_MESSAGE_LENGTH,

		  "<DaughterBoard>"
		    "<PCBVer>%s</PCBVer>"
		    "<PCBType>%s</PCBType>"
		    "<PCBSubType>%s</PCBSubType>"
		    "<FPGAConfig>%s</FPGAConfig>"
		    "<FPGAConfigVer>%s</FPGAConfigVer>"
		  "</DaughterBoard>",

		mk5version->DB_PCBVersion,
		mk5version->DB_PCBType,
		mk5version->DB_PCBSubType,
		mk5version->DB_FPGAConfig,
		mk5version->DB_FPGAConfigVersion);

		if(v >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendMk5Version: message dbinfo overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

			return -1;
		}
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
	
		"<mark5Version>"
		  "<ApiVer>%s</ApiVer>"
		  "<ApiDate>%s</ApiDate>"
		  "<FirmVer>%s</FirmVer>"
		  "<FirmDate>%s</FirmDate>"
		  "<MonVer>%s</MonVer>"
		  "<XbarVer>%s</XbarVer>"
		  "<AtaVer>%s</AtaVer>"
		  "<UAtaVer>%s</UAtaVer>"
		  "<DriverVer>%s</DriverVer>"
		  "<BoardType>%s</BoardType>"
		  "<SerialNum>%d</SerialNum>"
		  "%s"
		"</mark5Version>",

		mk5version->ApiVersion,
		mk5version->ApiDateCode,
		mk5version->FirmwareVersion,
		mk5version->FirmDateCode,
		mk5version->MonitorVersion,
		mk5version->XbarVersion,
		mk5version->AtaVersion,
		mk5version->UAtaVersion,
		mk5version->DriverVersion,
		mk5version->BoardType,
		mk5version->SerialNum,
		dbInfo);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendMk5Version: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_MARK5STATUS],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendMk5Version: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxStatus(enum DifxState state, const char *stateMessage, double visMJD, int numdatastreams, float *weight)
{
	char message[DIFX_MESSAGE_LENGTH];
	char weightstr[DIFX_MESSAGE_LENGTH];
	char stateMessageExpanded[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int i, n, v;
	
	v = expandEntityRefrences(stateMessageExpanded, stateMessage, DIFX_MESSAGE_LENGTH);

	if(v < 0)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);

		return -1;
	}

	weightstr[0] = 0;
	n = 0;

	for(i = 0; i < numdatastreams; i++)
	{
		if(weight[i] >= 0)
		{
			n += snprintf(weightstr+n, DIFX_MESSAGE_LENGTH-n,
				"<weight ant=\"%d\" wt=\"%5.3f\"/>", 
				i, weight[i]);
			if(n >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendDifxStatus: message weightstr overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxStatus>"
		  "<state>%s</state>"
		  "<message>%s</message>"
		  "<visibilityMJD>%9.7f</visibilityMJD>"
		  "%s"
		"</difxStatus>",

		DifxStateStrings[state],
		stateMessageExpanded,
		visMJD,
		weightstr);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_STATUS],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxStatus2(const char *jobName, enum DifxState state, const char *stateMessage)
{
	char message[DIFX_MESSAGE_LENGTH];
	char stateMessageExpanded[DIFX_MESSAGE_LENGTH];
	int v;

	v = expandEntityRefrences(stateMessageExpanded, stateMessage, DIFX_MESSAGE_LENGTH);

	if(v < 0)
	{
		fprintf(stderr, "difxMessageSendDifxStatus2: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,

		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<difxMessage>"
		  "<header>"
		    "<from>%s</from>"
		    "<mpiProcessId>-1</mpiProcessId>"
		    "<identifier>%s</identifier>"
		    "<type>DifxStatusMessage</type>"
		  "</header>"
		  "<body>"
		    "<seqNumber>%d</seqNumber>"
		    "<difxStatus>"
		      "<state>%s</state>"
		      "<message>%s</message>"
		      "<visibilityMJD>0</visibilityMJD>"
		    "</difxStatus>"
		  "</body>"
		"</difxMessage>\n",
		
		difxMessageHostname,
		jobName,
		difxMessageSequenceNumber++,
		DifxStateStrings[state],
		stateMessageExpanded);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxStatus2: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

int difxMessageSendDifxStatus3(enum DifxState state, const char *stateMessage,
	double visMJD, int numdatastreams, float *weight, double mjdStart, double mjdStop)
{
	char message[DIFX_MESSAGE_LENGTH];
	char weightstr[DIFX_MESSAGE_LENGTH];
	char stateMessageExpanded[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int i, n, v;
	
	v = expandEntityRefrences(stateMessageExpanded, stateMessage, DIFX_MESSAGE_LENGTH);

	if(v < 0)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);

		return -1;
	}

	weightstr[0] = 0;
	n = 0;

	for(i = 0; i < numdatastreams; i++)
	{
		if(weight[i] >= 0)
		{
			n += snprintf(weightstr+n, DIFX_MESSAGE_LENGTH-n,
				"<weight ant=\"%d\" wt=\"%5.3f\"/>", 
				i, weight[i]);
			if(n >= DIFX_MESSAGE_LENGTH)
			{
				fprintf(stderr, "difxMessageSendDifxStatus: message weightstr overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

				return -1;
			}
		}
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxStatus>"
		  "<state>%s</state>"
		  "<message>%s</message>"
		  "<visibilityMJD>%9.7f</visibilityMJD>"
		  "<jobstartMJD>%9.7f</jobstartMJD>"
		  "<jobstopMJD>%9.7f</jobstopMJD>"
		  "%s"
		"</difxStatus>",

		DifxStateStrings[state],
		stateMessageExpanded,
		visMJD, mjdStart, mjdStop,
		weightstr);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_STATUS],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxStatus: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxInfo(const char *infoMessage)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char infoMessageExpanded[DIFX_MESSAGE_LENGTH];
	int v;

	v = expandEntityRefrences(infoMessageExpanded, infoMessage, DIFX_MESSAGE_LENGTH);

	if(v < 0)
	{
		fprintf(stderr, "difxMessageSendDifxInfo: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxInfo>"
		  "<message>%s</message>"
		"</difxInfo>",

		infoMessageExpanded);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxInfo: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_INFO],
		difxMessageSequenceNumber++, body);
	
	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxInfo: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

int difxMessageSendDifxCommand(const char *command)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char commandExpanded[DIFX_MESSAGE_LENGTH];
	int v;

	v = expandEntityRefrences(commandExpanded, command, DIFX_MESSAGE_LENGTH);
	
	if(v < 0)
	{
		fprintf(stderr, "difxMessageSendDifxCommand: message body overflow in entity replacement (>= %d)\n", DIFX_MESSAGE_LENGTH);
		
		return -1;
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxCommand>"
		  "<command>%s</command>"
		"</difxCommand>",

		commandExpanded);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxCommand: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);
		
		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_COMMAND],
		difxMessageSequenceNumber++, body);
	
	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxCommand: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

/* mpiDestination: 
	>= 0 implies mpiId, 
	  -1 implies ALL, 
	  -2 implies all Datastrems, 
	  -3 implies all Cores
*/
int difxMessageSendDifxParameter(const char *name, 
	const char *value, int mpiDestination)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxParameter>"
		  "<targetMpiId>%d</targetMpiId>"
		  "<name>%s</name>"
		  "<value>%s</value>"
		"</difxParameter>",

		mpiDestination,
		name,
		value);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);
		
		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_PARAMETER],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

int difxMessageSendDifxParameterTo(const char *name,
        const char *value, const char *to)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxParameter>"
		  "<targetMpiId>%d</targetMpiId>"
		  "<name>%s</name>"
		  "<value>%s</value>"
		"</difxParameter>",

		-10,
		name,
		value);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageToXMLFormat,
		to,
		DifxMessageTypeStrings[DIFX_MESSAGE_PARAMETER],
		difxMessageSequenceNumber++, 
		body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);

}

int difxMessageSendDifxParameter1(const char *name, int index1,
	const char *value, int mpiDestination)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxParameter>"
		  "<targetMpiId>%d</targetMpiId>"
		  "<name>%s</name>"
		  "<index1>%d</index1>"
		  "<value>%s</value>"
		"</difxParameter>",

		mpiDestination,
		name,
		index1,
		value);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter1: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);
		
		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_PARAMETER],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter1: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxParameter2(const char *name, int index1, int index2,
	const char *value, int mpiDestination)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxParameter>"
		  "<targetMpiId>%d</targetMpiId>"
		  "<name>%s</name>"
		  "<index1>%d</index1>"
		  "<index2>%d</index1>"
		  "<value>%s</value>"
		"</difxParameter>",

		mpiDestination,
		name,
		index1,
		index2,
		value);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter2: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_PARAMETER],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameter2: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxParameterGeneral(const char *name, int nIndex, const int *index,
	const char *value, int mpiDestination)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	char indices[DIFX_MESSAGE_LENGTH];
	int i, v;
	int p=0;

	for(i = 0; i < nIndex; i++)
	{
		p += snprintf(indices + p, DIFX_MESSAGE_LENGTH-p,
			"<index%d>%d</index%d>", i+1, index[i], i+1);
		if(p >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendDifxParameterGeneral: message indicies overflow (%d >= %d)\n", p, DIFX_MESSAGE_LENGTH);

			return -1;
		}
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,
		
		"<difxParameter>"
		  "<targetMpiId>%d</targetMpiId>"
		  "<name>%s</name>"
		  "%s"
		  "<value>%s</value>"
		"</difxParameter>",

		mpiDestination,
		name,
		indices,
		value);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameterGeneral: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_PARAMETER],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxParameterGeneral: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}
	
	return difxMessageSend(message);
}

int difxMessageSendDifxTransient(const DifxMessageTransient *transient)
{
	char message[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int v;

	v = snprintf(body, DIFX_MESSAGE_LENGTH,

		"<difxTransient>"
		  "<jobId>%s</jobId>"
		  "<startMJD>%14.8f</startMJD>"
		  "<stopMJD>%14.8f</stopMJD>"
		  "<priority>%f</priority>"
		  "<destDir>%s</destDir>"
		  "<comment>%s</comment>"
		  "<dm>%8.6f</dm>"
		"</difxTransient>",

		transient->jobId,
		transient->startMJD,
		transient->stopMJD,
		transient->priority,
		transient->destDir,
		transient->comment,
		transient->dm);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxTransient: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);
		
		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_TRANSIENT],
		difxMessageSequenceNumber++, body);
	
	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxTransient: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

int difxMessageSendDifxSmart(double mjdData, const char *vsn, int slot, int nValue, const int *ids, const long long *values)
{
	char message[DIFX_MESSAGE_LENGTH];
	char smartstr[DIFX_MESSAGE_LENGTH];
	char body[DIFX_MESSAGE_LENGTH];
	int i, n, v;

	smartstr[0] = 0;
	n = 0;

	for(i = 0; i < nValue; i++)
	{
		n += snprintf(smartstr+n, DIFX_MESSAGE_LENGTH-n,
			"<smart id=\"%d\" value=\"%Ld\"/>",
			ids[i], values[i]);
		if(n >= DIFX_MESSAGE_LENGTH)
		{
			fprintf(stderr, "difxMessageSendDifxSmart: message weightstr overflow (%d >= %d)\n", n, DIFX_MESSAGE_LENGTH);
		
			return -1;
		}
	}

	v = snprintf(body, DIFX_MESSAGE_LENGTH,

		"<difxSmart>"
		  "<mjd>%12.6f</mjd>"
		  "<vsn>%s</vsn>"
		  "<slot>%d</slot>"
		  "%s"
		"</difxSmart>",

		mjdData,
		vsn,
		slot,
		smartstr);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxSmart: message body overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	v = snprintf(message, DIFX_MESSAGE_LENGTH,
		difxMessageXMLFormat,
		DifxMessageTypeStrings[DIFX_MESSAGE_SMART],
		difxMessageSequenceNumber++, body);

	if(v >= DIFX_MESSAGE_LENGTH)
	{
		fprintf(stderr, "difxMessageSendDifxSmart: message overflow (%d >= %d)\n", v, DIFX_MESSAGE_LENGTH);

		return -1;
	}

	return difxMessageSend(message);
}

