#include <windows.h>
#include "AgemoDebug.h"
#include "..\win32\resource.h"
#include <stdio.h>
#include "..\libpcsxcore\psxcommon.h"
#include "r3000a.h"
#include "..\libpcsxcore\psxinterpreter.h"

BOOL CALLBACK DbgDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DbgRegsDlgProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI Debugger_WorkThread(LPVOID  lpParameter);
DWORD WINAPI Debugger_RegsWorkThread(LPVOID  lpParameter);

void UI_OnChkLog();
void UI_OnChkGpuUpload();
void UI_OnChkGpuChain();
void UI_OnChkCdromRead();
void UI_OnBtnPause();
void UI_OnChkBootPause();
void UI_OnChkPauseW();
void UI_OnChkAlign();
void UI_OnChkSpuUpload();
unsigned long agemo_w_ram_1=-1;
unsigned long agemo_w_ram_2=-1;
int	agemo_flag_boot_pause = 0;		//default is no pause
int firstinstruction = 0;

void UI_OnChkPauseR();
unsigned long agemo_r_ram_1=-1;
unsigned long agemo_r_ram_2=-1;
unsigned long agemo_memval_addr = -1;
unsigned char agemo_memval_val = -1;

unsigned int agemo_mem_align_check = 0;

char agemo_mem_monitor[0x00200000];

void UI_OnChkPauseOnPC();
void UI_OnChkPauseOnMemVal();
void UI_OnBtnOp();
void UI_OnBtnOpExecTo();
void UI_OnBtnDump();
void UI_OnBtnLoad();
void UI_OnChkRegs();

void __agemo_log_flush();		//declared in PsxInterpreter.c
void __agemo_log_clear();	
void __agemo_pause_cpu(int is_exec_ing);
void __agemo_resume_cpu();
void __agemo_show_next_instruction(int is_exec_ing);
void __agemo_show_regs();
void __agemo_reg_update();
void __agemo_mem_update();
void __agemo_mem_read();
void __agemo_step_next_instruction();

HWND hDbg;
HWND hRegs;
HANDLE hWorkThread;
HANDLE hWorkThreadRegs;
HINSTANCE hCurInst;

// extern vars to control 
extern int agemo_flag_log;
extern int agemo_flag_pause_cpu;
int agemo_paused_disassembly = 1;
int agemo_paused_disassembly_chk = 0;
int agemo_flag_gpu_upload = 0;
int agemo_flag_gpu_chain = 0;
int agemo_flag_cdrom_read = 0;

int agemo_flag_spu_upload = 0;

extern __int64 agemo_ops_break;		//总计执行 xxx 条指令后暂停
extern __int64 agemo_ops_total_exec;
extern u32 agemo_pause_on_pc;
extern u32 agemo_is_pause_resume;
extern HWND hAgemoMainWnd;
extern HANDLE hAgemoMainThread;

//
void Agemo_Doevents()
{


     MSG msg;

     while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
     {
         if(msg.message==WM_QUIT) return ;
         TranslateMessage(&msg);
         DispatchMessage(&msg);
     }
     return ;
}
void Agemo_CreateDbgWndThread(HINSTANCE hInst)
{

	DWORD dwThreadID;
	DWORD dwThreadID2;
	hCurInst = hInst;

	hWorkThread = CreateThread(NULL, 0, Debugger_WorkThread, NULL, CREATE_SUSPENDED, &dwThreadID);
	Sleep(100);	
	ResumeThread(hWorkThread);

	hWorkThreadRegs = CreateThread(NULL, 0, Debugger_RegsWorkThread, NULL, CREATE_SUSPENDED, &dwThreadID2);
	Sleep(100);	
	ResumeThread(hWorkThreadRegs);

}

DWORD WINAPI Debugger_WorkThread(LPVOID  lpParameter)
{
	MSG Msg;

	hDbg = CreateDialog(hCurInst, TEXT("DBG_DLG"), 0, DbgDlgProc);

	if(hDbg == 0)
	{
		MessageBox(0, "Can't create debugger window", "Error", 0);
		return 0;
	}


	while(GetMessage( &Msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hDbg, &Msg) )
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	return Msg.wParam;
}

DWORD WINAPI Debugger_RegsWorkThread(LPVOID  lpParameter)
{
	MSG Msg;

	hRegs = CreateDialog(hCurInst, TEXT("REG_DLG"), 0, DbgRegsDlgProc);

	if(hRegs == 0)
	{
		MessageBox(0, "Can't create registers window", "Error", 0);
		return 0;
	}

	ShowWindow(hRegs, SW_HIDE);

	while(GetMessage( &Msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hRegs, &Msg) )
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	return Msg.wParam;
}

