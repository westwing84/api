#include <stdio.h>
#include <windows.h>		//Windows環境
#include <windowsx.h>		//Windows環境
#include <process.h>		//スレッド用
#include <stdlib.h>

#pragma comment(lib,"winmm.lib")//高精度タイマ

#include "resource.h"		//リソースファイル

//関数宣言
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);	//メインダイアログプロシージャ
BOOL WinInitialize(HINSTANCE hInst, HWND hPaWnd, HMENU chID, LPCTSTR cWinName, HWND PaintArea, WNDPROC WndProc, HWND* hDC);//子ウィンドウを生成
HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	//子ウィンドウプロシージャ
UINT WINAPI TFunc(LPVOID thParam);												//データ読み込み用スレッド
void PaintAxis(HWND hWnd, COLORREF color, COLORREF colorPen);					//軸描画

//定数宣言
#define DEF_APP_NAME	TEXT("Waveform Test")
#define DEF_MUTEX_NAME	DEF_APP_NAME	//ミューテックス名
#define XMIN 30							//波形のx座標の最小値
#define XMAX 700						//波形のx座標の最大値
#define Y_OFFSET 81						//波形のy=0に相当する座標
#define DEF_DATAPERS 61.5				//1秒間に何データ入出力するか

static COLORREF color, colorPen;	//色

//構造体
typedef struct {
	HWND	hwnd;
	HWND	hEdit1;
	HWND	hEdit2;
}SEND_POINTER_STRUCT;

//======================================
//ここからダイアログバージョン

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
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, MainDlgProc);

	return FALSE;			//終了
}

//編集するのはここから

//メインプロシージャ（ダイアログ）
/********************************

システム画面（.rcファイル）に配置したボタン等が押されたときや，
初期化，終了時にどのような処理を行うかをここに記述する．

********************************/
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hWnd1, hWnd2;		//子ウィンドウハンドル
	static HWND hPict1, hPict2;		//ウィンドウハンドル（PictureBox）
	static HANDLE hThread;
	static UINT thID;
	static SEND_POINTER_STRUCT Sps;

	

	switch (uMsg) {
	case WM_INITDIALOG:		//ダイアログ初期化(exeをダブルクリックした時)
		Sps.hwnd = hDlg;
		hPict1 = GetDlgItem(hDlg, IDC_PICTBOX1);
		hPict2 = GetDlgItem(hDlg, IDC_PICTBOX2);
		
		return TRUE;

	case WM_COMMAND:		//ボタンが押された時
		/*******
		case WM_COMMAND:では各ボタンが押されたときの処理を記載します．
	   ********/
		switch (LOWORD(wParam)) {
		case ID_START:			//開始ボタン
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST1"), hPict1, WndProc, &hWnd1); //初期化
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST2"), hPict2, WndProc, &hWnd2);

			Sps.hEdit1 = hWnd1;
			Sps.hEdit2 = hWnd2;

			hThread = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps, 0, &thID);   //_beginthreadex→スレッドを立ち上げる関数	
			EnableWindow(GetDlgItem(hDlg, ID_START), FALSE);

			return TRUE;

		case ID_STOP:				//停止ボタン
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



	case WM_CTLCOLORDLG:	//ダイアログ背景描画
		return ((BOOL)GetStockObject(WHITE_BRUSH));			//ダイアログ背景を白に

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//ダイアログ終了
		return TRUE;
	}

	SendMessage(hWnd1, uMsg, wParam, lParam);
	SendMessage(hWnd2, uMsg, wParam, lParam);

	//オーナー描画後に再描画
	if (uMsg == WM_PAINT) {
		InvalidateRect(hWnd1, NULL, TRUE);	//再描画
		InvalidateRect(hWnd2, NULL, TRUE);
	}

	return FALSE;
}

