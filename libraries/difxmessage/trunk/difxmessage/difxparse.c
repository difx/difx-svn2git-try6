
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
#include <string.h>
#include <expat.h>
#include "../difxmessage.h"

/* allow space or comma separated list of nodes */
int addNodes(char nodes[][DIFX_MESSAGE_PARAM_LENGTH], int maxNodes, int *n, const char *nodeList)
{
	char *str, *p;
	char name[DIFX_MESSAGE_LENGTH];
	int nnew=0;
	int i, l;

	str = strdup(nodeList);

	for(i = 0; str[i]; i++)
	{
		if(str[i] == ',' || 
		   str[i] == '(' || str[i] == ')' ||
		   str[i] == '{' || str[i] == '}' ||
		   str[i] == '[' || str[i] == ']')
		{
			str[i] = ' ';
		}
	}

	p = str;
	for(;;)
	{
		if(*n >= maxNodes)
		{
			break;
		}
		name[0] = 0;
		sscanf(p, "%s%n", name, &l);
		if(name[0] == 0)
		{
			break;
		}
		p += l;
		strncpy(nodes[*n], name, DIFX_MESSAGE_PARAM_LENGTH-1);
		nodes[*n][DIFX_MESSAGE_PARAM_LENGTH-1] = 0;
		(*n)++;
		nnew++;
	}

	free(str);

	return nnew;
}

static void XMLCALL startElement(void *userData, const char *name, 
	const char **atts)
{
	DifxMessageGeneric *G;
	int nThread = 1;
	int i, j, n;

	G = (DifxMessageGeneric *)userData;

	G->_xml_string[0] = 0;
	
	if(G->type == DIFX_MESSAGE_START)
	{
		DifxMessageStart *S;
		
		S = &G->body.start;
		for(i = 0; atts[i]; i+=2)
		{
			if(strcmp(atts[i], "threads") == 0)
			{
				nThread = atoi(atts[i+1]);
			}
		}
		if(strcmp(name, "manager") == 0)
		{
			for(i = 0; atts[i]; i+=2)
			{
				if(strcmp(atts[i], "node") == 0)
				{
					strncpy(S->headNode, atts[i+1], DIFX_MESSAGE_PARAM_LENGTH-1);
					S->headNode[DIFX_MESSAGE_PARAM_LENGTH-1] = 0;
				}
			}
		}
		else if(strcmp(name, "datastream") == 0)
		{
			for(i = 0; atts[i]; i+=2)
			{
				if(strcmp(atts[i], "nodes") == 0)
				{
					addNodes(S->datastreamNode, DIFX_MESSAGE_MAX_DATASTREAMS, &S->nDatastream, atts[i+1]);
				}
			}
		}
		else if(strcmp(name, "process") == 0)
		{
			for(i = 0; atts[i]; i+=2)
			{
				if(strcmp(atts[i], "nodes") == 0)
				{
					n = addNodes(S->processNode, DIFX_MESSAGE_MAX_CORES, &S->nProcess, atts[i+1]);
					for(j = S->nProcess-n; j < S->nProcess; j++)
					{
						S->nThread[j] = nThread;
					}
				}
			}
		}
	}
	else if(G->type == DIFX_MESSAGE_STATUS)
	{
		DifxMessageStatus *S;
		int ds = -1;

		S = &G->body.status;
		if(strcmp(name, "weight") == 0)
		{
			for(i = 0; atts[i]; i+=2)
			{
				if(strcmp(atts[i], "ant") == 0)
				{
					ds = atoi(atts[i+1]);
				}
				else if(strcmp(atts[i], "wt") == 0)
				{
					if(ds >= 0 && ds < DIFX_MESSAGE_MAX_DATASTREAMS)
					{
						if(ds > S->maxDS)
						{
							S->maxDS = ds;
						}
						S->weight[ds] = atof(atts[i+1]);
					}
				}
			}
		}
	}

	G->_xml_level++;
	strcpy(G->_xml_element[G->_xml_level], name);
}