void Agemo_OnReadMem(u32 uMem, int nBytes, u32 rValue)
{
	uMem = uMem & 0x00FFFFFF;
	
	if (agemo_mem_align_check)
	{
		if(nBytes == 2 && uMem % 2 != 0)
		{
			AgemoTrace("CPU Break, non-valid mem address read at %08X, %d bytes", uMem, nBytes);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
			return;
		}
		if(nBytes == 4 && uMem % 4 != 0)
		{
			AgemoTrace("CPU Break, non-valid mem address read at %08X, %d bytes", uMem, nBytes);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
			return;
		}
	}

	//交集的判定
	if (uMem <= agemo_r_ram_2 )
	{
		if( (u32)(uMem + nBytes -1) >= agemo_r_ram_1)
		{
			AgemoTrace("CPU Break, mem read at %08X, %d bytes, value %08X", uMem, nBytes, rValue);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
		}
	}
/*
	if (uMem >= 0x80000000 && uMem <= 0x801FFFFF)
	{
		int i;
		for(i=0;i<nBytes;i++)
			*(agemo_mem_monitor + uMem - 0x80000000)=(unsigned char)0xff;
	}
*/
}
void Agemo_OnWriteMem(u32 uMem, int nBytes, u32 wValue)
{
	uMem = uMem & 0x00FFFFFF;

	if (agemo_mem_align_check)
	{
		if(nBytes == 2 && uMem % 2 != 0)
		{
			AgemoTrace("CPU Break, non-valid mem address read at %08X, %d bytes", uMem, nBytes);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
			return;
		}
		if(nBytes == 4 && uMem % 4 != 0)
		{
			AgemoTrace("CPU Break, non-valid mem address read at %08X, %d bytes", uMem, nBytes);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
			return;
		}
	}


	if (uMem <= agemo_w_ram_2 )
	{
		if ((u32)(uMem + nBytes - 1) >= agemo_w_ram_1)
		{
			AgemoTrace("CPU Break, Mem Write at %08X, %d bytes, value %08X", uMem, nBytes, wValue);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
		}
	}

	if( uMem <= agemo_memval_addr && (u32)(uMem + nBytes) > agemo_memval_addr)
	{
		if((u8)psxM[uMem] == agemo_memval_val) //uMem-0x80000000 seemed to be expecting the virtual, not file offset, at some point in the past. 
		{
			AgemoTrace("CPU Break, Mem address %08X = %02x", uMem, agemo_memval_val);
			if (!agemo_flag_pause_cpu)
				__agemo_pause_cpu(1);
		}
	}
	/*
	//if (uMem < 0x80000000 || uMem > 0x8FFFFFFF){AgemoTrace("w over! %X", uMem); return;}
	if (uMem >= 0x80000000 && uMem <= 0x801FFFFF)
	{
		int i;
		for(i=0;i<nBytes;i++)
			*(agemo_mem_monitor + uMem - 0x80000000)=(unsigned char)0xff;
	}
*/

}

void TraceEditbox(long nItemID,  char *pTxtBuf, long nMaxTruncated)
{
	//nMaxTruncated is ignore.
	static char pEditBuf[65535];
	long nEditBuf;
	long nTxtBuf;
	HWND hItemWnd;

	nTxtBuf = lstrlen(pTxtBuf);

	nEditBuf = GetDlgItemText(hDbg, nItemID, pEditBuf, sizeof(pEditBuf));
	if (nEditBuf + nTxtBuf >= sizeof(pEditBuf))
	{
		//truncated
		strcpy(pEditBuf, "== too long(>65535), auto truncated by UI ==\r\n");
		nEditBuf = strlen(pEditBuf);
	}
	
	sprintf(pEditBuf + nEditBuf, "\r\n%s", pTxtBuf);

	SetDlgItemText(hDbg, nItemID, pEditBuf);
	
	hItemWnd = GetDlgItem(hDbg, nItemID);
	SendMessage(hItemWnd, WM_VSCROLL, SB_BOTTOM, 0);

}
void AgemoTrace(LPCTSTR lpszFormat, ...)
{
	//return;
	char pTxtBuf[4096];
	va_list argList;

	ZeroMemory(pTxtBuf, sizeof(pTxtBuf));

	va_start(argList, lpszFormat);
	wvsprintf(pTxtBuf, lpszFormat, argList);
	va_end(argList);

	TraceEditbox(IDC_ED_INFO, pTxtBuf, 0);

	OutputDebugString(pTxtBuf);
	OutputDebugString("\r\n");
	
}

