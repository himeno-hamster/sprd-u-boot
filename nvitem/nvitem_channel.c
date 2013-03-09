
#include "nvitem_common.h"
#include "nvitem_channel.h"

#ifndef WIN32
#define CHANNEL_HEADER_TAG	0x7e7f
#define CHANNEL_HEADER_TYPE	0xaa55

typedef struct{
	uint16 tag;
	uint16 type;
	uint32 length;
	uint32 frame_num;
	uint32 reserved;
}CHANNEL_PACKET_HEADER;

#define _REV_BUFSIZE	4096
static uint8					 _revBuf[_REV_BUFSIZE];
static CHANNEL_PACKET_HEADER	*_revHeader;
static uint8					*_revBody;
#define _SEND_BUFSIZE	4096
static uint8					_sendBuf[_SEND_BUFSIZE];
static CHANNEL_PACKET_HEADER	*_sendHeader;
static uint8					*_sendBody;


static SDIO_HANDLE channel_fd = 0;

void channel_open(void)
{
	_revHeader = (CHANNEL_PACKET_HEADER*)_revBuf;
	_revBody = &_revBuf[sizeof(CHANNEL_PACKET_HEADER)];
	_sendHeader = (CHANNEL_PACKET_HEADER*)_sendBuf;
	_sendBody = &_sendBuf[sizeof(CHANNEL_PACKET_HEADER)];
	channel_fd =  sdio_open();
	return;
}


BOOLEAN channel_read(uint8* buf, uint32 size, uint32* hasRead)
{
	uint32 ifSuccess;
	uint32 retRead;

	*hasRead = 0;
	if(0 != gpio_get_value(CP_AP_RTS)){
		return 0;
	}
	do
	{
		gpio_set_value(AP_CP_RDY, 1); // r
		gpio_set_value(AP_CP_RTS, 1);
		while(0 == gpio_get_value(CP_AP_RDY));

		retRead = sdio_read(channel_fd,_revBuf,_REV_BUFSIZE);
		if((sizeof(CHANNEL_PACKET_HEADER)+size) > retRead){
			ifSuccess = 0;
		}
		else if(
			(CHANNEL_HEADER_TAG != _revHeader->tag)
			||(CHANNEL_HEADER_TYPE != _revHeader->type)
			||(_revHeader->length < size)
		){
			ifSuccess = 0;
		}
		else{
			memcpy(buf,_revBody,size);
			*hasRead = size;
			ifSuccess = 1;
		}

		if(ifSuccess) {
			gpio_set_value(AP_CP_RDY, 1);
			gpio_set_value(AP_CP_RTS, 0);
			while(1 == gpio_get_value(CP_AP_RDY));
			return 1;
		} else {
			gpio_set_value(AP_CP_RDY, 0);
			gpio_set_value(AP_CP_RTS, 0);
			while(1 == gpio_get_value(CP_AP_RDY));
			return 0;
		}
	}while(0);
}

BOOLEAN channel_write(uint8* buf, uint32 size, uint32* hasWrite)
{
	uint32 ifSuccess;
	uint32 retWrite;

	do{
		// prepare frame
		_sendHeader->tag = CHANNEL_HEADER_TAG;
		_sendHeader->type = CHANNEL_HEADER_TYPE;
		_sendHeader->length = size;
		_sendHeader->frame_num = 0;
		_sendHeader->reserved = 0xabcdef00;	// I don't understand how to use this member.  Jaons.Wu
		memcpy(_sendBody,buf,size);

		gpio_set_value(AP_CP_RDY, 0); // w
		gpio_set_value(AP_CP_RTS, 1);
		while(0 == gpio_get_value(CP_AP_RDY));

		retWrite = sdio_write(channel_fd,_sendBuf,_SEND_BUFSIZE);
		if((sizeof(CHANNEL_PACKET_HEADER)+size) > retWrite){
			ifSuccess = 0;
		}
		else{
			ifSuccess = 1;
		}
		if(ifSuccess) {
			gpio_set_value(AP_CP_RDY, 1);
			gpio_set_value(AP_CP_RTS, 0);
			while(1 == gpio_get_value(CP_AP_RDY));
			return 1;
		} else {
			gpio_set_value(AP_CP_RDY, 0);
			gpio_set_value(AP_CP_RTS, 0);
			while(1 == gpio_get_value(CP_AP_RDY));
			return 0;
		}
	}while(0);
}

void channel_close(void)
{
	sdio_close(channel_fd);
	return;
}

#else

#include <windows.h>
#include <Winnt.h>
#include <winioctl.h>

#define FILENAME	"D:\\analyzer\\commonTools\\testCode\\testdata\\packet.bin"
static HANDLE hDevice;
#define CHANNEL_BUFSIZE	2048
static uint8 channelBuf[CHANNEL_BUFSIZE];



void channel_open(void)
{
//	DWORD count;
	unsigned __int32 offsetH,offsetL;

	do
	{
		hDevice = CreateFile(FILENAME,  // drive to open
			GENERIC_READ|GENERIC_WRITE,                // no access to the drive
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,             // default security attributes
			OPEN_EXISTING,    // disposition
			0, //FILE_FLAG_OVERLAPPED,                // file attributes
			NULL
		);            // do not copy file attributes
		if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
		{
			Sleep(1);
			continue;
		}
		else
		{
			return;
		}
	}while(1);
//	DeviceIoControl(hDevice,FSCTL_LOCK_VOLUME,   NULL,0,NULL,0,&count,NULL);

	offsetH = 0;
	offsetL = 0;
	SetFilePointer(hDevice,offsetL, &offsetH, FILE_BEGIN);

	return;

}

BOOLEAN channel_read(uint8* buf, uint32 size, uint32* hasRead)
{
	DWORD lpNumberOfBytesRead;
	BOOL bResult;

//	DeviceIoControl(hDevice,FSCTL_LOCK_VOLUME,   NULL,0,NULL,0,&count,NULL);

	bResult = ReadFile(hDevice, channelBuf, CHANNEL_BUFSIZE, &lpNumberOfBytesRead, 0);

	if(1 != bResult)
	{
		lpNumberOfBytesRead = GetLastError();
		return FALSE;
	}
//	DeviceIoControl(hDevice,FSCTL_UNLOCK_VOLUME, NULL,0,NULL,0,&count,NULL);

	memcpy(buf,channelBuf,size);
	*hasRead = (size<lpNumberOfBytesRead?size:lpNumberOfBytesRead);

	return TRUE;
}

BOOLEAN channel_write(uint8* buf, uint32 size, uint32* hasWrite)
{
	return TRUE;
}

void channel_close(void)
{
//	DWORD count;
//	DeviceIoControl(hDevice,FSCTL_UNLOCK_VOLUME, NULL,0,NULL,0,&count,NULL);
	CloseHandle(hDevice);
	hDevice = 0;

	return;
}

#endif