static void XMLCALL endElement(void *userData, const char *name)
{
	DifxMessageGeneric *G;
	const char *elem;
	const char *s;
	int i;

	G = (DifxMessageGeneric *)userData;
	elem = G->_xml_element[G->_xml_level];
	s = G->_xml_string;

	if(G->_xml_string[0] != 0)
	{
		enum DifxMessageType t;
		enum Mk5State m;
		enum DifxState d;

		if(strcmp(G->_xml_element[0], "difxMessage") == 0)
		{
			if(strcmp(G->_xml_element[1], "header") == 0)
			{
				if(strcmp(elem, "from") == 0)
				{
					strncpy(G->from, s, 31);
					G->from[31] = 0;
				}
				else if(strcmp(elem, "to") == 0)
				{
					strncpy(G->to[G->nTo], s, 31);
					G->to[G->nTo][31] = 0;
					G->nTo++;
				}
				else if(strcmp(elem, "mpiProcessId") == 0)
				{
					G->mpiId = atoi(s);
				}
				else if(strcmp(elem, "identifier") == 0)
				{
					strncpy(G->identifier, s, 31);
					G->identifier[31] = 0;
				}
				else if(strcmp(elem, "type") == 0)
				{
					for(t = 0; t < NUM_DIFX_MESSAGE_TYPES; t++)
					{
						if(!strcmp(DifxMessageTypeStrings[t],s))
						{
							G->type = t;
							if(G->type == DIFX_MESSAGE_STATUS)
							{
								for(i = 0; i < DIFX_MESSAGE_MAX_DATASTREAMS; i++)
								{
									G->body.status.weight[i] = -1;
								}
								G->body.status.maxDS = -1;
							}
						}
					}
				}
			}
			else if(strcmp(G->_xml_element[1], "body") == 0)
			{
				if(strcmp(elem, "seqNumber") == 0)
				{
					G->seqNumber = atoi(s);
				}
				switch(G->type)
				{
				case DIFX_MESSAGE_LOAD:
					if(strcmp(elem, "cpuLoad") == 0)
					{
						G->body.load.cpuLoad = atof(s);
					}
					else if(strcmp(elem, "totalMemory") == 0)
					{
						G->body.load.totalMemory = atof(s);
					}
					else if(strcmp(elem, "usedMemory") == 0)
					{
						G->body.load.usedMemory = atof(s);
					}
					else if(strcmp(elem, "netRXRate") == 0)
					{
						G->body.load.netRXRate = atof(s);
					}
					else if(strcmp(elem, "netTXRate") == 0)
					{
						G->body.load.netTXRate = atof(s);
					}
					break;
				case DIFX_MESSAGE_ALERT:
					if(strcmp(elem, "alertMessage") == 0)
					{
						strncpy(G->body.alert.message, s, DIFX_MESSAGE_LENGTH-1);
						G->body.alert.message[DIFX_MESSAGE_LENGTH-1] = 0;
					}
					else if(strcmp(elem, "severity") == 0)
					{
						G->body.alert.severity = atoi(s);
					}
					break;
				case DIFX_MESSAGE_MARK5STATUS:
					if(strcmp(elem, "bankAVSN") == 0)
					{
						strncpy(G->body.mk5status.vsnA, s, 
							DIFX_MESSAGE_MARK5_VSN_LENGTH);
						G->body.mk5status.vsnA[DIFX_MESSAGE_MARK5_VSN_LENGTH] = 0;
					}
					else if(strcmp(elem, "bankBVSN") == 0)
					{
						strncpy(G->body.mk5status.vsnB, s, 
							DIFX_MESSAGE_MARK5_VSN_LENGTH);
						G->body.mk5status.vsnB[DIFX_MESSAGE_MARK5_VSN_LENGTH] = 0;
					}
					else if(strcmp(elem, "statusWord") == 0)
					{
						sscanf(s, "%x", 
							&G->body.mk5status.status);
					}
					else if(strcmp(elem, "activeBank") == 0)
					{
						G->body.mk5status.activeBank = s[0];
					}
					else if(strcmp(elem, "state") == 0)
					{
						G->body.mk5status.state = 0;
						for(m = 0; m < NUM_MARK5_STATES; m++)
						{
							if(strcmp(Mk5StateStrings[m], s) == 0)
							{
								G->body.mk5status.state = m;
							}
						}
					}
					else if(strcmp(elem, "scanNumber") == 0)
					{
						G->body.mk5status.scanNumber = atoi(s);
					}
					else if(strcmp(elem, "scanName") == 0)
					{
						strncpy(G->body.mk5status.scanName, s, 63);
						G->body.mk5status.scanName[63] = 0;
					}
					else if(strcmp(elem, "position") == 0)
					{
						G->body.mk5status.position = atoll(s);
					}
					else if(strcmp(elem, "playRate") == 0)
					{
						G->body.mk5status.rate = atof(s);
					}
					else if(strcmp(elem, "dataMJD") == 0)
					{
						G->body.mk5status.dataMJD = atof(s);
					}
					break;
				case DIFX_MESSAGE_CONDITION:
					if(strncmp(elem, "bin", 3) == 0)
					{
						i = atoi(elem+3);
						if(i >= 0 && i < DIFX_MESSAGE_N_CONDITION_BINS)
						{
							G->body.condition.bin[i] = atoi(s);
						}
					}
					else if(strcmp(elem, "serialNumber") == 0)
					{
						strncpy(G->body.condition.serialNumber, s,
							DIFX_MESSAGE_DISC_SERIAL_LENGTH);
						G->body.condition.serialNumber[DIFX_MESSAGE_DISC_SERIAL_LENGTH] = 0;
					}
					else if(strcmp(elem, "modelNumber") == 0)
					{
						strncpy(G->body.condition.modelNumber, s,
							DIFX_MESSAGE_DISC_MODEL_LENGTH);
						G->body.condition.modelNumber[DIFX_MESSAGE_DISC_MODEL_LENGTH] = 0;
					}
					else if(strcmp(elem, "size") == 0)
					{
						G->body.condition.diskSize = atoi(s);
					}
					else if(strcmp(elem, "moduleVSN") == 0)
					{
						strncpy(G->body.condition.moduleVSN, s,
							DIFX_MESSAGE_MARK5_VSN_LENGTH);
						G->body.condition.moduleVSN[DIFX_MESSAGE_MARK5_VSN_LENGTH] = 0;
					}
					else if(strcmp(elem, "moduleSlot") == 0)
					{
						G->body.condition.moduleSlot = atoi(s);
					}
					else if(strcmp(elem, "startMJD") == 0)
					{
						G->body.condition.startMJD = atof(s);
					}
					else if(strcmp(elem, "stopMJD") == 0)
					{
						G->body.condition.stopMJD = atof(s);
					}
					break;
				case DIFX_MESSAGE_MARK5VERSION:
					if(strcmp(elem, "ApiVer") == 0)
					{
						strncpy(G->body.mk5version.ApiVersion, s, 7);
						G->body.mk5version.ApiVersion[7] = 0;
					}
					else if(strcmp(elem, "ApiDate") == 0)
					{
						strncpy(G->body.mk5version.ApiDateCode, s, 11);
						G->body.mk5version.ApiDateCode[11] = 0;
					}
					else if(strcmp(elem, "FirmVer") == 0)
					{
						strncpy(G->body.mk5version.FirmwareVersion, s, 7);
						G->body.mk5version.FirmwareVersion[7] = 0;
					}
					else if(strcmp(elem, "FirmDate") == 0)
					{
						strncpy(G->body.mk5version.FirmDateCode, s, 11);
						G->body.mk5version.FirmDateCode[11] = 0;
					}
					else if(strcmp(elem, "MonVer") == 0)
					{
						strncpy(G->body.mk5version.MonitorVersion, s, 7);
						G->body.mk5version.MonitorVersion[7] = 0;
					}
					else if(strcmp(elem, "XbarVer") == 0)
					{
						strncpy(G->body.mk5version.XbarVersion, s, 7);
						G->body.mk5version.XbarVersion[7] = 0;
					}
					else if(strcmp(elem, "AtaVer") == 0)
					{
						strncpy(G->body.mk5version.AtaVersion, s, 7);
						G->body.mk5version.AtaVersion[7] = 0;
					}
					else if(strcmp(elem, "UAtaVer") == 0)
					{
						strncpy(G->body.mk5version.UAtaVersion, s, 7);
						G->body.mk5version.UAtaVersion[7] = 0;
					}
					else if(strcmp(elem, "DriverVer") == 0)
					{
						strncpy(G->body.mk5version.DriverVersion, s, 7);
						G->body.mk5version.DriverVersion[7] = 0;
					}
					else if(strcmp(elem, "BoardType") == 0)
					{
						strncpy(G->body.mk5version.BoardType, s, 12);
						G->body.mk5version.BoardType[11] = 0;
					}
					else if(strcmp(elem, "SerialNum") == 0)
					{
						G->body.mk5version.SerialNum = atoi(s);
					}
					else if(strcmp(elem, "PCBType") == 0)
					{
						strncpy(G->body.mk5version.DB_PCBType, s, 12);
						G->body.mk5version.DB_PCBType[11] = 0;
					}
					else if(strcmp(elem, "PCBSubType") == 0)
					{
						strncpy(G->body.mk5version.DB_PCBSubType, s, 12);
						G->body.mk5version.DB_PCBSubType[11] = 0;
					}
					else if(strcmp(elem, "PCBVer") == 0)
					{
						strncpy(G->body.mk5version.DB_PCBVersion, s, 8);
						G->body.mk5version.DB_PCBVersion[7] = 0;
					}
					else if(strcmp(elem, "DB_FPGAConfig") == 0)
					{
						strncpy(G->body.mk5version.DB_FPGAConfig, s, 12);
						G->body.mk5version.DB_FPGAConfig[11] = 0;
					}
					else if(strcmp(elem, "DB_FPGAConfigVer") == 0)
					{
						strncpy(G->body.mk5version.DB_FPGAConfigVersion, s, 8);
						G->body.mk5version.DB_FPGAConfigVersion[7] = 0;
					}
					break;
				case DIFX_MESSAGE_STATUS:
					if(strcmp(elem, "message") == 0)
					{
						strncpy(G->body.status.message, s, DIFX_MESSAGE_LENGTH-1);
						G->body.status.message[DIFX_MESSAGE_LENGTH-1] = 0;
					}
					else if(strcmp(elem, "state") == 0)
					{
						for(d = 0; d < NUM_DIFX_STATES; d++)
						{
							if(strcmp(DifxStateStrings[d], s) == 0)
							{
								G->body.status.state = d;
							}
						}
					}
					break;
				case DIFX_MESSAGE_INFO:
					if(strcmp(elem, "message") == 0)
					{
						strncpy(G->body.info.message, s, DIFX_MESSAGE_LENGTH-1);
						G->body.info.message[DIFX_MESSAGE_LENGTH-1] = 0;
					}
					break;
				case DIFX_MESSAGE_COMMAND:
					if(strcmp(elem, "command") == 0)
					{
						strncpy(G->body.command.command, s, DIFX_MESSAGE_LENGTH-1);
					}
					break;
				case DIFX_MESSAGE_PARAMETER:
					if(strcmp(elem, "targetMpiId") == 0)
					{
						G->body.param.targetMpiId = atoi(s);
					}
					else if(strncmp(elem, "index", 5) == 0)
					{
						int p;
						p = atoi(elem+5);
						if(p > 1 || p < DIFX_MESSAGE_MAX_INDEX)
						{
							int i;
							i = atoi(s);
							if(p > G->body.param.nIndex)
							{
								G->body.param.nIndex = p;
							}
							G->body.param.paramIndex[p-1] = i;
						}
					}
					else if(strcmp(elem, "name") == 0)
					{
						strncpy(G->body.param.paramName, s, DIFX_MESSAGE_PARAM_LENGTH-1);
					}
					else if(strcmp(elem, "value") == 0)
					{
						strncpy(G->body.param.paramValue, s, DIFX_MESSAGE_LENGTH-1);
					}
					break;
				case DIFX_MESSAGE_START:
					if(strcmp(elem, "input") == 0)
					{
						strncpy(G->body.start.inputFilename, s, DIFX_MESSAGE_FILENAME_LENGTH-1);
					}
					else if(strcmp(elem, "env") == 0)
					{
						if(G->body.start.nEnv < DIFX_MESSAGE_MAX_ENV)
						{
							strncpy(G->body.start.envVar[G->body.start.nEnv], s, DIFX_MESSAGE_FILENAME_LENGTH-1);
							G->body.start.nEnv++;
						}
					}
					else if(strcmp(elem, "mpiWrapper") == 0)
					{
						strncpy(G->body.start.mpiWrapper, s, DIFX_MESSAGE_FILENAME_LENGTH-1);
						G->body.start.mpiWrapper[DIFX_MESSAGE_FILENAME_LENGTH-1] = 0;
					}
					else if(strcmp(elem, "mpiOptions") == 0)
					{
						strncpy(G->body.start.mpiOptions, s, DIFX_MESSAGE_FILENAME_LENGTH-1);
						G->body.start.mpiOptions[DIFX_MESSAGE_FILENAME_LENGTH-1] = 0;
					}
					else if(strcmp(elem, "difxProgram") == 0)
					{
						strncpy(G->body.start.difxProgram, s, DIFX_MESSAGE_FILENAME_LENGTH-1);
						G->body.start.difxProgram[DIFX_MESSAGE_FILENAME_LENGTH-1] = 0;
					}
					else if(strcmp(elem, "force") == 0)
					{
						if(s[0] == '0' || s[0] == 'n' || s[0] == 'N' || s[0] == 'f' || s[0] == 'F')
						{
							G->body.start.force = 0;
						}
						else
						{
							G->body.start.force = 1;
						}
					}
					else if(strcmp(elem, "difxVersion") == 0)
					{
						strncpy(G->body.start.difxVersion, s, DIFX_MESSAGE_VERSION_LENGTH-1);
						G->body.start.difxVersion[DIFX_MESSAGE_VERSION_LENGTH-1] = 0;
					}
					break;
				case DIFX_MESSAGE_STOP:
					break;
				case DIFX_MESSAGE_TRANSIENT:
					if(strcmp(elem, "jobId") == 0)
					{
						strncpy(G->body.transient.jobId, s, DIFX_MESSAGE_IDENTIFIER_LENGTH-1);
					}
					else if(strcmp(elem, "startMJD") == 0)
					{
						G->body.transient.startMJD = atof(s);
					}
					else if(strcmp(elem, "stopMJD") == 0)
					{
						G->body.transient.stopMJD = atof(s);
					}
					else if(strcmp(elem, "priority") == 0)
					{
						G->body.transient.priority = atof(s);
					}
					else if(strcmp(elem, "destDir") == 0)
					{
						strncpy(G->body.transient.destDir, s, DIFX_MESSAGE_FILENAME_LENGTH-1);
					}
					else if(strcmp(elem, "comment") == 0)
					{
						strncpy(G->body.transient.comment, s, DIFX_MESSAGE_COMMENT_LENGTH-1);
					}
				default:
					break;
				}
			}
		}
	}

	G->_xml_level--;
}