BOOL CALLBACK DbgDlgProc(
						 HWND hwnd,
						 UINT message,
						 WPARAM wParam,
						 LPARAM lParam
						 )
{
		 
	switch(message)
	{
	case WM_INITDIALOG:
		//SendDlgItemMessage( hDbg, IDC_CHK_LOG, BM_SETCHECK, 0, 0 );

		return TRUE;

	case WM_LBUTTONDOWN:
		
		AgemoTrace("%s", "hi");
		
		return TRUE;
	case WM_RBUTTONDOWN:
		InvalidateRect(hwnd, NULL, 1);
		return TRUE;
	case WM_CLOSE:
		PostQuitMessage(0);
		return TRUE;
	case WM_COMMAND:
		switch LOWORD(wParam)
		{
			case IDC_BTN_FLUSH:
				__agemo_log_flush();
				return TRUE;
			case IDC_BTN_CLEAR_LOG:
				__agemo_log_clear();
				return TRUE;

			case IDC_CHK_GPU_UPLOAD:
				UI_OnChkGpuUpload();
				return TRUE;

			case IDC_CHK_GPU_CHAIN:
				UI_OnChkGpuChain();
				return TRUE;

			case IDC_CHK_CDROM_READ:
				UI_OnChkCdromRead();
				return TRUE;

			case IDC_CHK_LOG:
				UI_OnChkLog();
				return TRUE;
			//case IDC_ED_INFO
			case IDC_BTN_PAUSE:
				UI_OnBtnPause();
				return TRUE;
			//case IDC_ED_OP_NUM
			case IDC_CHK_REGS:
				UI_OnChkRegs();
				return TRUE;

			case IDC_CHK_BOOT_PAUSE:
				UI_OnChkBootPause();
				return TRUE;
			
			case IDC_CHK_PAUSE_WMEM:
				UI_OnChkPauseW();
				return TRUE;

			case IDC_CHK_PAUSE_RMEM:
				UI_OnChkPauseR();
				return TRUE;

			case IDC_CHK_PAUSE_ON_PC:
				UI_OnChkPauseOnPC();
				return TRUE;

			case IDC_CHK_PAUSE_ON_MEMVAL:
				UI_OnChkPauseOnMemVal();
				return TRUE;

			case IDC_CHK_ALIGN:
				UI_OnChkAlign();
				return TRUE;
			case IDC_CHK_SPU_UPLOAD:
				UI_OnChkSpuUpload();
				return TRUE;
				/*
			case IDC_BTN_S:
				PADhandleKey(VK_F1);
				return TRUE;

			case IDC_BTN_L:
				PADhandleKey(VK_F3);
				return TRUE;
				*/

			case IDC_BTN_OP:
				UI_OnBtnOp();
				return TRUE;
			case IDC_BTN_OP_EXEC_TO:
				UI_OnBtnOpExecTo();
				return TRUE;
				
			case IDC_BTN_DUMP:
				UI_OnBtnDump();
				return TRUE;
			case IDC_BTN_LOAD:
				UI_OnBtnLoad();
				return TRUE;
			case IDC_BTN_STEP:
				__agemo_step_next_instruction();
				return TRUE;
		}
		return FALSE;
	default:
		return FALSE;
	}
	return FALSE;
}
BOOL CALLBACK DbgRegsDlgProc(
						 HWND hwnd,
						 UINT message,
						 WPARAM wParam,
						 LPARAM lParam
						 )
{
		 
	switch(message)
	{
	case WM_INITDIALOG:
		//SendDlgItemMessage( hDbg, IDC_CHK_LOG, BM_SETCHECK, 0, 0 );
		return TRUE;
	case WM_CLOSE:
		PostQuitMessage(0);
		return TRUE;
	case WM_COMMAND:
		switch LOWORD(wParam)
		{
			case IDC_BTN_REG_UPDATE:
				__agemo_reg_update();
				__agemo_show_regs();

				return TRUE;
			case IDC_BTN_MEM_UPDATE:
				__agemo_mem_update();
				return TRUE;
			case IDC_BTN_MEM_UPDATE2:
				__agemo_mem_read();
				return TRUE;
		}
		return FALSE;
	default:
		return FALSE;
	}
	return FALSE;
}

void UI_OnChkLog()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_LOG, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("log disabled");
		agemo_paused_disassembly_chk = 0;
	}
	else
	{
		AgemoTrace("log enabled");
		agemo_paused_disassembly_chk = 1;
	}

	agemo_flag_log = n;
}
void UI_OnChkGpuUpload()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_GPU_UPLOAD, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("gpu upload monitor disabled");
	}
	else
	{
		AgemoTrace("gpu upload monitor enabled");
	}

	agemo_flag_gpu_upload = n;
}

void UI_OnChkGpuChain()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_GPU_CHAIN, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("gpu dma chain break disabled");
	}
	else
	{
		AgemoTrace("gpu dma chain break enabled");
	}

	agemo_flag_gpu_chain = n;
}

void UI_OnChkCdromRead()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_CDROM_READ, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("cdrom read monitor disabled");
	}
	else
	{
		AgemoTrace("cdrom read monitor enabled");
	}

	agemo_flag_cdrom_read = n;

}

void __agemo_step_next_instruction()
{
	// SetForegroundWindow(hAgemoMainWnd);
	if (agemo_flag_pause_cpu == 1) {
		execIStep();
		//Sleep(30);
		//SuspendThread(hAgemoMainThread);
		//SetThreadPriority(hAgemoMainThread, THREAD_PRIORITY_IDLE);
		__agemo_show_next_instruction(0);
		__agemo_show_regs();
		//__agemo_update_cpu_button_state();
		__agemo_log_flush();
	}
	else {
		AgemoTrace("debugger must be paused first to step through instructions!");
	}
	//__agemo_update_cpu_button_state();
}

void __agemo_update_cpu_button_state()
{
	if (agemo_flag_pause_cpu)
		SetDlgItemText(hDbg, IDC_BTN_PAUSE, "Resume CPU");
	else
		SetDlgItemText(hDbg, IDC_BTN_PAUSE, "Pause CPU");

}
void __agemo_resume_cpu()
{
	if (!agemo_flag_pause_cpu) {
		MessageBox(hRegs, "Unexpected - you shouldn't be able to resume if the pause flag is not set...", "err", 0);
	}
	else {
		// SetForegroundWindow(hAgemoMainWnd);
		agemo_flag_pause_cpu = 0;
		agemo_is_pause_resume = 1;
		if (!agemo_flag_log) agemo_paused_disassembly_chk = 0;

		AgemoTrace("cpu resume. PC at [%X], ops [%d]", psxRegs.pc, agemo_ops_total_exec);

		//SetThreadPriority(hAgemoMainThread, THREAD_PRIORITY_NORMAL);
		//ResumeThread(hAgemoMainThread);
		__agemo_update_cpu_button_state();
	}
}
void __agemo_pause_cpu(int is_exec_ing)
{
	    // SetForegroundWindow(hAgemoMainWnd);
	if (!agemo_flag_pause_cpu) {
		agemo_paused_disassembly_chk = 1;
		firstinstruction = 1;
		agemo_flag_pause_cpu = 1;
		__agemo_update_cpu_button_state();

		//Sleep(30);

		__agemo_show_next_instruction(is_exec_ing);
		__agemo_show_regs();
		__agemo_update_cpu_button_state();
		__agemo_log_flush();
		//SuspendThread(hAgemoMainThread);
		//SetThreadPriority(hAgemoMainThread, THREAD_PRIORITY_IDLE);
	}
	else {
		agemo_paused_disassembly_chk = 1;
		__agemo_show_next_instruction(is_exec_ing);
		__agemo_show_regs();
		__agemo_update_cpu_button_state();
		__agemo_log_flush();
	}
}
void UI_OnBtnPause()
{

	//AgemoTrace("UI Pause");
	if (agemo_flag_pause_cpu)
		__agemo_resume_cpu();
	else	
		__agemo_pause_cpu(1);
}

