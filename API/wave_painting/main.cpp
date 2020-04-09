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
HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);		//子ウィンドウプロシージャ

//定数宣言
#define DEF_APP_NAME	TEXT("Waveform Test")
#define DEF_MUTEX_NAME	DEF_APP_NAME			//ミューテックス名
#define DTMAX 8192

static COLORREF	color, colorPen;	//色
static FILE *fp;

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
	static HWND hPict1, hPict2;		//ウィンドウハンドル（PictureBox）
	static HWND hWnd;		//子ウィンドウハンドル
	static HWND hStart;

	switch (uMsg) {
	case WM_INITDIALOG:		//ダイアログ初期化(exeをダブルクリックした時)


		/*******
		システム画面（.rcファイル）に自分で追加したボタンやPicture Control等
		は必ずHWND型でGetDlgItem関数を用いてハンドルを取得します．

		例：hWnd = GetDlgItem(hDlg, ID);
		hWnd：取得したいハンドル
		hDlg：ダイアログのハンドル．ここは基本的にそのまま
		ID：取得したいハンドルの対象となるボタンやPicture Control等
			のID．.rcファイルから確認できる．詳細は配布資料を参考



		ここでヘッダを見てみると，hRadioR・hRadioG・hRadioBというハンドルが宣言
		されています．
		これをそれぞれシステム画面上のラジオボタン赤・緑・青に対応するように
		宣言してみましょう．
	   ********/
	   //ここから



		//ここまで

		SendMessage(hDlg, WM_COMMAND, 0, 0);	//
		return TRUE;

	case WM_COMMAND:		//ボタンが押された時
		/*******
		case WM_COMMAND:では各ボタンが押されたときの処理を記載します．
	   ********/
		switch (LOWORD(wParam)) {
		case ID_START:			//開始ボタン
			/***********************
			OKボタンが押されたときに描画を開始します．
			描画を開始するためにはPictureControlのハンドルを取得する必要があります
			↑を参考にハンドル hPictをGetDlgItem関数を用いて取得しましょう．
			***********************/
			//ここから
			hStart = GetDlgItem(hDlg, ID_START);
			EnableWindow(hStart, FALSE);
			hPict1 = GetDlgItem(hDlg, IDC_PICTBOX1);
			hPict2 = GetDlgItem(hDlg, IDC_PICTBOX2);



			//ここまで	
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST1"), hPict1, WndProc, &hWnd); //初期化
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST2"), hPict2, WndProc, &hWnd);
			return TRUE;

		case ID_END:		//キャンセルボタン
			EndDialog(hDlg, 0);		//ダイアログ終了
			return TRUE;

		}
		break;



	case WM_CTLCOLORDLG:	//ダイアログ背景描画
		return ((BOOL)GetStockObject(WHITE_BRUSH));			//ダイアログ背景を白に

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//ダイアログ終了
		return TRUE;
	}

	SendMessage(hWnd, uMsg, wParam, lParam);

	//オーナー描画後に再描画
	if (uMsg == WM_PAINT) {
		InvalidateRect(hWnd, NULL, TRUE);	//再描画
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

	HDC			hdc;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps;					//(構造体)クライアント領域描画するための情報	
	HBRUSH		hBrush, hOldBrush;	//ブラシ
	HPEN		hPen, hOldPen;		//ペン

	switch (uMsg) {
	case WM_CREATE:
		colorPen = RGB(255, 255, 255);	//色指定
		break;

	case WM_PAINT:

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

		//ペン，ブラシ生成
		hBrush = CreateSolidBrush(color);				//ブラシ生成
		hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);	//ブラシ設定
		hPen = CreatePen(PS_SOLID, 2, colorPen);		//ペン生成
		hOldPen = (HPEN)SelectObject(hdc, hPen);		//ペン設定

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
		MoveToEx(hdc, 30, 75, NULL);
		LineTo(hdc, 700, 75);
		MoveToEx(hdc, 30, 10, NULL);
		LineTo(hdc, 30, 150);



		//ここまで
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);
		TextOut(hdc, 350, 140, TEXT("Time [s]"), 8);		//テキスト描画

		//ペン，ブラシ廃棄
		/********************************

		使い終わったペンとブラシは破棄する必要があります．

		********************************/
		SelectObject(hdc, hOldBrush);
		DeleteObject(hBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);

		//デバイスコンテキストのハンドル破棄
		EndPaint(hWnd, &ps);
		break;
	}

	return TRUE;
}

//ここまで

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
