#ifndef _OS_API_H
#define _OS_API_H

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
// c standard head file.
#include <linux/string.h>
#include <malloc.h>

// Basic data types.
#include "sci_types.h"


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/
// Invalid state value.
#define SCI_INVALID_STATE           ~0
 
//---------------------------------------------
// API return values.
//---------------------------------------------
//Success, no error.
#define SCI_SUCCESS                 0x00
//Object was deleted. 
#define SCI_DELETED                 0x01
//Invalid pool pointer.
#define SCI_POOL_ERROR              0x02
//Invalid pointer.
#define SCI_PTR_ERROR               0x03
//A wait option other than TX_NO_WAIT was specified on call from a non-thread.
#define SCI_WAIT_ERROR              0x04
//Size is invalid.
#define SCI_SIZE_ERROR              0x05
//Invalid event group pointer.
#define SCI_GROUP_ERROR             0x06
//Service was unable to get the specified events.
#define SCI_NO_EVENTS               0x07
//Invalid option was specified.
#define SCI_OPTION_ERROR            0x08
//Invalid queue pointer.
#define SCI_QUEUE_ERROR             0x09
//Service was unable to retrieve a message because the queue was empty.
#define SCI_QUEUE_EMPTY             0x0A
//Service was unable to send message because the queue was full.
#define SCI_QUEUE_FULL              0x0B
//Invalid counting semaphore pointer.
#define SCI_SEMAPHORE_ERROR         0x0C
//Service was unable to retrieve an instance of the counting semaphore (semaphore count is zero).
#define SCI_NO_INSTANCE             0x0D
//Invalid thread control pointer.
#define SCI_THREAD_ERROR            0x0E
//Invalid thread priority, which is a value outside the range of 0-31.
#define SCI_PRIORITY_ERROR          0x0F
//Service was unable to allocate memory.
#define SCI_NO_MEMORY               0x10
//Specified thread is not in a terminated or completed state.
#define SCI_DELETE_ERROR            0x11
//Specified thread is not suspended.
#define SCI_RESUME_ERROR            0x12
//Invalid caller of this service.
#define SCI_CALLER_ERROR            0x13
//Specified thread is in a terminated or completed state.
#define SCI_SUSPEND_ERROR           0x14
//Invalid application timer pointer.
#define SCI_TIMER_ERROR             0x15
//Invalid value (a zero) supplied for initial ticks.
#define SCI_TICK_ERROR              0x16
//Timer was already active./Invalid activation selected.
#define SCI_ACTIVATE_ERROR          0x17
//Invalid preemption threshold specified. 
//This value must be a valid priority less than or equal to the initial priority of the thread.
#define SCI_THRESH_ERROR            0x18
//Previously set delayed suspension was lifted.
#define SCI_SUSPEND_LIFTED          0x19
//Suspension was aborted by another thread, timer, or ISR.
#define SCI_WAIT_ABORTED            0x1A
//Specified thread is not in a waiting state.
#define SCI_WAIT_ABORT_ERROR        0x1B
//Invalid mutex pointer.
#define SCI_MUTEX_ERROR             0x1C
//Service was unable to get ownership of the mutex.
#define SCI_NOT_AVAILABLE           0x1D
//Mutex is not owned by caller.
#define SCI_NOT_OWNED               0x1E
//Invalid priority inherit parameter.
#define SCI_INHERIT_ERROR           0x1F
//Invalid auto-start selection.
#define SCI_START_ERROR             0x20
//Parameter is invalid.
#define SCI_PARAM_ERROR             0x21
//Normal error.
#define SCI_ERROR                   0xFF

//---------------------------------------------
// API input parameters.
//---------------------------------------------
// Boolean value
#define SCI_TRUE                    TRUE       // Boolean true value
#define SCI_FALSE                   FALSE       // Boolean false value

// Wait option.
#define SCI_NO_WAIT                 0x0
#define SCI_WAIT_FOREVER            0xFFFFFFFF

// Option used to set/get event.
#define SCI_OR                      0
#define SCI_OR_CLEAR                1
#define SCI_AND                     2
#define SCI_AND_CLEAR               3

// Auto start option on thread
#define SCI_DONT_START              0
#define SCI_AUTO_START              1

// Indicates if the thread is preemptable.
#define SCI_NO_PREEMPT              0
#define SCI_PREEMPT                 1

// Auto start option on timer.
#define SCI_NO_ACTIVATE             0
#define SCI_AUTO_ACTIVATE           1

//@Zhemin.Lin, add, 09/12/2003, CR:MS00004678
//priority inherit mode for mutex
#define SCI_NO_INHERIT 0
#define SCI_INHERIT 1

// Thread priority definetion
#define SCI_PRIORITY_KERNEL         0 
#define SCI_PRIORITY_TIME_CRITICAL  5 
#define SCI_PRIORITY_HIGHEST        10
#define SCI_PRIORITY_ABOVE_NORMAL   15
#define SCI_PRIORITY_NORMAL         20
#define SCI_PRIORITY_BELOW_NORMAL   24
#define SCI_PRIORITY_LOWEST         28
#define SCI_PRIORITY_IDLE           31