void UI_OnChkBootPause()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_BOOT_PAUSE, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("auto pause before load psx-exe disabled");
	}
	else
	{
		AgemoTrace("auto pause before load psx-exe enabled");
	}

	agemo_flag_boot_pause = n;

}

void UI_OnChkRegs()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_REGS, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("regs/mem window disabled");
		ShowWindow(hRegs, SW_HIDE);
	}
	else
	{
		AgemoTrace("regs/mem window enabled");
		ShowWindow(hRegs, SW_SHOW);
	}

	__agemo_show_regs();

}
void UI_OnChkAlign()
{

	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_ALIGN, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("mem r/w alignment check - OFF");
		agemo_mem_align_check = 0;
	}
	else
	{
		AgemoTrace("mem r/w alignment check - ON");
		agemo_mem_align_check = 1;
	}

}

void UI_OnChkSpuUpload()
{
	int n;
	n = SendDlgItemMessage( hDbg, IDC_CHK_SPU_UPLOAD, BM_GETCHECK, 0, 0 );
	if (0 == n) 
	{
		AgemoTrace("spu upload monitor disabled");
	}
	else
	{
		AgemoTrace("spu upload monitor enabled");
	}

	agemo_flag_spu_upload = n;
}



void UI_OnChkPauseW()
{
	int n;
	char pIntBuf[20];

	n = SendDlgItemMessage( hDbg, IDC_CHK_PAUSE_WMEM, BM_GETCHECK, 0, 0 );
	if (1 == n)
	{
		unsigned long v1, v2;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_W_RAM_1, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v1);
			if (strlen(pIntBuf) != 8) v1 = v1 + 0x80000000;
		}
		else
			v1=0x80000000;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_W_RAM_2, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v2);
			if (strlen(pIntBuf) != 8) v2 = v2 + 0x80000000;
		}
		else
			v2=0x80000000;


		if (v2 < v1)v2=v1;

		sprintf(pIntBuf, "%08X", v1);
		SetDlgItemText(hDbg, IDC_ED_W_RAM_1, pIntBuf);

		sprintf(pIntBuf, "%08X", v2);
		SetDlgItemText(hDbg, IDC_ED_W_RAM_2, pIntBuf);
		
		AgemoTrace("wait for pause on write mem from %08X to %08X",  v1, v2);

		agemo_w_ram_1 = v1 & 0x00FFFFFF;
		agemo_w_ram_2 = v2 & 0x00FFFFFF;

	}
	else
	{
		agemo_w_ram_1 = -1;
		agemo_w_ram_2 = -1;
	}
}

void UI_OnChkPauseR()
{
	int n;
	char pIntBuf[20];

	n = SendDlgItemMessage( hDbg, IDC_CHK_PAUSE_RMEM, BM_GETCHECK, 0, 0 );
	if (1 == n)
	{
		unsigned long v1, v2;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_R_RAM_1, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v1);
			if (strlen(pIntBuf) != 8) v1 = v1 + 0x80000000;
		}
		else
			v1=0x80000000;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_R_RAM_2, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v2);
			if (strlen(pIntBuf) != 8) v2 = v2 + 0x80000000;
		}
		else
			v2=0x80000000;

		if (v2 < v1)v2=v1;

		sprintf(pIntBuf, "%08X", v1);
		SetDlgItemText(hDbg, IDC_ED_R_RAM_1, pIntBuf);

		sprintf(pIntBuf, "%08X", v2);
		SetDlgItemText(hDbg, IDC_ED_R_RAM_2, pIntBuf);
		
		AgemoTrace("wait for pause on read  mem from %08X to %08X",  v1, v2);

		agemo_r_ram_1 = v1 & 0x00FFFFFF;
		agemo_r_ram_2 = v2 & 0x00FFFFFF;

	}
	else
	{
		agemo_r_ram_1 = -1;
		agemo_r_ram_2 = -1;
	}
}

void UI_OnChkPauseOnMemVal()
{
	int n;
	char pIntBuf[20];

	n = SendDlgItemMessage( hDbg, IDC_CHK_PAUSE_ON_MEMVAL, BM_GETCHECK, 0, 0 );
	if (1 == n)
	{
		unsigned long v1;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_PAUSE_ON_MEMVAL_ADDR, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v1);
			if (strlen(pIntBuf) != 8) v1 = v1 + 0x80000000;
		}
		else
			v1=0x80000000;

		sprintf(pIntBuf, "%08X", v1);
		SetDlgItemText(hDbg, IDC_ED_PAUSE_ON_MEMVAL_ADDR, pIntBuf);
		
		agemo_memval_addr=v1;

		u32 agemo_memval_addr_display = agemo_memval_addr;

		agemo_memval_addr = v1 & 0x00FFFFFF;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_PAUSE_ON_MEMVAL_VAL, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%02x", &v1);
		}
		else
			v1=0x00;

		sprintf(pIntBuf, "%02X", v1);
		SetDlgItemText(hDbg, IDC_ED_PAUSE_ON_MEMVAL_VAL, pIntBuf);
		
		agemo_memval_val=(unsigned char)v1;

		AgemoTrace("wait for pause on mem address %08X = %02x",  agemo_memval_addr_display, agemo_memval_val);

	}
	else
	{
		agemo_memval_addr=-1;
	}

}

