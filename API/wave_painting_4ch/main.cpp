#include <stdio.h>
#include <windows.h>		//Windows��
#include <windowsx.h>		//Windows��
#include <process.h>		//�X���b�h�p
#include <stdlib.h>

#pragma comment(lib,"winmm.lib")//�����x�^�C�}

#include "resource.h"		//���\�[�X�t�@�C��

//�֐��錾
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);	//���C���_�C�A���O�v���V�[�W��
BOOL WinInitialize(HINSTANCE hInst, HWND hPaWnd, HMENU chID, LPCTSTR cWinName, HWND PaintArea, WNDPROC WndProc, HWND* hDC);//�q�E�B���h�E�𐶐�
HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	//�q�E�B���h�E�v���V�[�W��
UINT WINAPI TFunc(LPVOID thParam);												//�f�[�^�ǂݍ��ݗp�X���b�h
void PaintAxis(HWND hWnd, COLORREF color, COLORREF colorPen);					//���`��

//�萔�錾
#define DEF_APP_NAME	TEXT("Waveform Test")
#define DEF_MUTEX_NAME	DEF_APP_NAME	//�~���[�e�b�N�X��
#define XMIN 20							//�`��̈��x���W�̍ŏ��l
#define XMAX 580						//�`��̈��x���W�̍ő�l
#define YMAX 130						//�`��̈��y���W�̍ő�l
#define Y_OFFSET 136					//�`��̈��y=0�ɑ���������W
#define DTMAX 1.8						//�f�[�^�̍ő�l
#define DEF_DATAPERS 300				//1�b�Ԃɉ��f�[�^���o�͂��邩

static COLORREF color, colorPen;	//�F

//�\����
typedef struct {
	HWND	hwnd;
	HWND	hEdit1;
	HWND	hEdit2;
	HWND	hEdit3;
	HWND	hEdit4;
}SEND_POINTER_STRUCT;

//======================================
//��������_�C�A���O�o�[�W����

//���C���֐�(�_�C�A���O�o�[�W����)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HANDLE hMutex;

	//���d�N������
	hMutex = CreateMutex(NULL, TRUE, DEF_MUTEX_NAME);		//�~���[�e�b�N�X�I�u�W�F�N�g�̐���
	if (GetLastError() == ERROR_ALREADY_EXISTS) {				//2�d�N���̗L�����m�F
		MessageBox(NULL, TEXT("���ɋN������Ă��܂��D"), NULL, MB_OK | MB_ICONERROR);
		return 0;											//�I��
	}

	//�_�C�A���O�N��
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, MainDlgProc);

	return FALSE;			//�I��
}

//�ҏW����̂͂�������

