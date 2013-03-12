
#include "nvitem_common.h"
#include "nvitem_channel.h"
#include "nvitem_sync.h"
#include "nvitem_packet.h"
#include "nvitem_os.h"
#include "nvitem_buf.h"

int nvitem_sync(void)
{
	static uint32 ifInit = 0;

	if(0 == ifInit){
		initEvent();
		initBuf();
		channel_open();
		_initPacket();
		_syncInit();
		ifInit = 1;
	}
	syncAnalyzer();
}