void UI_OnChkPauseOnPC()
{
	int n;
	char pIntBuf[20];

	n = SendDlgItemMessage( hDbg, IDC_CHK_PAUSE_ON_PC, BM_GETCHECK, 0, 0 );
	if (1 == n)
	{
		unsigned long v1;

		memset(pIntBuf, 0, sizeof(pIntBuf));
		GetDlgItemText(hDbg, IDC_ED_PAUSE_ON_PC, pIntBuf, sizeof(pIntBuf) - 1);
		if(strlen(pIntBuf) > 0)
		{
			sscanf(pIntBuf, "%x", &v1);
			if (strlen(pIntBuf) != 8) v1 = v1 + 0x80000000;
		}
		else
			v1=0x80000000;

		sprintf(pIntBuf, "%08X", v1);
		SetDlgItemText(hDbg, IDC_ED_PAUSE_ON_PC, pIntBuf);
		
		agemo_pause_on_pc=v1;

		AgemoTrace("wait for pause on pc %08X",  v1);

	}
	else
	{
		agemo_pause_on_pc=0;
	}
}
void UI_OnBtnOp()
{
	int n;

	n = GetDlgItemInt( hDbg, IDC_ED_OP_NUM, NULL, FALSE);
	if (0 != n)
	{
		agemo_ops_break = agemo_ops_total_exec + n;
		AgemoTrace("running ops count %d", n);
		//AgemoTrace("counter total %d", agemo_ops_total_exec);
		//AgemoTrace("counter next  %d", agemo_ops_break);
		agemo_flag_pause_cpu = 0;
	}
}
void UI_OnBtnOpExecTo()
{
	unsigned long n;

	n = GetDlgItemInt( hDbg, IDC_ED_OP_NUM, NULL, FALSE);
	if (0 != n)
	{
		if (n <= agemo_ops_total_exec)
		{
			AgemoTrace("should > current ops count");
			return;
		}

		agemo_ops_break = n;
		agemo_flag_pause_cpu = 0;

	}
}
void UI_OnBtnDump()
{
	//Dump 

	GPUFreeze_t *gpufP;
//	SPUFreeze_t *spufP;
//	int Size;
	//unsigned char *pMem;
	FILE *f;
	//char hdrBM[54];

	if( agemo_flag_pause_cpu == 0)
	{
		AgemoTrace("first pause cpu and dump");
		return;
	}
	//screen pic
	/*
	SetForegroundWindow(hAgemoMainWnd);

	f = fopen("dump\\screen.bmp", "wb");
	if (f == NULL) return;

	//write Bitmap header
	memset(hdrBM, 0, sizeof(hdrBM));
	hdrBM[0]='B'; hdrBM[1]='M'; hdrBM[3]=0x0c;
	hdrBM[0xA]=0x36; hdrBM[0xE]=0x28; 
	hdrBM[0x12]=(unsigned char)128; hdrBM[0x16]=(unsigned char)96;
	hdrBM[0x1A]=0x1; hdrBM[0x1C]=0x18;
	hdrBM[0x23]=0xc; 
	hdrBM[0x26]=(unsigned char)0xC4; hdrBM[0x27]=0x0E;
	hdrBM[0x2A]=(unsigned char)0xC4; hdrBM[0x2B]=0x0E;
	fwrite(hdrBM, sizeof(hdrBM), 1, f);

	//write bitmap data
	pMem = (unsigned char *) malloc(128*96*3);
	if (pMem == NULL) return ;
	GPU_getScreenPic(pMem);

	fwrite(pMem, 128*96*3, 1, f);
	free(pMem);
	fclose(f);
	*/
	//ram
	f = fopen("dump\\ram.bin", "wb"); //while running in debugger, for some reason it expects dump folder to be in win32 folder (root of project, not debug executable folder as when running the actual executable)
	if (f == NULL) return;
	fwrite(psxM, 0x00200000, 1, f);
	fclose(f);

	//ram monitor
	//f = fopen("dump\\ram_usage.bin", "wb");
	//if (f == NULL) return;
	//fwrite(agemo_mem_monitor, sizeof(agemo_mem_monitor), 1, f);
	//fclose(f);

	//rom (bios)
	f = fopen("dump\\bios.bin", "wb");
	if (f == NULL) return;
	fwrite(psxR, 0x00080000, 1, f);
	fclose(f);

	//parallel port
	//f = fopen("dump\\parallel.bin", "wb");
	//if (f == NULL) return;
	//fwrite(psxP, 0x00010000, 1, f);
	//fclose(f);

	//Hardware registers, and misc.
	//f = fopen("dump\\hw.bin", "wb");
	//if (f == NULL) return;
	//fwrite(psxH, 0x00010000, 1, f);
	//fclose(f);

	// gpu
	
	f = fopen("dump\\vram.bin", "wb");
	if (f == NULL) return;
	gpufP = (GPUFreeze_t *) malloc(sizeof(GPUFreeze_t));
	gpufP->ulFreezeVersion = 1;
	GPU_freeze(1, gpufP);

	fwrite(gpufP->psxVRam, 1024*512*2, 1, f);

	free(gpufP);
	fclose(f);

/*
	// spu
	spufP = (SPUFreeze_t *) malloc(16);
	SPU_freeze(2, spufP);
	Size = spufP->Size; gzwrite(f, &Size, 4);
	free(spufP);
	spufP = (SPUFreeze_t *) malloc(Size);
	SPU_freeze(1, spufP);
	gzwrite(f, spufP, Size);
	free(spufP);

	sioFreeze(f, 1);
	cdrFreeze(f, 1);
	psxHwFreeze(f, 1);
	psxRcntFreeze(f, 1);
	mdecFreeze(f, 1);

	gzclose(f);

*/

	SetForegroundWindow(hDbg);
	AgemoTrace("ram, vram, bios dumped");
}
void UI_OnBtnLoad()
{

	//LOAD RAM/VRAM ONLY 
	//notice that u must pause the CPU first.

	GPUFreeze_t *gpufP;
	FILE *f;

	//ram
	f = fopen("dump\\ram.bin", "rb");
	if (f == NULL) return;
	fread(psxM, 0x00200000, 1, f);
	fclose(f);

	//vram
	f = fopen("dump\\vram.bin", "rb");
	if (f == NULL) return;
	gpufP = (GPUFreeze_t *) malloc (sizeof(GPUFreeze_t));
	fread(gpufP, sizeof(GPUFreeze_t), 1, f);
	GPU_freeze(0, gpufP);
	free(gpufP);

}