//���C���v���V�[�W���i�_�C�A���O�j
/********************************

�V�X�e����ʁi.rc�t�@�C���j�ɔz�u�����{�^�����������ꂽ�Ƃ���C
�������C�I�����ɂǂ̂悤�ȏ������s�����������ɋL�q����D

********************************/
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hWnd1, hWnd2, hWnd3, hWnd4;		//�q�E�B���h�E�n���h��
	static HWND hPict1, hPict2, hPict3, hPict4;		//�E�B���h�E�n���h���iPictureBox�j
	static HANDLE hThread;
	static UINT thID;
	static SEND_POINTER_STRUCT Sps;

	

	switch (uMsg) {
	case WM_INITDIALOG:		//�_�C�A���O������(exe���_�u���N���b�N������)
		Sps.hwnd = hDlg;
		hPict1 = GetDlgItem(hDlg, IDC_PICTBOX1);
		hPict2 = GetDlgItem(hDlg, IDC_PICTBOX2);
		hPict3 = GetDlgItem(hDlg, IDC_PICTBOX3);
		hPict4 = GetDlgItem(hDlg, IDC_PICTBOX4);

		return TRUE;

	case WM_COMMAND:		//�{�^���������ꂽ��
		/*******
		case WM_COMMAND:�ł͊e�{�^���������ꂽ�Ƃ��̏������L�ڂ��܂��D
	   ********/
		switch (LOWORD(wParam)) {
		case ID_START:			//�J�n�{�^��

			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST1"), hPict1, WndProc, &hWnd1); //������
			Sps.hEdit1 = hWnd1;
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST2"), hPict2, WndProc, &hWnd2);
			Sps.hEdit2 = hWnd2;
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST3"), hPict3, WndProc, &hWnd3);
			Sps.hEdit3 = hWnd3;
			WinInitialize(NULL, hDlg, (HMENU)110, TEXT("TEST4"), hPict4, WndProc, &hWnd4);
			Sps.hEdit4 = hWnd4;
	
			hThread = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps, 0, &thID);   //_beginthreadex���X���b�h�𗧂��グ��֐�
			EnableWindow(GetDlgItem(hDlg, ID_START), FALSE);

			return TRUE;

		case ID_STOP:				//��~�{�^��
			/*�@�T�X�y���h�J�E���^�@**************************
						�@�@�@���s��������܂ŃX���b�h�𓮂����Ȃ��B
						   �@�@ResumeThread�F�@�T�X�y���h�J�E���^��1���炷
							 �@SuspendThread�F�@�T�X�y���h�J�E���^��1���₷

							  0�̂Ƃ��͎��s�B����ȊO�͑ҋ@����B
							  **************************************************/


			if (ResumeThread(hThread) == 0) {					//��~�����𒲂ׂ�(�T�X�y���h�J�E���g���P���炷)
				SetDlgItemText(hDlg, ID_STOP, TEXT("�ĊJ"));	//�ĊJ�ɕύX
				SuspendThread(hThread);						//�X���b�h�̎��s���~(�T�X�y���h�J�E���g���P���₷)
			}
			else
				SetDlgItemText(hDlg, ID_STOP, TEXT("��~"));	//��~�ɕύX

			return TRUE;
		}
		break;



	case WM_CTLCOLORDLG:	//�_�C�A���O�w�i�`��
		return ((BOOL)GetStockObject(WHITE_BRUSH));			//�_�C�A���O�w�i�𔒂�

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//�_�C�A���O�I��
		return TRUE;
	}
	SendMessage(hWnd1, uMsg, wParam, lParam);
	SendMessage(hWnd2, uMsg, wParam, lParam);
	SendMessage(hWnd3, uMsg, wParam, lParam);
	SendMessage(hWnd4, uMsg, wParam, lParam);

	//�I�[�i�[�`���ɍĕ`��
	if (uMsg == WM_PAINT) {
		InvalidateRect(hWnd1, NULL, TRUE);	//�ĕ`��
		InvalidateRect(hWnd2, NULL, TRUE);
		InvalidateRect(hWnd3, NULL, TRUE);
		InvalidateRect(hWnd4, NULL, TRUE);
	}

	return FALSE;
}

/********************************

������PictureControl�̕`����s���܂��D

�q�E�B���h�E�v���V�[�W������hWnd��PictureControl�̃n���h���ł��D

********************************/
//�q�E�B���h�E�v���V�[�W��
HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		colorPen = RGB(255, 255, 255);	//�F�w��
		color = RGB(0, 0, 0);
		
		break;

	}

	return TRUE;
}


