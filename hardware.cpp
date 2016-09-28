#include "hardware.h"

#include   <windows.h>   
#include   <stdio.h> 

#define   DFP_GET_VERSION   0x00074080   
#define   DFP_SEND_DRIVE_COMMAND   0x0007c084   
#define   DFP_RECEIVE_DRIVE_DATA   0x0007c088   

#pragma   pack(1)   
typedef   struct   _GETVERSIONOUTPARAMS   {   
	BYTE   bVersion;     //   Binary   driver   version.   
	BYTE   bRevision;     //   Binary   driver   revision.   
	BYTE   bReserved;     //   Not   used.   
	BYTE   bIDEDeviceMap;   //   Bit   map   of   IDE   devices.   
	DWORD   fCapabilities;   //   Bit   mask   of   driver   capabilities.   
	DWORD   dwReserved[4];   //   For   future   use.   
}   GETVERSIONOUTPARAMS,   *PGETVERSIONOUTPARAMS,   *LPGETVERSIONOUTPARAMS;   

typedef   struct   _HWSENDCMDINPARAMS   {   
	DWORD   cBufferSize;     //   Buffer   size   in   bytes   
	IDEREGS   irDriveRegs;     //   Structure   with   drive   register   values.   
	BYTE   bDriveNumber;     //   Physical   drive   number   to   send   
	//   command   to   (0,1,2,3).   
	BYTE   bReserved[3];     //   Reserved   for   future   expansion.   
	DWORD   dwReserved[4];     //   For   future   use.   
	//BYTE     bBuffer[1];       //   Input   buffer.   
}   HWSENDCMDINPARAMS,   *PHWSENDCMDINPARAMS,   *LPHWSENDCMDINPARAMS;

typedef   struct   _HWSENDCMDOUTPARAMS   {   
	DWORD         cBufferSize;     //   Size   of   bBuffer   in   bytes   
	DRIVERSTATUS   DriverStatus;     //   Driver   status   structure.   
	BYTE       bBuffer[512];       //   Buffer   of   arbitrary   length   
	//   in   which   to   store   the   data   read   from   the   drive.   
}   HWSENDCMDOUTPARAMS,   *PHWSENDCMDOUTPARAMS,   *LPHWSENDCMDOUTPARAMS;  

typedef   struct   _IDSECTOR   {   
	USHORT   wGenConfig;   
	USHORT   wNumCyls;   
	USHORT   wReserved;   
	USHORT   wNumHeads;   
	USHORT   wBytesPerTrack;   
	USHORT   wBytesPerSector;   
	USHORT   wSectorsPerTrack;   
	USHORT   wVendorUnique[3];   
	CHAR   sSerialNumber[20];   
	USHORT   wBufferType;   
	USHORT   wBufferSize;   
	USHORT   wECCSize;   
	CHAR   sFirmwareRev[8];   
	CHAR   sModelNumber[40];   
	USHORT   wMoreVendorUnique;   
	USHORT   wDoubleWordIO;   
	USHORT   wCapabilities;   
	USHORT   wReserved1;   
	USHORT   wPIOTiming;   
	USHORT   wDMATiming;   
	USHORT   wBS;   
	USHORT   wNumCurrentCyls;   
	USHORT   wNumCurrentHeads;   
	USHORT   wNumCurrentSectorsPerTrack;   
	ULONG   ulCurrentSectorCapacity;   
	USHORT   wMultSectorStuff;   
	ULONG   ulTotalAddressableSectors;   
	USHORT   wSingleWordDMA;   
	USHORT   wMultiWordDMA;   
	BYTE   bReserved[128];   
}   IDSECTOR,   *PIDSECTOR;   
/*+++   
Global   vars   
---*/   
GETVERSIONOUTPARAMS   vers;   
HWSENDCMDINPARAMS   in;   
HWSENDCMDOUTPARAMS   out;   
HANDLE   h;   
DWORD   i;   
BYTE   j;   


VOID   ChangeByteOrder(PCHAR   szString,   USHORT   uscStrSize)   
{   

	USHORT   i;   
	CHAR   temp;   

	for   (i   =   0;   i   <   uscStrSize;   i+=2)   
	{   
		temp   =   szString[i];   
		szString[i]   =   szString[i+1];   
		szString[i+1]   =   temp;   
	}   
}   