void Agemo_OP_Over()
{
	AgemoTrace("PC breakpoint at [%X], ops [%d]", psxRegs.pc, agemo_ops_total_exec);

	__agemo_show_next_instruction(2);
	__agemo_show_regs();
	__agemo_update_cpu_button_state();
	__agemo_log_flush();

}

void __agemo_show_regs()
{
	char pBuf[4096];

	sprintf(pBuf, 
		"a0=%08X a1=%08X a2=%08X a3=%08X \r\n"
		"v0=%08X v1=%08X at=%08X ra=%08X sp=%08X\r\n"
		"t0=%08X t1=%08X t2=%08X t3=%08X t4=%08x\r\n"
		"t5=%08X t6=%08X t7=%08X t8=%08X t9=%08x\r\n"
		"s0=%08X s1=%08X s2=%08X s3=%08X s4=%08x\r\n"
		"s5=%08X s6=%08X s7=%08X s8=%08X \r\n"
		"gp=%08X k0=%08X k1=%08X lo=%08X hi=%08X\r\n"
		"pc=%08X code=%08X cycle=%08X interrupt=%08X\r\n"
		"rt=%08X",
		psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3,
		psxRegs.GPR.n.v0, psxRegs.GPR.n.v1, psxRegs.GPR.n.at, psxRegs.GPR.n.ra, psxRegs.GPR.n.sp,
		psxRegs.GPR.n.t0, psxRegs.GPR.n.t1, psxRegs.GPR.n.t2, psxRegs.GPR.n.t3, psxRegs.GPR.n.t4,
		psxRegs.GPR.n.t5, psxRegs.GPR.n.t6, psxRegs.GPR.n.t7, psxRegs.GPR.n.t8, psxRegs.GPR.n.t9,
		psxRegs.GPR.n.s0, psxRegs.GPR.n.s1, psxRegs.GPR.n.s2, psxRegs.GPR.n.s3, psxRegs.GPR.n.s4,
		psxRegs.GPR.n.s5, psxRegs.GPR.n.s6, psxRegs.GPR.n.s7, psxRegs.GPR.n.s8,
		psxRegs.GPR.n.gp, psxRegs.GPR.n.k0, psxRegs.GPR.n.k1, psxRegs.GPR.n.lo, psxRegs.GPR.n.hi,
		psxRegs.pc, psxRegs.code, psxRegs.cycle, psxRegs.interrupt, psxRegs.GPR.r[_Rt_]
		);

	SetDlgItemText(hRegs, IDC_ED_REGS, pBuf);

	sprintf(pBuf,
		"a0=%08X a1=%08X a2=%08X a3=%08X \r\n"
		"v0=%08X v1=%08X at=%08X ra=%08X sp=%08X\r\n"
		"t0=%08X t1=%08X t2=%08X t3=%08X t4=%08x\r\n"
		"t5=%08X t6=%08X t7=%08X t8=%08X t9=%08x\r\n"
		"s0=%08X s1=%08X s2=%08X s3=%08X s4=%08x\r\n"
		"s5=%08X s6=%08X s7=%08X s8=%08X \r\n"
		"gp=%08X k0=%08X k1=%08X lo=%08X hi=%08X\r\n"
		"pc=%08X code=%08X cycle=%08X interrupt=%08X\r\n"
		"rt=%08X",
		psxRegsPrev.GPR.n.a0, psxRegsPrev.GPR.n.a1, psxRegsPrev.GPR.n.a2, psxRegsPrev.GPR.n.a3,
		psxRegsPrev.GPR.n.v0, psxRegsPrev.GPR.n.v1, psxRegsPrev.GPR.n.at, psxRegsPrev.GPR.n.ra, psxRegsPrev.GPR.n.sp,
		psxRegsPrev.GPR.n.t0, psxRegsPrev.GPR.n.t1, psxRegsPrev.GPR.n.t2, psxRegsPrev.GPR.n.t3, psxRegsPrev.GPR.n.t4,
		psxRegsPrev.GPR.n.t5, psxRegsPrev.GPR.n.t6, psxRegsPrev.GPR.n.t7, psxRegsPrev.GPR.n.t8, psxRegsPrev.GPR.n.t9,
		psxRegsPrev.GPR.n.s0, psxRegsPrev.GPR.n.s1, psxRegsPrev.GPR.n.s2, psxRegsPrev.GPR.n.s3, psxRegsPrev.GPR.n.s4,
		psxRegsPrev.GPR.n.s5, psxRegsPrev.GPR.n.s6, psxRegsPrev.GPR.n.s7, psxRegsPrev.GPR.n.s8,
		psxRegsPrev.GPR.n.gp, psxRegsPrev.GPR.n.k0, psxRegsPrev.GPR.n.k1, psxRegsPrev.GPR.n.lo, psxRegsPrev.GPR.n.hi,
		psxRegsPrev.pc, psxRegsPrev.code, psxRegsPrev.cycle, psxRegsPrev.interrupt, psxRegsPrev.GPR.r[_Rt_]
	);

	SetDlgItemText(hRegs, IDC_ED_REGS2, pBuf);

}

