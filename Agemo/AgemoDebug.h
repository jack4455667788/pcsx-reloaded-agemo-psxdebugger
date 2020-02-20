#include "..\libpcsxcore\psxcommon.h"

#ifndef __AGEMO_DEBUGGER_H__
#define __AGEMO_DEBUGGER_H__

void Agemo_CreateDbgWndThread(HINSTANCE hInst); 
void Agemo_OP_Over();
void Agemo_Doevents();
void Agemo_OnReadMem(u32 uAddr, int nBytes, u32 rValue);
void Agemo_OnWriteMem(u32 uAddr, int nBytes, u32 wValue);
void TraceEditbox(long nItemID, char *pTxtBuf, long nMaxTruncated);
void __agemo_update_cpu_button_state();
void __agemo_update_total_op();
void AgemoTrace(LPCTSTR lpszFormat, ...);

void __agemo_pause_cpu(int is_exec_ing);

void UI_OnBtnDump();
void Agemo_OnGpuDmaChain(unsigned long *, unsigned long);

#endif // __Agemo_DEBUGGER_H__