void   hdid9x(char *Module_Number,char *Serial_Number)
{   
	ZeroMemory(&vers,sizeof(vers));   
	//We   start   in   95/98/Me   
	h=CreateFileW(TEXT("\\\\.\\Smartvsd "),0,0,0,CREATE_NEW,0,0);   
	if   (!h){   
		exit(0);   
	}   

	if   (!DeviceIoControl(h,DFP_GET_VERSION,0,0,&vers,sizeof(vers),&i,0)){   
		CloseHandle(h);   
		return;   
	}   
	//If   IDE   identify   command   not   supported,   fails   
	if   (!(vers.fCapabilities&1)){   
		CloseHandle(h);   
		return;   
	}   
	//Display   IDE   drive   number   detected   
	//Identify   the   IDE   drives   
	for   (j=0;j <4;j++){   
		PIDSECTOR   phdinfo;   
		char   s[41];   

		ZeroMemory(&in,sizeof(in));   
		ZeroMemory(&out,sizeof(out));   
		if   (j&1){   
			in.irDriveRegs.bDriveHeadReg=0xb0;   
		}else{   
			in.irDriveRegs.bDriveHeadReg=0xa0;   
		}   
		if   (vers.fCapabilities&(16>> j)){   
			//We   don 't   detect   a   ATAPI   device.   
			continue;   
		}else{   
			in.irDriveRegs.bCommandReg=0xec;   
		}   
		in.bDriveNumber=j;   
		in.irDriveRegs.bSectorCountReg=1;   
		in.irDriveRegs.bSectorNumberReg=1;   
		in.cBufferSize=512;   
		if   (!DeviceIoControl(h,DFP_RECEIVE_DRIVE_DATA,&in,sizeof(in),&out,sizeof(out),&i,0)){   
			CloseHandle(h);   
			return;   
		}   
		phdinfo=(PIDSECTOR)out.bBuffer;   
		memcpy(s,phdinfo-> sModelNumber,40);   
		s[40]=0;   
		ChangeByteOrder(s,40);  
		memcpy(Module_Number,s,41); 
		memcpy(s,phdinfo-> sFirmwareRev,8);   
		s[8]=0;   
		ChangeByteOrder(s,8);   
		memcpy(s,phdinfo-> sSerialNumber,20);   
		s[20]=0;   
		ChangeByteOrder(s,20); 
		memcpy(Serial_Number,s,21);
	}   

	//Close   handle   before   quit   
	CloseHandle(h);   

}   
void   hdidnt(char *Module_Number,char *Serial_Number)
{   
	//WCHAR   hd[80];   
	PIDSECTOR   phdinfo;   
	char   s[41];

	ZeroMemory(&vers,sizeof(vers));   
	//We   start   in   NT/Win2000   
	for   (j=0;j <4;j++)
	{   
		//sprintf(hd,TEXT("\\\\.\\PhysicalDrive%d "),j); 
		if(j==0)
		{
			h=CreateFileW(TEXT("\\\\.\\PhysicalDrive0 "),GENERIC_READ|GENERIC_WRITE,   
				FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);   
		}
		else if(j==1)
		{
			h=CreateFile(TEXT("\\\\.\\PhysicalDrive1 "),GENERIC_READ|GENERIC_WRITE,   
				FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
		}
		else if(j==2)
		{
			h=CreateFile(TEXT("\\\\.\\PhysicalDrive2 "),GENERIC_READ|GENERIC_WRITE,   
				FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
		}
		else if(j==3)
		{
			h=CreateFile(TEXT("\\\\.\\PhysicalDrive3 "),GENERIC_READ|GENERIC_WRITE,   
				FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);
		}
		if   (!h){   
			continue;   
		}   
		if   (!DeviceIoControl(h,DFP_GET_VERSION,0,0,&vers,sizeof(vers),&i,0)){   
			CloseHandle(h);   
			continue;   
		}   
		//If   IDE   identify   command   not   supported,   fails   
		if   (!(vers.fCapabilities&1)){   
			CloseHandle(h);   
			return;   
		}   
		//Identify   the   IDE   drives   
		ZeroMemory(&in,sizeof(in));   
		ZeroMemory(&out,sizeof(out));   
		if   (j&1){   
			in.irDriveRegs.bDriveHeadReg=0xb0;   
		}else{   
			in.irDriveRegs.bDriveHeadReg=0xa0;   
		}   
		if   (vers.fCapabilities&(16>> j)){   
			//We   don 't   detect   a   ATAPI   device.   
			continue;   
		}else{   
			in.irDriveRegs.bCommandReg=0xec;   
		}   
		in.bDriveNumber=j;   
		in.irDriveRegs.bSectorCountReg=1;   
		in.irDriveRegs.bSectorNumberReg=1;   
		in.cBufferSize=512;   
		if   (!DeviceIoControl(h,DFP_RECEIVE_DRIVE_DATA,&in,sizeof(in),&out,sizeof(out),&i,0))
		{   
			CloseHandle(h);   
			return;   
		}   
		phdinfo=(PIDSECTOR)out.bBuffer;   
		memcpy(s,phdinfo-> sModelNumber,40);   
		s[40]=0;   
		ChangeByteOrder(s,40); 
		memcpy(Module_Number,s,41); 
		memcpy(s,phdinfo-> sFirmwareRev,8);   
		s[8]=0;   
		ChangeByteOrder(s,8);   
		memcpy(s,phdinfo-> sSerialNumber,20);   
		s[20]=0;   
		ChangeByteOrder(s,20); 
		memcpy(Serial_Number,s,21); 
		CloseHandle(h);   
	}   
}   

void cpuid_p(DWORD veax,DWORD *PB)
{
	DWORD deax,debx,decx,dedx;
	__asm{
		mov eax,veax   //将输入参数移入eax
			cpuid     //;执行cpuid
			mov deax,eax
			mov debx,ebx
			mov decx,ecx
			mov dedx,edx
	}
	*PB++ = deax;
	*PB++ = debx;
	*PB++ = decx;
	*PB++ = dedx;
}


int get_ideid(unsigned char *Module_Number,unsigned char *Serial_Number)
{
	OSVERSIONINFO   VersionInfo;   

	ZeroMemory(&VersionInfo,sizeof(VersionInfo));   
	VersionInfo.dwOSVersionInfoSize=sizeof(VersionInfo);   
	GetVersionEx(&VersionInfo);   

	switch   (VersionInfo.dwPlatformId)
	{   
	case   VER_PLATFORM_WIN32s:   
		return 0;   
	case   VER_PLATFORM_WIN32_WINDOWS:   
		hdid9x((char *)Module_Number,(char *)Serial_Number);  
		return 0;   
	case   VER_PLATFORM_WIN32_NT:   
		hdidnt((char *)Module_Number,(char *)Serial_Number);  
		return 0;
	}

	return 0;
}


void GetCPUID(unsigned char *pMyCpuID)
{
	unsigned char vendor_id[]="------------";
	DWORD  gd[5];
	unsigned char pCPUID1[32] = { 0 };
	unsigned char pCPUID2[32] = { 0 };

	cpuid_p(0,gd);
	memcpy(vendor_id,&gd[1],4);
	memcpy(vendor_id+4,&gd[3],4);
	memcpy(vendor_id+8,&gd[2],4);

	cpuid_p(1,gd);
	sprintf((char *)pCPUID1,":%08x,",gd[0]);

	cpuid_p(1,gd);
	sprintf((char *)pCPUID2,"%08x,%08x",gd[3],gd[2]);

	sprintf((char *)pMyCpuID,"%s%s%s",vendor_id,pCPUID1,pCPUID2);
}


//************************************************************/
/* CRC 高位字节值表 */ 
unsigned char auchCRCHi[256] = { 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 
} ; 
/* CRC低位字节值表*/ 
unsigned char auchCRCLo[256] = { 
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 
	0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 
	0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 
	0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 
	0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 
	0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 
	0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 
	0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 
	0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 
	0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 
	0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 
	0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 
	0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 
	0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 
	0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 
	0x43, 0x83, 0x41, 0x81, 0x80, 0x40 
} ;


void get_xrx(unsigned char *Module_Number,unsigned char *cpuids,unsigned char *dat)
{
	int i = strlen((const char *)Module_Number);
	int k;
	unsigned char temp;
	int pt=0;
	for(k=0;k<i;k++)   //。
	{
		if(Module_Number[i-k] > 0x2f)
		{
			temp = Module_Number[i-k];
			dat[pt++] = auchCRCHi[temp];
		}
		if(pt >= 8)
			break;
	}

	i = strlen((const char *)cpuids);
	pt=8;
	for(k=0;k<15;k++)   //。
	{
		temp = cpuids[k+15];
		dat[pt++] = auchCRCLo[temp];
	}
}

unsigned char Module_Number[50];

void getSerialNumber(unsigned char *pSerialNumberBytes)
{
	unsigned char cpuids[128] = { 0 };
	unsigned char *se_temp;

	get_ideid(Module_Number,pSerialNumberBytes); 
	memcpy(&Module_Number[13],&pSerialNumberBytes[12],20); 
	GetCPUID(cpuids);
	se_temp = Module_Number;
	get_xrx(se_temp,cpuids,pSerialNumberBytes);
}