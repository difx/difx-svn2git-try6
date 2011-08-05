#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include "config.h"
#include "vsis_commands.h"

int DTS_id_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : mk5daemon : %s : %s : 1;", fields[0], VERSION, D->hostName);
}

int packet_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : %d : %d : %d : %d : %d;", fields[0],
		D->payloadOffset, D->dataFrameOffset, D->packetSize, D->psnMode, D->psnOffset);
}

int packet_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	if(nField > 1 && fields[1][0])
	{
		D->payloadOffset = atoi(fields[1]);
	}
	if(nField > 2 && fields[2][0])
	{
		D->dataFrameOffset = atoi(fields[2]);
	}
	if(nField > 3 && fields[3][0])
	{
		D->packetSize = atoi(fields[3]);
	}
	if(nField > 4 && fields[4][0])
	{
		D->psnMode = atoi(fields[4]);
	}
	if(nField > 5 && fields[5][0])
	{
		D->psnOffset = atoi(fields[5]);
	}

	return snprintf(response, maxResponseLength, "!%s = 0;", fields[0]);
}

int bank_set_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int newBank = -1;
	int v;

	if(nField < 2)
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : Bank name must be A, B or INC, case insensitive;", fields[0]);
	}
	else if(nField > 2)
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : Only one parameter allowed;", fields[0]);
	}
	else if(strlen(fields[1]) == 1)
	{
		newBank = toupper(fields[1][0]) - 'A';
		if(newBank < 0 || newBank >= N_BANK)
		{
			newBank = -1;

			v = snprintf(response, maxResponseLength, "!%s = 6 : Bank name must be A, B or INC, case insensitive;", fields[0]);
		}
	}
	else if(strcasecmp(fields[1], "INC") == 0)
	{
		if(D->activeBank < 0)
		{
			newBank = 0;
		}
		else
		{
			newBank = (D->activeBank + 1) % N_BANK;
		}
	}
	if(newBank >= 0)
	{
		if(D->vsns[newBank][0] == 0)
		{
			v =  snprintf(response, maxResponseLength, "!%s = 1 : No disk mounted in bank %c;", fields[0], 'A'+newBank);
		}
		else
		{
			D->activeBank = newBank;

			v = snprintf(response, maxResponseLength, "!%s = 0;", fields[0]);
		}
	}

	return v;
}

int bank_set_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int otherBank;
	int bank;
	int v;

	/* FIXME: report extended VSN? */

	if(D->activeBank >= 0)
	{
		bank = D->activeBank;
	}
	else
	{
		bank = 0;
	}

	otherBank = (bank + 1) % N_BANK;

	v = snprintf(response, maxResponseLength, "!%s? 0 : %c : %s : %c : %s;", fields[0],
		(D->vsns[bank][0]      ? ('A' + bank)       : '-'),
		(D->vsns[bank][0]      ? D->vsns[bank]      : "-"),
		(D->vsns[otherBank][0] ? ('A' + otherBank)  : '-'),
		(D->vsns[otherBank][0] ? D->vsns[otherBank] : "-") );

	return v;
}

int SS_rev_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;
	char dbInfo[DIFX_MESSAGE_LENGTH];

	if(strlen(D->mk5ver.DB_PCBVersion) > 0)
	{
		/* FIXME: get DBSerialNum and DBNumChannels */
		sprintf(dbInfo, " : DBPCBVersion %s : DBPCBType %s : DBPCBSubType %s : DBFPGConfig %s : DBFPGAConfigVers %s",
			D->mk5ver.DB_PCBVersion, 
			D->mk5ver.DB_PCBType,
			D->mk5ver.DB_PCBSubType,
			D->mk5ver.DB_FPGAConfig,
			D->mk5ver.DB_FPGAConfigVersion);
	}
	else
	{
		dbInfo[0] = 0;
	}

	v = snprintf(response, maxResponseLength, "!%s? 0 : BoardType %s : SerialNum %d : APIVersion %s : APIDateCode %s : FirmwareVersion %s : FirmDateCode %s : MonitorVersion %s : XBarVersion %s : ATAVersion %s : UATAVersion %s : DriverVersion %s%s;", fields[0],
		D->mk5ver.BoardType,
		D->mk5ver.SerialNum,
		D->mk5ver.ApiVersion,
		D->mk5ver.ApiDateCode,
		D->mk5ver.FirmwareVersion,
		D->mk5ver.FirmDateCode,
		D->mk5ver.MonitorVersion,
		D->mk5ver.XbarVersion,
		D->mk5ver.AtaVersion,
		D->mk5ver.UAtaVersion,
		D->mk5ver.DriverVersion,
		dbInfo);

	return v;
}

int OS_rev_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	FILE *pin;
	int v;
	const char command[] = "uname -rms";
	char A[3][32];

	A[0][0] = A[1][0] = A[2][0] = 0;

	pin = popen(command, "r");
	if(!pin)
	{
		v = snprintf(response, maxResponseLength, "!%s? 1 : Unable to execute : %s;", fields[0], command);
	}
	else
	{
		fscanf(pin, "%s%s%s", A[0], A[1], A[2]);
		fclose(pin);

		v = snprintf(response, maxResponseLength, "!%s? 0 : %s : %s : %s : %s;", fields[0], D->hostName, A[0], A[1], A[2]);
	}

	return v;
}