/********************************

ここでPictureControlの描画を行います．

子ウィンドウプロシージャ中のhWndはPictureControlのハンドルです．

********************************/
//子ウィンドウプロシージャ
HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		colorPen = RGB(255, 255, 255);	//色指定
		color = RGB(0, 0, 0);
		
		break;
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
//子ウィンドウ初期化＆生成
//指定したウィンドウハンドルの領域に子ウィンドウを生成する．
//----------------------------------------------------------
// hInst	: 生成用インスタンスハンドル
// hPaWnd	: 親ウィンドウのハンドル
// chID		: 子ウィンドウのID
// cWinName	: 子ウィンドウ名
// PaintArea: 子ウィンドウを生成する領域のデバイスハンドル
// WndProc	: ウィンドウプロシージャ
// *hWnd	: 子ウィンドウのハンドル（ポインタ）
// 戻り値	: 成功時=true
//-----------------------------------------------------------------------------
BOOL WinInitialize(HINSTANCE hInst, HWND hPaWnd, HMENU chID, LPCTSTR cWinName, HWND PaintArea, WNDPROC WndProc, HWND* hWnd)
{
	WNDCLASS wc;			//ウィンドウクラス
	WINDOWPLACEMENT	wplace;	//子ウィンドウ生成領域計算用（画面上のウィンドウの配置情報を格納する構造体）
	RECT WinRect;			//子ウィンドウ生成領域
	ATOM atom;				//アトム

	//ウィンドウクラス初期化
	wc.style = CS_HREDRAW ^ WS_MAXIMIZEBOX | CS_VREDRAW;	//ウインドウスタイル
	wc.lpfnWndProc = WndProc;									//ウインドウのメッセージを処理するコールバック関数へのポインタ
	wc.cbClsExtra = 0;											//
	wc.cbWndExtra = 0;
	wc.hCursor = NULL;										//プログラムのハンドル
	wc.hIcon = NULL;										//アイコンのハンドル
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);		//ウインドウ背景色
	wc.hInstance = hInst;										//ウインドウプロシージャがあるインスタンスハンドル
	wc.lpszMenuName = NULL;										//メニュー名
	wc.lpszClassName = (LPCTSTR)cWinName;									//ウインドウクラス名

	if (!(atom = RegisterClass(&wc))) {
		MessageBox(hPaWnd, TEXT("ウィンドウクラスの生成に失敗しました．"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	GetWindowPlacement(PaintArea, &wplace);	//描画領域ハンドルの情報を取得(ウィンドウの表示状態を取得)
	WinRect = wplace.rcNormalPosition;		//描画領域の設定

	//ウィンドウ生成
	*hWnd = CreateWindow(
		(LPCTSTR)atom,
		(LPCTSTR)cWinName,
		WS_CHILD | WS_VISIBLE,//| WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME |WS_VISIBLE, 
		WinRect.left, WinRect.top,
		WinRect.right - WinRect.left, WinRect.bottom - WinRect.top,
		hPaWnd, chID, hInst, NULL
	);

	if (*hWnd == NULL) {
		MessageBox(hPaWnd, TEXT("ウィンドウの生成に失敗しました．"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

//データ読み込み用スレッド
UINT WINAPI TFunc(LPVOID thParam)
{
	static SEND_POINTER_STRUCT* FU = (SEND_POINTER_STRUCT*)thParam;        //構造体のポインタ取得

	FILE* fp;			//ファイルポインタ
	BOOL Flag = TRUE;		//ループフラグ
	double data[2], prevdata[2];		//データ
	DWORD DNum = 0, beforeTime;
	DWORD nowTime, progress, idealTime;
	int time = 0;
	HDC	hdc1, hdc2;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps1, ps2;					//(構造体)クライアント領域描画するための情報	
	HPEN hPen1, hPen2;			//ペン
	
	beforeTime = timeGetTime();						//現在の時刻計算（初期時間）

	
	//ファイルオープン
	if ((fopen_s(&fp, "data.txt", "r")) != 0) {
		MessageBox(NULL, TEXT("ファイルを開けませんでした．"), NULL, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	//データ読み込み・波形描画
	int x = 0;									//x座標
	colorPen = RGB(163, 199, 230);				//線の色
	hdc1 = BeginPaint(FU->hEdit1, &ps1);
	hdc2 = BeginPaint(FU->hEdit2, &ps2);
	hPen1 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc1, hPen1);
	hPen2 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc2, hPen2);

	colorPen = RGB(255, 255, 255);
	color = RGB(0, 0, 0);
	InvalidateRect(FU->hEdit1, NULL, TRUE);
	PaintAxis(FU->hEdit1, color, colorPen);
	InvalidateRect(FU->hEdit2, NULL, TRUE);
	PaintAxis(FU->hEdit2, color, colorPen);


	while (Flag == TRUE) {
		//時間の調整
		nowTime = timeGetTime();					//現在の時刻計算
		progress = nowTime - beforeTime;				//処理時間を計算
		idealTime = (DWORD)(DNum * (1000.0 / (double)DEF_DATAPERS));	//理想時間を計算
		if (idealTime > progress) {
			Sleep(idealTime - progress);			//理想時間になるまで待機
		}
		
		//データの読み込み
		if (fscanf_s(fp, "%lf\t%lf", &(data[0]), &(data[1])) == EOF) {
			MessageBox(NULL, TEXT("終了"), TEXT("INFORMATION"), MB_OK | MB_ICONEXCLAMATION);
			EnableWindow(GetDlgItem(FU->hwnd, ID_START), TRUE);		//開始ボタン有効
			Flag = FALSE;												//ループ終了フラグ
			return FALSE;
		}

		DNum++;
		//一秒経過時
		if (progress >= 1000.0) {
			beforeTime = nowTime;
			DNum = 0;
		}

		if (x == 0) {
			prevdata[0] = data[0];
			prevdata[1] = data[1];
			x += 1;
			continue;
		}

		//波形描画
		MoveToEx(hdc1, XMIN + x - 1, (int)(60 * (-prevdata[0]) + Y_OFFSET), NULL);
		LineTo(hdc1, XMIN + x, (int)(60 * (-data[0]) + Y_OFFSET));
		MoveToEx(hdc2, XMIN + x - 1, (int)(60 * (-prevdata[1]) + Y_OFFSET), NULL);
		LineTo(hdc2, XMIN + x, (int)(60 * (-data[1]) + Y_OFFSET));
		prevdata[0] = data[0];
		prevdata[1] = data[1];
		if (x + XMIN >= XMAX) {
			color = RGB(0, 0, 0);
			colorPen = RGB(255, 255, 255);
			InvalidateRect(FU->hEdit1, NULL, TRUE);
			PaintAxis(FU->hEdit1, color, colorPen);
			InvalidateRect(FU->hEdit2, NULL, TRUE);
			PaintAxis(FU->hEdit2, color, colorPen);
			x = 0;
		}
		x += 1;
	}
	EndPaint(FU->hEdit1, &ps1);
	EndPaint(FU->hEdit2, &ps2);
	DeleteObject(hPen1);
	DeleteObject(hPen2);


	return 0;
}

//PictureBox内にx軸とy軸を描画
void PaintAxis(HWND hWnd, COLORREF color, COLORREF colorPen) {
	HDC			hdc;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps;					//(構造体)クライアント領域描画するための情報	
	HBRUSH		hBrush;				//ブラシ
	HPEN		hPen;				//ペン
	static RECT rect;
	/********************************

		PictureControlに描画するためには，HDC型のハンドルを別に取得する
		必要があります．

		例：hdc = BeginPaint(hWnd, &ps);
		hdc:デバイスコンテキストのハンドル
		hWnd:PictureControlのハンドル
		ps：(構造体)クライアント領域描画するための情報

		********************************/

	hdc = BeginPaint(hWnd, &ps);//デバイスコンテキストのハンドル取得

	/********************************

	PictureControlに描画するためには，線を引きたいときはペン，
	塗りつぶす際にはブラシが必要です．

	********************************/

	color = RGB(0, 0, 0);
	colorPen = RGB(255, 255, 255);
	//ペン，ブラシ生成
	hBrush = CreateSolidBrush(color);				//ブラシ生成
	SelectObject(hdc, hBrush);						//ブラシ設定
	hPen = CreatePen(PS_SOLID, 2, colorPen);		//ペン生成
	SelectObject(hdc, hPen);						//ペン設定

	//描画
	/********************************

	図形を描画するためには以下の関数を用います．
	長方形：Rectangle(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);
	円：Ellipse(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);

	 nLiftRect：長方形の左上X座標
	  nTopRect：左上Y座標
	  nRightRect：右下X座標
	  nBottomRect：右下のY座標

	線を引くには以下の関数を用います．

	線の始点設定：MoveToEx(HDC hdc , int X , int Y , NULL);
	  X,Y：線の始点の座標
	線；LineTo(HDC hdc , int nXEnd , int nYEnd);
	  nXEnd, nYEnd：線の終点の設定


	  以上を参考に図形を描画する関数を以下に記述しましょう
	********************************/
	//ここから
	GetClientRect(hWnd, &rect);
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);	//背景を黒に塗りつぶす
	MoveToEx(hdc, XMIN, Y_OFFSET, NULL);
	LineTo(hdc, XMAX, Y_OFFSET);									//x軸描画
	MoveToEx(hdc, XMIN, 10, NULL);
	LineTo(hdc, XMIN, 150);											//y軸描画


	//ここまで
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, 350, 140, TEXT("Time [s]"), 8);		//テキスト描画

	//デバイスコンテキストのハンドル破棄
	EndPaint(hWnd, &ps);

	//ペン，ブラシ廃棄
	/********************************

	使い終わったペンとブラシは破棄する必要があります．

	********************************/
	DeleteObject(hBrush);
	DeleteObject(hPen);

}