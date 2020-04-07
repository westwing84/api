#include <windows.h>		//Windows環境
#include <windowsx.h>		//Windows環境
#include <stdio.h>			//入出力用
#include <process.h>		//スレッド用
#include <stdlib.h>

#pragma comment(lib,"winmm.lib")//高精度タイマ

#include "resource.h"		//リソースファイル
#include "Header.h"		//リソースファイル

//構造体
typedef struct {
	HWND	hwnd;
	HWND	hEdit;
}SEND_POINTER_STRUCT;

//メイン関数(ダイアログバージョン)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HANDLE hMutex;

	//多重起動判定
	hMutex = CreateMutex(NULL, TRUE, DEF_MUTEX_NAME);		//ミューテックスオブジェクトの生成
	if (GetLastError() == ERROR_ALREADY_EXISTS) {				//2重起動の有無を確認
		MessageBox(NULL, TEXT("既に起動されています．"), NULL, MB_OK | MB_ICONERROR);
		return 0;											//終了
	}

	//ダイアログ起動
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainDlgProc);

	return FALSE;			//終了
}

//メインプロシージャ（ダイアログ）
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont;				//フォント
	static HANDLE hThread;
	static UINT thID;
	static SEND_POINTER_STRUCT Sps;
	static HWND hEdit;

	switch (uMsg) {
	case WM_INITDIALOG:		//ダイアログ初期化
		Sps.hwnd = hDlg;
		hEdit = GetDlgItem(hDlg, IDC_EDIT1);
		Sps.hEdit = hEdit;
		return TRUE;

	case WM_COMMAND:		//ボタンが押された時
		switch (LOWORD(wParam)) {
		case ID_START:			//OKボタン
								//データ読み込みスレッド起動
			hThread = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps, 0, &thID);   //_beginthreadex→スレッドを立ち上げる関数	
			EnableWindow(GetDlgItem(hDlg, ID_START), FALSE);						//開始ボタン無効化　　　　//EnableWindowで入力を無効または有効にする。

			return TRUE;

		case ID_STOP:	//停止・再開ボタン

						/*　サスペンドカウンタ　**************************
						　　　実行を許可するまでスレッドを動かさない。
						   　　ResumeThread：　サスペンドカウンタを1減らす
							 　SuspendThread：　サスペンドカウンタを1増やす

							  0のときは実行。それ以外は待機する。
							  **************************************************/


			if (ResumeThread(hThread) == 0) {					//停止中かを調べる(サスペンドカウントを１減らす)
				SetDlgItemText(hDlg, ID_STOP, TEXT("再開"));	//再開に変更　　　　　　　　　　　　　　　　　　　//SetDlgItemTextでダイアログ内のテキストなどを変更することができる
				SuspendThread(hThread);						//スレッドの実行を停止(サスペンドカウントを１増やす)
			}
			else
				SetDlgItemText(hDlg, ID_STOP, TEXT("停止"));	//停止に変更

			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//ダイアログ終了
		return TRUE;
	}

	return FALSE;
}

//データ読み込み用スレッド
UINT WINAPI TFunc(LPVOID thParam)
{
	static SEND_POINTER_STRUCT *FU = (SEND_POINTER_STRUCT*)thParam;        //構造体のポインタ取得

	FILE *fp;			//ファイルポインタ
	BOOL Flag = TRUE;		//ループフラグ
	HFONT hFont;		//フォント

	double data;		//データ
	char str[256];		//データ
	char str2[256];     //分数用
	wchar_t wcs1[256];
	wchar_t wcs2[256];
	size_t ret;

	/*　wchar_t型　***********************************************
	char型はLPCWS型と互換性がないためそのままだと文字化けする。
	mbstowcs_s関数を用いて、charからwcharに変換する必要がある。
	***************************************************************/


	DWORD DNum = 0, beforeTime;

	int time = 0, min = 0;

	//エディタのフォント変更(case WM_INITDIALOG:の中で設定しても良い)
	hFont = CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_ROMAN, NULL);
	SendMessage(FU->hEdit, WM_SETFONT, (int)hFont, TRUE);

	beforeTime = timeGetTime();						//現在の時刻計算（初期時間）

													//ファイルオープン
	if ((fp = fopen("data.txt", "r")) == NULL) {
		MessageBox(NULL, TEXT("Input File Open ERROR!"), NULL, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	//データ読み込み・表示
	while (Flag == TRUE) {
		DWORD nowTime, progress, idealTime;

		//時間の調整
		nowTime = timeGetTime();					//現在の時刻計算
		progress = nowTime - beforeTime;				//処理時間を計算
		idealTime = (DWORD)(DNum * (1000.0 / (double)DEF_DATAPERS));	//理想時間を計算
		if (idealTime > progress){
			Sleep(idealTime - progress);			//理想時間になるまで待機
		}
																		//データの読み込み
		if (fscanf(fp, "%lf", &data) == EOF) {
			MessageBox(NULL, TEXT("終了"), TEXT("INFORMATION"), MB_OK | MB_ICONEXCLAMATION);
			EnableWindow(GetDlgItem(FU->hwnd, ID_START), TRUE);		//開始ボタン有効
			Flag = FALSE;												//ループ終了フラグ
			return FALSE;
		}

		//表示
		_gcvt(data, 8, str);								//double型を文字列に変換
		//_gcvt(idealTime, 8, str);								//double型を文字列に変換
		mbstowcs_s(&ret, wcs1, 256, str, _TRUNCATE);        //charからwcharに変換する
		SetDlgItemText(FU->hwnd, IDC_EDIT1, wcs1);				//エディタに表示

		DNum++;

		//一秒経過時
		if (progress >= 1000.0) {
			beforeTime = nowTime;
			DNum = 0;
		}

		//秒数表示
		time++;
		_itoa((int)(time / (int)DEF_DATAPERS), str, 10);	//秒数(int型)を文字列に変換
		mbstowcs_s(&ret, wcs1, 256, str, _TRUNCATE);        //charからwcharに変換する
		SetDlgItemText(FU->hwnd, IDC_STATIC3, wcs1);			//表示


																//60秒経過したとき
		if (time == 60000) {
			min++;                                          //分表示に切り替える
			time = 0;
			itoa((int)(time / (int)DEF_DATAPERS), str, 10);	//秒数(int型)を文字列に変換
			mbstowcs_s(&ret, wcs1, 256, str, _TRUNCATE);    //charからwcharに変換する
			SetDlgItemText(FU->hwnd, IDC_STATIC3, wcs1);			//0と表示
			itoa((int)(min), str2, 10);	                 //分数(int型)を文字列に変換
			mbstowcs_s(&ret, wcs2, 256, str2, _TRUNCATE);    //charからwcharに変換する
			SetDlgItemText(FU->hwnd, IDC_STATIC1, wcs2);       //分数を画面表示

		}
	}

	return 0;
}