int fill_pattern_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v = -1;
	UINT32 p;

	if(nField != 2)
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : One parameter must be supplied;", fields[0]);
	}
	else
	{
		if(strlen(fields[1]) < 3 || fields[1][0] != '0' || fields[1][1] != 'x')
		{
			v = snprintf(response, maxResponseLength, "!%s = 6 : Hexadecimal value expected;", fields[0]);
		}
		else
		{
			v = sscanf(fields[1]+2, "%x", &p);
			if(v != 1)
			{
				v = snprintf(response, maxResponseLength, "!%s = 6 : Hexadecimal value expected;", fields[0]);
			}
			else
			{
				v = snprintf(response, maxResponseLength, "!%s = 0;", fields[0]);
			}
		}
	}

	return v;
}

int fill_pattern_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : 0x%08x;", fields[0], D->fillPattern);
}

int protect_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

#ifdef HAVE_XLRAPI_H
	enum WriteProtectState state;
	char msg[256];

	if(nField != 1 || (strcmp(fields[1], "on") != 0 && strcmp(fields[1], "off") != 0))
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : on or off expected;", fields[0]);
	}
	else if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s = 4 : No module mounted;", fields[0]);
	}
	else
	{
		state = (strcmp(fields[1], "on") == 0 ? PROTECT_ON : PROTECT_OFF);
		v = Mk5Daemon_setProtect(D, state, msg);

		if(v < 0)
		{
			v = snprintf(response, maxResponseLength, "!%s = 4 : Error %d encountered;", fields[0], -v);
		}
		else if(v == 0)
		{
			v = snprintf(response, maxResponseLength, "!%s = %d;", fields[0], v);
		}
	}
#else
	v = snprintf(response, maxResponseLength, "!%s = 2 : Not implemented on this DTS;", fields[0]);
#endif

	return v;
}

int protect_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

#ifdef HAVE_XLRAPI_H
	int p;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;");
	}
	else
	{
		p = D->bank_stat[D->activeBank].WriteProtected;
		v = snprintf(response, maxResponseLength, "!%s? 0 : %s;", fields[0], (p ? "on" : "off") );
	}
#else
	v = snprintf(response, maxResponseLength, "!%s = 2 : Not implemented on this DTS;", fields[0]);
#endif

	return v;
}

int error_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

#ifdef HAVE_XLRAPI_H
	char msg[256];
	unsigned int xlrError;
	int p;

	p = Mk5Daemon_error(D, &xlrError , msg);

	v = snprintf(response, maxResponseLength, "!%s? %d : %u : %s", fields[0], p, xlrError, msg);

#else
	v = snprintf(response, maxResponseLength, "!%s? 2 : Not implemented on this DTS;", fields[0]);
#endif

	return v;
}

int disk_model_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else 
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;
	
		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0", fields[0]);

			for(int d = 0; d < N_SMART_DRIVES; d++)
			{
				const DriveInformation *drive = smart->drive + d;
				int good = (drive->capacity > 0LL);

				v += snprintf(response+v, maxResponseLength-v, " : %s",
					(good ? drive->model : "") );
			}
			v += snprintf(response+v, maxResponseLength-v, ";");
		}
	}
	
	return v;
}

int disk_model_rev_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else 
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;

		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0", fields[0]);

			for(int d = 0; d < N_SMART_DRIVES; d++)
			{
				const DriveInformation *drive = smart->drive + d;
				int good = (drive->capacity > 0LL);

				v += snprintf(response+v, maxResponseLength-v, " : %s",
					(good ? drive->rev : "") );
			}
			v += snprintf(response+v, maxResponseLength-v, ";");
		}
	}
	
	return v;
}

int disk_serial_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;
		
		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0", fields[0]);

			for(int d = 0; d < N_SMART_DRIVES; d++)
			{
				const DriveInformation *drive = smart->drive + d;
				int good = (drive->capacity > 0LL);

				v += snprintf(response+v, maxResponseLength-v, " : %s",
					(good ? drive->serial : "") );
			}
			v += snprintf(response+v, maxResponseLength-v, ";");
		}
	}
	
	return v;
}

int disk_size_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;
		
		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0", fields[0]);

			for(int d = 0; d < N_SMART_DRIVES; d++)
			{
				const DriveInformation *drive = smart->drive + d;
				int good = (drive->capacity > 0LL);

				if(good)
				{
					v += snprintf(response+v, maxResponseLength-v, " : %Ld", drive->capacity);
				}
				else
				{
					v += snprintf(response+v, maxResponseLength-v, " :");
				}
			}
			v += snprintf(response+v, maxResponseLength-v, ";");
		}
	}
	
	return v;
}

int dir_info_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;
		
		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0 : %d : %Ld : %Ld;", fields[0],
				D->nScan[D->activeBank], D->bytesUsed[D->activeBank], D->bytesTotal[D->activeBank]);
		}
	}

	return v;
}