//-----------------------------------------------------------------------------
//�q�E�B���h�E������������
//�w�肵���E�B���h�E�n���h���̗̈�Ɏq�E�B���h�E�𐶐�����D
//----------------------------------------------------------
// hInst	: �����p�C���X�^���X�n���h��
// hPaWnd	: �e�E�B���h�E�̃n���h��
// chID		: �q�E�B���h�E��ID
// cWinName	: �q�E�B���h�E��
// PaintArea: �q�E�B���h�E�𐶐�����̈�̃f�o�C�X�n���h��
// WndProc	: �E�B���h�E�v���V�[�W��
// *hWnd	: �q�E�B���h�E�̃n���h���i�|�C���^�j
// �߂�l	: ������=true
//-----------------------------------------------------------------------------
BOOL WinInitialize(HINSTANCE hInst, HWND hPaWnd, HMENU chID, LPCTSTR cWinName, HWND PaintArea, WNDPROC WndProc, HWND* hWnd)
{
	WNDCLASS wc;			//�E�B���h�E�N���X
	WINDOWPLACEMENT	wplace;	//�q�E�B���h�E�����̈�v�Z�p�i��ʏ�̃E�B���h�E�̔z�u�����i�[����\���́j
	RECT WinRect;			//�q�E�B���h�E�����̈�
	ATOM atom;				//�A�g��

	//�E�B���h�E�N���X������
	wc.style = CS_HREDRAW ^ WS_MAXIMIZEBOX | CS_VREDRAW;	//�E�C���h�E�X�^�C��
	wc.lpfnWndProc = WndProc;									//�E�C���h�E�̃��b�Z�[�W����������R�[���o�b�N�֐��ւ̃|�C���^
	wc.cbClsExtra = 0;											//
	wc.cbWndExtra = 0;
	wc.hCursor = NULL;										//�v���O�����̃n���h��
	wc.hIcon = NULL;										//�A�C�R���̃n���h��
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);		//�E�C���h�E�w�i�F
	wc.hInstance = hInst;										//�E�C���h�E�v���V�[�W��������C���X�^���X�n���h��
	wc.lpszMenuName = NULL;										//���j���[��
	wc.lpszClassName = (LPCTSTR)cWinName;									//�E�C���h�E�N���X��

	if (!(atom = RegisterClass(&wc))) {
		MessageBox(hPaWnd, TEXT("�E�B���h�E�N���X�̐����Ɏ��s���܂����D"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	GetWindowPlacement(PaintArea, &wplace);	//�`��̈�n���h���̏����擾(�E�B���h�E�̕\����Ԃ��擾)
	WinRect = wplace.rcNormalPosition;		//�`��̈�̐ݒ�

	//�E�B���h�E����
	*hWnd = CreateWindow(
		(LPCTSTR)atom,
		(LPCTSTR)cWinName,
		WS_CHILD | WS_VISIBLE,//| WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME |WS_VISIBLE, 
		WinRect.left, WinRect.top,
		WinRect.right - WinRect.left, WinRect.bottom - WinRect.top,
		hPaWnd, chID, hInst, NULL
	);

	if (*hWnd == NULL) {
		MessageBox(hPaWnd, TEXT("�E�B���h�E�̐����Ɏ��s���܂����D"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}


//�f�[�^�ǂݍ��ݗp�X���b�h
UINT WINAPI TFunc(LPVOID thParam)
{
	static SEND_POINTER_STRUCT* FU = (SEND_POINTER_STRUCT*)thParam;        //�\���̂̃|�C���^�擾

	FILE* fp;			//�t�@�C���|�C���^
	BOOL Flag = TRUE;		//���[�v�t���O
	char buf[64];
	int t;								//���莞��
	double data[4], prevdata[4];		//�f�[�^
	DWORD DNum = 0, beforeTime;
	DWORD nowTime, progress, idealTime;
	int time = 0;
	HDC	hdc1, hdc2, hdc3, hdc4;				//�f�o�C�X�R���e�L�X�g�̃n���h��
	PAINTSTRUCT ps1, ps2, ps3, ps4;			//(�\����)�N���C�A���g�̈�`�悷�邽�߂̏��	
	HPEN hPen1, hPen2, hPen3, hPen4;			//�y��
	
	beforeTime = timeGetTime();				//���݂̎����v�Z�i�������ԁj

	//�t�@�C���I�[�v��
	if ((fopen_s(&fp, "data_4ch.txt", "r")) != 0) {
		MessageBox(NULL, TEXT("�t�@�C�����J���܂���ł����D"), NULL, MB_OK | MB_ICONERROR);
		return FALSE;
	}
	fgets(buf, sizeof(buf), fp); //�ŏ���1�s���΂�

	//�f�[�^�ǂݍ��݁E�g�`�`��
	int x = 0;									//x���W
	colorPen = RGB(255, 255, 0);				//���̐F
	hdc1 = BeginPaint(FU->hEdit1, &ps1);
	hdc2 = BeginPaint(FU->hEdit2, &ps2);
	hdc3 = BeginPaint(FU->hEdit3, &ps3);
	hdc4 = BeginPaint(FU->hEdit4, &ps4);
	hPen1 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc1, hPen1);
	hPen2 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc2, hPen2);
	hPen3 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc3, hPen3);
	hPen4 = CreatePen(PS_SOLID, 2, colorPen);
	SelectObject(hdc4, hPen4);

	color = RGB(0, 0, 0);
	colorPen = RGB(255, 255, 255);
	InvalidateRect(FU->hEdit1, NULL, TRUE);
	PaintAxis(FU->hEdit1, color, colorPen);
	InvalidateRect(FU->hEdit2, NULL, TRUE);
	PaintAxis(FU->hEdit2, color, colorPen);
	InvalidateRect(FU->hEdit3, NULL, TRUE);
	PaintAxis(FU->hEdit3, color, colorPen);
	InvalidateRect(FU->hEdit4, NULL, TRUE);
	PaintAxis(FU->hEdit4, color, colorPen);

	while (Flag == TRUE) {
		//���Ԃ̒���
		nowTime = timeGetTime();					//���݂̎����v�Z
		progress = nowTime - beforeTime;				//�������Ԃ��v�Z
		idealTime = (DWORD)(DNum * (1000.0 / (double)DEF_DATAPERS));	//���z���Ԃ��v�Z
		if (idealTime > progress) {
			Sleep(idealTime - progress);			//���z���ԂɂȂ�܂őҋ@
		}

		DNum++;
		//��b�o�ߎ�
		if (progress >= 1000.0) {
			beforeTime = nowTime;
			DNum = 0;
		}
		
		//�f�[�^�̓ǂݍ���
		if (fscanf_s(fp, "%d\t%lf\t%lf\t%lf\t%lf",&t, &(data[0]), &(data[1]), &(data[2]), &(data[3])) == EOF) {
			MessageBox(NULL, TEXT("�I��"), TEXT("INFORMATION"), MB_OK | MB_ICONEXCLAMATION);
			EnableWindow(GetDlgItem(FU->hwnd, ID_START), TRUE);		//�J�n�{�^���L��
			Flag = FALSE;											//���[�v�I���t���O
			return FALSE;
		}

		if (x == 0) {
			prevdata[0] = data[0];
			prevdata[1] = data[1];
			prevdata[2] = data[2];
			prevdata[3] = data[3];
			x += 1;
			continue;
		}

		//�g�`�`��
		MoveToEx(hdc1, XMIN + x - 1, (int)((YMAX / DTMAX) * (-prevdata[0]) + Y_OFFSET), NULL);
		LineTo(hdc1, XMIN + x, (int)((YMAX / DTMAX) * (-data[0]) + Y_OFFSET));
		MoveToEx(hdc2, XMIN + x - 1, (int)((YMAX / DTMAX) * (-prevdata[1]) + Y_OFFSET), NULL);
		LineTo(hdc2, XMIN + x, (int)((YMAX / DTMAX) * (-data[1]) + Y_OFFSET));
		MoveToEx(hdc3, XMIN + x - 1, (int)((YMAX / DTMAX) * (-prevdata[2]) + Y_OFFSET), NULL);
		LineTo(hdc3, XMIN + x, (int)((YMAX / DTMAX) * (-data[2]) + Y_OFFSET));
		MoveToEx(hdc4, XMIN + x - 1, (int)((YMAX / DTMAX) * (-prevdata[3]) + Y_OFFSET), NULL);
		LineTo(hdc4, XMIN + x, (int)((YMAX / DTMAX) * (-data[3]) + Y_OFFSET));
		prevdata[0] = data[0];
		prevdata[1] = data[1];
		prevdata[2] = data[2];
		prevdata[3] = data[3];

		if (x + XMIN >= XMAX) {		//�[�܂ōs�����珑������
			color = RGB(0, 0, 0);
			colorPen = RGB(255, 255, 255);
			InvalidateRect(FU->hEdit1, NULL, TRUE);
			PaintAxis(FU->hEdit1, color, colorPen);
			InvalidateRect(FU->hEdit2, NULL, TRUE);
			PaintAxis(FU->hEdit2, color, colorPen);
			InvalidateRect(FU->hEdit3, NULL, TRUE);
			PaintAxis(FU->hEdit3, color, colorPen);
			InvalidateRect(FU->hEdit4, NULL, TRUE);
			PaintAxis(FU->hEdit4, color, colorPen);
			x = 0;
		}
		x += 1;
	}
	EndPaint(FU->hEdit1, &ps1);
	EndPaint(FU->hEdit2, &ps2);
	EndPaint(FU->hEdit3, &ps3);
	EndPaint(FU->hEdit4, &ps4);
	DeleteObject(hPen1);
	DeleteObject(hPen2);
	DeleteObject(hPen3);
	DeleteObject(hPen4);

	return 0;
}

//PictureBox����x����y����`��
void PaintAxis(HWND hWnd, COLORREF color, COLORREF colorPen) {
	HDC			hdc;				//�f�o�C�X�R���e�L�X�g�̃n���h��
	PAINTSTRUCT ps;					//(�\����)�N���C�A���g�̈�`�悷�邽�߂̏��	
	HBRUSH		hBrush;				//�u���V
	HPEN		hPen;				//�y��
	static RECT rect;
	//TCHAR str[32];
	/********************************

		PictureControl�ɕ`�悷�邽�߂ɂ́CHDC�^�̃n���h����ʂɎ擾����
		�K�v������܂��D

		��Fhdc = BeginPaint(hWnd, &ps);
		hdc:�f�o�C�X�R���e�L�X�g�̃n���h��
		hWnd:PictureControl�̃n���h��
		ps�F(�\����)�N���C�A���g�̈�`�悷�邽�߂̏��

		********************************/

	hdc = BeginPaint(hWnd, &ps);//�f�o�C�X�R���e�L�X�g�̃n���h���擾

	/********************************

	PictureControl�ɕ`�悷�邽�߂ɂ́C�������������Ƃ��̓y���C
	�h��Ԃ��ۂɂ̓u���V���K�v�ł��D

	********************************/

	color = RGB(0, 0, 0);
	colorPen = RGB(255, 255, 255);
	//�y���C�u���V����
	hBrush = CreateSolidBrush(color);				//�u���V����
	SelectObject(hdc, hBrush);						//�u���V�ݒ�
	hPen = CreatePen(PS_SOLID, 2, colorPen);		//�y������
	SelectObject(hdc, hPen);						//�y���ݒ�

	//�`��
	/********************************

	�}�`��`�悷�邽�߂ɂ͈ȉ��̊֐���p���܂��D
	�����`�FRectangle(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);
	�~�FEllipse(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);

	 nLiftRect�F�����`�̍���X���W
	  nTopRect�F����Y���W
	  nRightRect�F�E��X���W
	  nBottomRect�F�E����Y���W

	���������ɂ͈ȉ��̊֐���p���܂��D

	���̎n�_�ݒ�FMoveToEx(HDC hdc , int X , int Y , NULL);
	  X,Y�F���̎n�_�̍��W
	���GLineTo(HDC hdc , int nXEnd , int nYEnd);
	  nXEnd, nYEnd�F���̏I�_�̐ݒ�


	  �ȏ���Q�l�ɐ}�`��`�悷��֐����ȉ��ɋL�q���܂��傤
	********************************/
	//��������
	GetClientRect(hWnd, &rect);
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);	//�w�i�����ɓh��Ԃ�
	MoveToEx(hdc, XMIN, Y_OFFSET, NULL);
	LineTo(hdc, XMAX, Y_OFFSET);									//x���`��
	MoveToEx(hdc, XMIN, 20, NULL);
	LineTo(hdc, XMIN, 252);											//y���`��


	//�����܂�
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, 290, 250, TEXT("Time [s]"), 8);		//�e�L�X�g�`��
	//wsprintf(str, L"%d �~ %d", rect.right, rect.bottom);
	//TextOut(hdc, 400, 10, str, 9);

	//�f�o�C�X�R���e�L�X�g�̃n���h���j��
	EndPaint(hWnd, &ps);

	//�y���C�u���V�p��
	/********************************

	�g���I������y���ƃu���V�͔j������K�v������܂��D

	********************************/
	DeleteObject(hBrush);
	DeleteObject(hPen);

}