static char pLastDisasm[1024] = "";

void __agemo_update_total_op()
{
	char pBuf[1024];
	sprintf_s(pBuf, sizeof(pBuf), "%d", (int)agemo_ops_total_exec);
	SetDlgItemText(hDbg, IDC_ED_OP_TOTAL, pBuf);

}
void __agemo_show_next_instruction(int is_exec_ing)
{
	char *p;

	//sprintf(pBuf, "%08x", psxRegs.pc);
	//SetDlgItemText(hDbg, IDC_ED_OP_PC, pBuf);

	//sprintf(pBuf, "%08x", psxRegs.code);
	//SetDlgItemText(hDbg, IDC_ED_OP_CODE, pBuf);

	__agemo_update_total_op();

	if (is_exec_ing == 1)
		p = disR3000AF(psxRegs.code, psxRegs.pc-4);
	if (is_exec_ing == 2)
		p = disR3000AF(psxRegs.code, psxRegs.pc);
	else
		p = disR3000AF(psxRegs.code, psxRegs.pc-4);

	strncpy(pLastDisasm, p, sizeof(pLastDisasm)-1);

	if (firstinstruction && pLastDisasm != "") {
		firstinstruction = 0;
		TraceEditbox(IDC_ED_OP, pLastDisasm, 0);
	}

	//显示下一条指令（还未执行）
	SetDlgItemText(hDbg, IDC_ED_OP_DISASM, p);

	//psxRegs.pc
}

void __agemo_reg_update()
{
	char pAdrBuf[1024];
	char pValBuf[1024];

	u32 val;

	memset(pAdrBuf, 0, sizeof(pAdrBuf));
	memset(pValBuf, 0, sizeof(pValBuf));
	GetDlgItemText(hRegs, IDC_ED_REG_NAME, pAdrBuf, sizeof(pAdrBuf) - 1);
	GetDlgItemText(hRegs, IDC_ED_REG_VAL, pValBuf, sizeof(pValBuf) - 1);
	if(strlen(pAdrBuf) < 0) return;
	if(strlen(pValBuf) < 0) return;

	sscanf(pValBuf, "%x", &val);

	if (strncmp(pAdrBuf, "a0", 2) == 0){ psxRegs.GPR.n.a0 = val; return;}
	if (strncmp(pAdrBuf, "a1", 2) == 0){ psxRegs.GPR.n.a1 = val; return;}
	if (strncmp(pAdrBuf, "a2", 2) == 0){ psxRegs.GPR.n.a2 = val; return;}
	if (strncmp(pAdrBuf, "a3", 2) == 0){ psxRegs.GPR.n.a3 = val; return;}

	if (strncmp(pAdrBuf, "v0", 2) == 0){ psxRegs.GPR.n.v0 = val; return;}
	if (strncmp(pAdrBuf, "v1", 2) == 0){ psxRegs.GPR.n.v1 = val; return;}

	if (strncmp(pAdrBuf, "t0", 2) == 0){ psxRegs.GPR.n.t0 = val; return;}
	if (strncmp(pAdrBuf, "t1", 2) == 0){ psxRegs.GPR.n.t1 = val; return;}
	if (strncmp(pAdrBuf, "t2", 2) == 0){ psxRegs.GPR.n.t2 = val; return;}
	if (strncmp(pAdrBuf, "t3", 2) == 0){ psxRegs.GPR.n.t3 = val; return;}
	if (strncmp(pAdrBuf, "t4", 2) == 0){ psxRegs.GPR.n.t4 = val; return;}
	if (strncmp(pAdrBuf, "t5", 2) == 0){ psxRegs.GPR.n.t5 = val; return;}
	if (strncmp(pAdrBuf, "t6", 2) == 0){ psxRegs.GPR.n.t6 = val; return;}
	if (strncmp(pAdrBuf, "t7", 2) == 0){ psxRegs.GPR.n.t7 = val; return;}
	if (strncmp(pAdrBuf, "t8", 2) == 0){ psxRegs.GPR.n.t8 = val; return;}
	if (strncmp(pAdrBuf, "t9", 2) == 0){ psxRegs.GPR.n.t9 = val; return;}

	if (strncmp(pAdrBuf, "s0", 2) == 0){ psxRegs.GPR.n.s0 = val; return;}
	if (strncmp(pAdrBuf, "s1", 2) == 0){ psxRegs.GPR.n.s1 = val; return;}
	if (strncmp(pAdrBuf, "s2", 2) == 0){ psxRegs.GPR.n.s2 = val; return;}
	if (strncmp(pAdrBuf, "s3", 2) == 0){ psxRegs.GPR.n.s3 = val; return;}
	if (strncmp(pAdrBuf, "s4", 2) == 0){ psxRegs.GPR.n.s4 = val; return;}
	if (strncmp(pAdrBuf, "s5", 2) == 0){ psxRegs.GPR.n.s5 = val; return;}
	if (strncmp(pAdrBuf, "s6", 2) == 0){ psxRegs.GPR.n.s6 = val; return;}
	if (strncmp(pAdrBuf, "s7", 2) == 0){ psxRegs.GPR.n.s7 = val; return;}
	if (strncmp(pAdrBuf, "s8", 2) == 0){ psxRegs.GPR.n.s8 = val; return;}

	if (strncmp(pAdrBuf, "at", 2) == 0){ psxRegs.GPR.n.at = val; return;}
	if (strncmp(pAdrBuf, "ra", 2) == 0){ psxRegs.GPR.n.ra = val; return;}
	if (strncmp(pAdrBuf, "sp", 2) == 0){ psxRegs.GPR.n.sp = val; return;}

	if (strncmp(pAdrBuf, "gp", 2) == 0){ psxRegs.GPR.n.gp = val; return;}
	if (strncmp(pAdrBuf, "k0", 2) == 0){ psxRegs.GPR.n.k0 = val; return;}
	if (strncmp(pAdrBuf, "k1", 2) == 0){ psxRegs.GPR.n.k1 = val; return;}
	if (strncmp(pAdrBuf, "lo", 2) == 0){ psxRegs.GPR.n.lo = val; return;}
	if (strncmp(pAdrBuf, "hi", 2) == 0){ psxRegs.GPR.n.hi = val; return;}

	if (strncmp(pAdrBuf, "pc", 2) == 0){ psxRegs.pc = val; return;}
	if (strncmp(pAdrBuf, "code", 4) == 0){ psxRegs.code = val; return;}
	if (strncmp(pAdrBuf, "interrupt", 4) == 0){ psxRegs.interrupt = val; return;}
	if (strncmp(pAdrBuf, "cycle", 4) == 0){ psxRegs.cycle = val; return;}
	if (strncmp(pAdrBuf, "rt", 4) == 0) { psxRegs.GPR.r[_Rt_] = val; return; }

	MessageBox(hRegs, "unknown regs", "err", 0);


}