//---------------------------------------------
// General constants.
//---------------------------------------------
// Max ID of static thread MUST be less than this value.
#define SCI_MAX_STATIC_THREAD_ID    100
// Number of all static threads should less than this value.
#define SCI_MAX_STATIC_THREAD_NUM   100

#define SCI_NULL                    0x0
#define SCI_INVALID_BLOCK_ID        0xFFFFFFFF

// Size of Queue item. Number of unsigned long int.
#define SCI_QUEUE_ITEM_SIZE         1

// Name size of thread, timer, queue, etc... 
#define SCI_MAX_NAME_SIZE           32

// RTOS system state.
#define SCI_RTOS_INITIALIZE_IN_PROGRESS     0
#define SCI_RTOS_INITIALIZE_COMPLETED       1


//define Heap Type 
#define   CONST_HEAP_MEMORY                       0x22222222UL 
#define   DYNAMIC_HEAP_BASE_MEMORY         0x33333333UL 
#define   DYNAMIC_HEAP_APP_MEMORY           0x44444444UL 


#define SCI_TRACE_NULL           SCI_TraceNull
#ifdef WIN32
	extern  void SCI_Trace(const char *, ...);
	#define SCI_TRACE_ERROR      SCI_Trace
	#define SCI_TRACE_HIGH       SCI_Trace
	#define SCI_TRACE_MID        SCI_Trace
	#define SCI_TRACE_LOW        SCI_Trace		
    #define SCI_TRACE_BUF        SCI_TraceBuf
	#define SCI_TRACE_DATA       SCI_TraceCapData
#else
	#define SCI_TRACE_ERROR      SCI_TraceError
	#define SCI_TRACE_HIGH       SCI_TraceHigh
	#define SCI_TRACE_DATA       SCI_TraceCapData
	
    #ifdef TRACE_INFO_SUPPORT 
        #define SCI_TRACE_BUF    SCI_TraceBuf
		#define SCI_TRACE_MID    SCI_TraceMid
		#define SCI_TRACE_LOW    SCI_TraceLow
	#else
        #define SCI_TRACE_BUF(...)
       	#define SCI_TRACE_MID(...)
       	#define SCI_TRACE_LOW(...)
    #endif
#endif  

//---------------------------------------------
// Type define.
//---------------------------------------------
typedef void    *SCI_EVENT_GROUP_PTR;
typedef void    *SCI_THREAD_PTR;
typedef void    *SCI_TIMER_PTR;

//Zhemin.Lin, add, 09/12/2003, CR:MS00004678,
typedef void    *SCI_MUTEX_PTR;
typedef void    *SCI_SEMAPHORE_PTR;



/**---------------------------------------------------------------------------*
 ** MACROES:
 **---------------------------------------------------------------------------*/

	#define SCI_ASSERT(_EXP)    
	#define SCI_PASSERT(_EXP,PRINT_STR) 


	#define SCI_MEMCPY(_DEST_PTR, _SRC_PTR, _SIZE) \
        do  \
        { \
            SCI_ASSERT(PNULL != (_DEST_PTR)); \
            if ((_SIZE)>0) { memcpy((_DEST_PTR), (_SRC_PTR), (_SIZE));} \
        }while(0);

    #define SCI_MEMSET(_DEST_PTR, _VALUE, _SIZE) \
        do\
        {\
            SCI_ASSERT(PNULL != (_DEST_PTR)); \
            if ((_SIZE)>0) { memset((_DEST_PTR), (_VALUE), (_SIZE));  }	\
        }while(0);


 
/**---------------------------------------------------------------------------*
 ** INTERRUPT
 **---------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description:    The function is used to disable IRQ and save old status to
//                  stack. 
//	Global resource dependence: 
//  Author:         lin.liu
//	Note:           
/*****************************************************************************/
PUBLIC void SCI_DisableIRQ(void);

/*****************************************************************************/
//  Description:    The function is used to restore the old IRQ status. 
//	Global resource dependence: 
//  Author:         lin.liu
//	Note:           
/*****************************************************************************/
PUBLIC void SCI_RestoreIRQ(void);




/**---------------------------------------------------------------------------*
 ** TIME MACRO:  
 **---------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description:    This function retrieves the number of milliseconds that 
//                  have elapsed since the system was started.
//	Global resource dependence:
//  Author:         Richard.Yang
//	Note:           The elapsed time is stored as a uint32 value. Therefore, 
//                  the time will wrap around to zero if the system is run 
//                  continuously for 49.7 days. 
/*****************************************************************************/
PUBLIC uint32 SCI_GetTickCount(void);    // Number of ticks.



/*****************************************************************************/
//  Description:    This function put a message to the trace buffer which log
//                  level is no more than LOG_LEVEL_LOW. 
//                  This function put a message to the trace buffer. 
//                  1.msg_trace_info.buf_is_send == TRACE_BUF_SEND
//                    Send the messages to the application tool when buffer 
//                      is full.
//                  2.msg_trace_info.buf_is_send == TRACE_BUF_COVER
//                    Always cover the oldest message when buffer is full.
//	Global resource dependence: 
//  Author: Richard.Yang
//	Note:
/*****************************************************************************/
PUBLIC uint32 SCI_TraceLow(
    const char *x_format, ...);
    






/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif  // End of _OS_API_H