int pointers_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(D->activeBank < 0)
	{
		v = snprintf(response, maxResponseLength, "!%s? 4 : No module mounted;", fields[0]);
	}
	else
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;
		
		if(smart->mjd < 50000.0)	/* information not populated */
		{
			v = snprintf(response, maxResponseLength, "!%s? 5;", fields[0]);
		}
		else
		{
			v = snprintf(response, maxResponseLength, "!%s? 0 : %Ld : %Ld : %Ld;", fields[0],
				D->bytesUsed[D->activeBank], D->startPointer[D->activeBank], D->stopPointer[D->activeBank]);
		}
	}

	return v;
}

int personality_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : mark5C : bank;", fields[0]);
}

int personality_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v;

	if(nField != 2)
	{
		 v = snprintf(response, maxResponseLength, "!%s = 6 : Two parameters must be supplied;", fields[0]);
	}
	else
	{
		if(strcasecmp(fields[1], "mark5C") != 0)
		{
			v = snprintf(response, maxResponseLength, "!%s = 4 : Only mark5C personality is supported;", fields[0]);
		}
		else if(strcasecmp(fields[2], "bank") != 0)
		{
			v = snprintf(response, maxResponseLength, "!%s = 4 : Only bank-mode is supported;", fields[0]);
		}
	}

	return v;
}

int bank_info_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int bank;
	int v;
	char bankStr[N_BANK][60];

	if(D->activeBank >= 0)
	{
		bank = D->activeBank;
	}
	else
	{
		bank = 0;
	}

	for(int b = 0; b < N_BANK; b++)
	{
		const Mk5Smart *smart = D->smartData + D->activeBank;

		if(D->vsns[bank][0])
		{
			if(smart->mjd >= 50000.0)
			{
				long long left = D->bytesTotal[bank] - D->bytesUsed[bank];
				if(left < 0LL)
				{
					left = 0LL;
				}
				sprintf(bankStr[b], "%c : %Ld", 'A'+bank, left);
			}
			else
			{
				sprintf(bankStr[b], "? : 0");
			}
		}
		else
		{
			sprintf(bankStr[b], "- : 0");
		}
	
		bank = (bank + 1) % N_BANK;
	}

	v = snprintf(response, maxResponseLength, "!%s? 0 : %s : %s;", fields[0], bankStr[0], bankStr[1]);

	return v;
}

int net_protocol_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : %s;", fields[0], netProtocolStrings[D->netProtocol]);
}

int net_protocol_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v = -1;

	if(nField != 1)
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : One parameter must be supplied;", fields[0]);
	}
	else
	{
		for(int n = 0; n < NUM_NET_PROTOCOLS; n++)
		{
			if(strcasecmp(fields[1], netProtocolStrings[n]) == 0)
			{
				D->netProtocol = static_cast<NetProtocolType>(n);
				v = snprintf(response, maxResponseLength, "!%s = 0;", fields[0]);

				break;
			}
		}

		if(v < 0)
		{
			v = snprintf(response, maxResponseLength, "!%s = 6 : Unrecognized net protocol;" ,fields[0]);
		}
	}

	return v;
}

int disk_state_mask_Query(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	return snprintf(response, maxResponseLength, "!%s? 0 : %d : %d : %d;", fields[0], 
		(D->diskStateMask & 1 ? 1 : 0), (D->diskStateMask & 2 ? 1 : 0), (D->diskStateMask & 4 ? 1 : 0));
}

int disk_state_mask_Command(Mk5Daemon *D, int nField, char **fields, char *response, int maxResponseLength)
{
	int v = 0, d;

	d = D->diskStateMask;

	if(nField > 4)
	{
		v = snprintf(response, maxResponseLength, "!%s = 6 : Up to three parameters allowed;", fields[0]);
	}
	else
	{
		if(nField > 1 && fields[1][0])
		{
			if(strcmp(fields[1], "1") == 0)
			{
				d = d | 1;
			}
			else if(strcmp(fields[1], "0") == 0)
			{
				d = d & ~1;
			}
			else
			{
				v = -1;
			}
		}
		if(nField > 2 && fields[2][0])
		{
			if(strcmp(fields[2], "1") == 0)
			{
				d = d | 2;
			}
			else if(strcmp(fields[2], "0") == 0)
			{
				d = d & ~2;
			}
			else
			{
				v = -1;
			}
		}
		if(nField > 3 && fields[3][0])
		{
			if(strcmp(fields[3], "1") == 0)
			{
				d = d | 4;
			}
			else if(strcmp(fields[3], "0") == 0)
			{
				d = d & ~4;
			}
			else
			{
				v = -1;
			}
		}
		if(v == -1)
		{
			v = snprintf(response, maxResponseLength, "!%s = 6 : Parameters must be 0 or 1;", fields[0]);
		}
		else
		{
			v = v = snprintf(response, maxResponseLength, "!%s = 0;", fields[0]);
		}
	}

	return v;
}
