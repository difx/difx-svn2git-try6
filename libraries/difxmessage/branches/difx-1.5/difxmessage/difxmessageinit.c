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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../difxmessage.h"
#include "difxmessageinternal.h"

#define MAX_GROUP_SIZE			16
#define DIFX_MESSAGE_FORMAT_LENGTH	256

char difxMessageGroup[MAX_GROUP_SIZE] = "";
int difxMessagePort = -1; 
char difxMessageIdentifier[DIFX_MESSAGE_IDENTIFIER_LENGTH] = "";
char difxMessageHostname[DIFX_MESSAGE_PARAM_LENGTH] = "";
int difxMessageMpiProcessId = -1;
char difxMessageXMLFormat[DIFX_MESSAGE_FORMAT_LENGTH] = "";
int difxMessageSequenceNumber = 0;
char difxBinarySTAGroup[MAX_GROUP_SIZE] = "";
int difxBinarySTAPort = -1;
char difxBinaryLTAGroup[MAX_GROUP_SIZE] = "";
int difxBinaryLTAPort = -1;
int difxMessageInUse = 0;

const char *getDifxMessageIdentifier()
{
	return difxMessageIdentifier;
}

int difxMessageInit(int mpiId, const char *identifier)
{
	const char *envstr;
	int v;

	difxMessageSequenceNumber = 0;
	difxMessageInUse = 1;
	
	snprintf(difxMessageIdentifier, DIFX_MESSAGE_IDENTIFIER_LENGTH, 
		"%s", identifier);

	difxMessageMpiProcessId = mpiId;

	gethostname(difxMessageHostname, DIFX_MESSAGE_PARAM_LENGTH);
	difxMessageHostname[DIFX_MESSAGE_PARAM_LENGTH-1] = 0;

	envstr = getenv("DIFX_MESSAGE_GROUP");
	if(envstr != 0)
	{
		v = snprintf(difxMessageGroup, MAX_GROUP_SIZE, "%s", envstr);
		if(v >= MAX_GROUP_SIZE)
		{
			fprintf(stderr, "Error: difxMessageInit: env var DIFX_MESSAGE_GROUP too long\n");
			return -1;
		}
	}
	else
	{
		difxMessageInUse = 0;
	}


	envstr = getenv("DIFX_MESSAGE_PORT");
	if(envstr != 0)
	{
		difxMessagePort = atoi(envstr);
	}
	else
	{
		difxMessageInUse = 0;
	}

	v = snprintf(difxMessageXMLFormat, DIFX_MESSAGE_FORMAT_LENGTH,
		
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
		"<difxMessage>"
		  "<header>"
		    "<from>%s</from>"
		    "<mpiProcessId>%d</mpiProcessId>"
		    "<identifier>%s</identifier>"
		    "<type>%%s</type>"
		  "</header>"
		  "<body>"
		    "<seqNumber>%%d</seqNumber>"
		    "%%s"
		  "</body>"
		"</difxMessage>",
		
		difxMessageHostname, 
		difxMessageMpiProcessId,
		difxMessageIdentifier);

	if(v >= DIFX_MESSAGE_FORMAT_LENGTH)
	{
		fprintf(stderr, "Error: difxMessageInit: format string overflow\n");
		return -1;
	}

	return 0;
}
int difxMessageInitBinary()
{
	const char *envstr;
	int v;

	envstr = getenv("DIFX_BINARY_GROUP");
	if(envstr != 0)
	{
		v = snprintf(difxBinarySTAGroup, MAX_GROUP_SIZE, "%s", envstr);
		if(v >= MAX_GROUP_SIZE)
		{
			fprintf(stderr, "Error: difxMessageInitBinary: env var DIFX_BINARY_GROUP too long\n");
			return -1;
		}
	}

	envstr = getenv("DIFX_BINARY_PORT");
	if(envstr != 0)
	{
		difxBinarySTAPort = atoi(envstr);
	}

	return 0;
}

void difxMessagePrint()
{
	printf("difxMessage: %s\n", difxMessageIdentifier);
	printf("  group/port = %s/%d\n", difxMessageGroup, difxMessagePort);
	printf("  hostname = %s\n", difxMessageHostname);
	printf("  identifier = %s / %d\n", difxMessageIdentifier, 
		difxMessageMpiProcessId);
}

void difxMessageGetMulticastGroupPort(char *group, int *port)
{
	strcpy(group, difxMessageGroup);
	*port = difxMessagePort;
}