static void XMLCALL charHandler(void *userData, const XML_Char *str, int len)
{
	DifxMessageGeneric *G;
	int l;

	G = (DifxMessageGeneric *)userData;

	l = strlen(G->_xml_string);

	if(len + l > 1023)
	{
		len = 1023 - l;
		if(len <= 0)
		{
			return;
		}
	}
	strncpy(G->_xml_string+l, str, len);
	G->_xml_string[len+l] = 0;
}

int difxMessageParse(DifxMessageGeneric *G, const char *message)
{
	XML_Parser parser;
	
	memset(G, 0, sizeof(DifxMessageGeneric));
	G->_xml_level = -1;
	G->type = DIFX_MESSAGE_UNKNOWN;
	parser = XML_ParserCreate(0);
	XML_ParserReset(parser, 0);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, charHandler);
	XML_SetUserData(parser, G);
	XML_Parse(parser, message, strlen(message), 0);
	XML_ParserFree(parser);

	return G->_xml_error_count;
}

void difxMessageGenericPrint(const DifxMessageGeneric *G)
{
	int i;

	printf("Generic Message [%p]\n", G);
	printf("  from = %s\n", G->from);
	printf("  to =");
	for(i = 0; i < G->nTo; i++)
	{
		printf(" %s", G->to[i]);
	}
	printf("\n");
	printf("  identifier = %s\n", G->identifier);
	printf("  mpi id = %d\n", G->mpiId);
	printf("  type = %s %d\n", DifxMessageTypeStrings[G->type], G->type);
	switch(G->type)
	{
	case DIFX_MESSAGE_LOAD:
		printf("    cpu load = %f\n", G->body.load.cpuLoad);
		printf("    total memory = %d kiB\n", G->body.load.totalMemory);
		printf("    used memory = %d kiB\n", G->body.load.usedMemory);
		printf("    network Receive Rate = %d B/s\n", 
			G->body.load.netRXRate);
		printf("    network Transmit Rate = %d B/s\n", 
			G->body.load.netTXRate);
		break;
	case DIFX_MESSAGE_ALERT:
		printf("    severity = %d\n", G->body.alert.severity);
		printf("    message = %s\n", G->body.alert.message);
		break;
	case DIFX_MESSAGE_MARK5STATUS:
		printf("    state = %s %d\n", 
			Mk5StateStrings[G->body.mk5status.state],
			G->body.mk5status.state);
		printf("    VSN A = %s\n", G->body.mk5status.vsnA);
		printf("    VSN B = %s\n", G->body.mk5status.vsnB);
		printf("    status word = 0x%x\n", G->body.mk5status.status);
		printf("    activeBank = %c\n", G->body.mk5status.activeBank);
		printf("    scanNumber = %d\n", G->body.mk5status.scanNumber);
		printf("    scanName = %s\n", G->body.mk5status.scanName);
		printf("    position = %lld\n", G->body.mk5status.position);
		printf("    rate = %7.3f Mbps\n", G->body.mk5status.rate);
		printf("    data MJD = %12.6f\n", G->body.mk5status.dataMJD);
		break;
	case DIFX_MESSAGE_MARK5VERSION:
		printf("    Api Version = %s  Date = %s\n", 
			G->body.mk5version.ApiVersion,
			G->body.mk5version.ApiDateCode);
		printf("    Firmware Version = %s  Date = %s\n", 
			G->body.mk5version.FirmwareVersion,
			G->body.mk5version.FirmDateCode);
		printf("    Monitor Version = %s\n", 
			G->body.mk5version.MonitorVersion);
		printf("    Xbar Version = %s\n", 
			G->body.mk5version.XbarVersion);
		printf("    Ata Version = %s\n", 
			G->body.mk5version.AtaVersion);
		printf("    UAta Version = %s\n", 
			G->body.mk5version.UAtaVersion);
		printf("    Driver Version = %s\n", 
			G->body.mk5version.DriverVersion);
		printf("    Board Type = %s  Serial Num. = %d\n",
			G->body.mk5version.BoardType,
			G->body.mk5version.SerialNum);
		if(G->body.mk5version.DB_PCBVersion[0] != 0)
		{
			printf("    Daughter Board:  Type = %s  Subtype = %s  Version = %s\n",
				G->body.mk5version.DB_PCBType,
				G->body.mk5version.DB_PCBSubType,
				G->body.mk5version.DB_PCBVersion);
			printf("    Daughter Board:  Configuration = %s  Version = %s\n",
				G->body.mk5version.DB_FPGAConfig,
				G->body.mk5version.DB_FPGAConfigVersion);
		}
		break;
	case DIFX_MESSAGE_CONDITION:
		printf("    serial number = %s\n", 
			G->body.condition.serialNumber);
		printf("    model number = %s\n",
			G->body.condition.modelNumber);
		printf("    disk size = %d GB\n",
			G->body.condition.diskSize);
		printf("    module VSN / slot = %s / %d\n", 
			G->body.condition.moduleVSN,
			G->body.condition.moduleSlot);
		printf("    MJD = %7.5f to %7.5f\n",
			G->body.condition.startMJD,
			G->body.condition.stopMJD);
		printf("    stats =");
		for(i = 0; i < DIFX_MESSAGE_N_CONDITION_BINS; i++)
		{
			printf(" %d", G->body.condition.bin[i]);
		}
		printf("\n");
		break;
	case DIFX_MESSAGE_STATUS:
		printf("    state = %s %d\n", 
			DifxStateStrings[G->body.status.state],
			G->body.status.state);
		printf("    message = %s\n", G->body.status.message);
		break;
	case DIFX_MESSAGE_INFO:
		printf("    message = %s\n", G->body.info.message);
		break;
	case DIFX_MESSAGE_COMMAND:
		printf("    command = %s\n", G->body.command.command);
		break;
	case DIFX_MESSAGE_PARAMETER:
		printf("    targetMpiId = %d\n", G->body.param.targetMpiId);
		printf("    name = %s", G->body.param.paramName);
		for(i = 0; i < G->body.param.nIndex; i++)
		{
			printf("[%d]", G->body.param.paramIndex[i]);
		}
		printf("\n");
		printf("    value = %s\n", G->body.param.paramValue);
		break;
	case DIFX_MESSAGE_START:
		printf("    MPI wrapper = %s\n", G->body.start.mpiWrapper);
		printf("    MPI options = %s\n", G->body.start.mpiOptions);
		printf("    program = %s\n", G->body.start.difxProgram);
		printf("    input file = %s\n", G->body.start.inputFilename);
		printf("    nEnv = %d\n", G->body.start.nEnv);
		for(i = 0; i < G->body.start.nEnv; i++)
		{
			printf("      %s\n", G->body.start.envVar[i]);
		}
		printf("    headNode = %s\n", G->body.start.headNode);
		printf("    nDatastream = %d\n", G->body.start.nDatastream);
		for(i = 0; i < G->body.start.nDatastream; i++)
		{
			printf("      %s\n", G->body.start.datastreamNode[i]);
		}
		printf("    nDatastream = %d\n", G->body.start.nDatastream);
		for(i = 0; i < G->body.start.nProcess; i++)
		{
			printf("      %s %d\n", G->body.start.processNode[i], G->body.start.nThread[i]);
		}
		printf("    force = %d\n", G->body.start.force);
		printf("    difxVersion = %s\n", G->body.start.difxVersion);
		break;
	case DIFX_MESSAGE_STOP:
		break;
	case DIFX_MESSAGE_TRANSIENT:
		printf("    jobId = %s\n", G->body.transient.jobId);
		printf("    startMJD = %14.8f\n", G->body.transient.startMJD);
		printf("    stopMJD = %14.8f\n", G->body.transient.stopMJD);
		printf("    priority = %f\n", G->body.transient.priority);
		printf("    destDir = %s\n", G->body.transient.destDir);
		printf("    comment = %s\n", G->body.transient.comment);
		break;
	default:
		break;
	}
	if(G->_xml_error_count > 0)
	{
		printf("  xml errors = %d\n", G->_xml_error_count);
	}
}