void __agemo_mem_update()
{
	char pAdrBuf[1024];
	char pValBuf[1024];
	
	u32 addr;
	u32 v;


	memset(pAdrBuf, 0, sizeof(pAdrBuf));
	memset(pValBuf, 0, sizeof(pValBuf));
	GetDlgItemText(hRegs, IDC_ED_MEM_ADDR, pAdrBuf, sizeof(pAdrBuf) - 1);
	GetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf, sizeof(pValBuf) - 1);
	if(strlen(pAdrBuf) < 0) return;
	if(strlen(pValBuf) < 0) return;

	addr=0;
	sscanf(pAdrBuf, "%x", &addr);
	if (strlen(pAdrBuf) != 8) addr = addr + 0x80000000;

	sprintf(pAdrBuf, "%08X", addr);
	SetDlgItemText(hRegs, IDC_ED_MEM_ADDR, pAdrBuf);


	sscanf(pValBuf, "%x", &v);
	if(strlen(pValBuf) == 2) 
	{
		sprintf(pValBuf, "%02X", v);
		SetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf);
		psxMemWrite8(addr, (u8)v);
		return;
	}
	if(strlen(pValBuf) == 4) 
	{
		sprintf(pValBuf, "%04X", v);
		SetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf);
		psxMemWrite16(addr, (u16)v);
		return;
	}
	if(strlen(pValBuf) == 8) 
	{
		sprintf(pValBuf, "%08X", v);
		SetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf);
		psxMemWrite32(addr, (u32)v);
		return;
	}

	MessageBox(hRegs, "incorrect val len, should be 2/4/8", "err", 0);
}

void __agemo_mem_read()
{
	char pAdrBuf[1024];
	char pValBuf[1024];

	u32 addr;
	u32 v;


	memset(pAdrBuf, 0, sizeof(pAdrBuf));
	memset(pValBuf, 0, sizeof(pValBuf));
	GetDlgItemText(hRegs, IDC_ED_MEM_ADDR2, pAdrBuf, sizeof(pAdrBuf) - 1);
	GetDlgItemText(hRegs, IDC_ED_MEM_VAL2, pValBuf, sizeof(pValBuf) - 1);
	if (strlen(pAdrBuf) < 0) return;
	if (strlen(pValBuf) < 0) return;

	addr = 0;
	sscanf(pAdrBuf, "%x", &addr);
	if (strlen(pAdrBuf) != 8) addr = addr + 0x80000000;

	sprintf(pAdrBuf, "%08X", addr);
	SetDlgItemText(hRegs, IDC_ED_MEM_ADDR2, pAdrBuf);


	sscanf(pValBuf, "%x", &v);
	//if (strlen(pValBuf) == 2)
	//{
	//	sprintf(pValBuf, "%02X", v);
	//	SetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf);
	//	psxMemWrite8(addr, (u8)v);
	//	return;
	//}
	//if (strlen(pValBuf) == 4)
	//{
		//sprintf(pValBuf, "%04X", v);
	sprintf(pValBuf, "%x", psxMemRead16(addr));
		SetDlgItemText(hRegs, IDC_ED_MEM_VAL2, pValBuf);
		return;
	//}
	//if (strlen(pValBuf) == 8)
	//{
	//	sprintf(pValBuf, "%08X", v);
	//	SetDlgItemText(hRegs, IDC_ED_MEM_VAL, pValBuf);
	//	psxMemWrite32(addr, (u32)v);
	//	return;
	//}

	//MessageBox(hRegs, "incorrect val len, should be 2/4/8", "err", 0);
}