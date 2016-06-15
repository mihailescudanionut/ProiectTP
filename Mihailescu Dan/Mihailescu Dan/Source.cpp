#include <windows.h>
#include <wchar.h>
#include <commctrl.h>
#include <strsafe.h>
#include <string.h>

#define IDC_MAIN_INSERTION		1001
#define IDC_MAIN_CpyRIGHT		1002
#define IDC_MAIN_RESET_CTL		1004
#define IDC_MAIN_INSERT_BUTTON	1005

HWND _hwnd;
HINSTANCE _hInstance;
MSG  msg;
int _nCmdShow;
HDC hdc;
PAINTSTRUCT ps;
bool check_WndAdauga = false;

HWND _NUME, _Banca, _Data, _PIN, _ID;
HWND hwnd_InsertWindow;

int iteratie_card = 0;           //Pentru a sti datele pe care sa le inseram

struct Date_Card
{
	char ID_Card[20];
	char Nume_Proprietar[100];
	char Banca[100];
	char Data_Expiratii[20];
};
Date_Card card[10];
int nr_carduri = 0;
HWND carduri[10];

bool CHECKPIN(char *sir){
	if (strlen(sir) != 4)
		return true;
	for (int i = 0; i < strlen(sir); i++)
	{
		if (!isdigit(sir[i]))
			return true;
	}
	return false;
}


//VLIST Algoritm
//-------------------------------------------------------------------------------
typedef struct sublist{
	struct sublist* next;
	int *buf;
} sublist_t;
sublist_t* sublist_new(int s)
{
	sublist_t* sub = (sublist_t*)malloc(sizeof(sublist_t) + sizeof(int) * s);
	sub->buf = (int*)(sub + 1);
	sub->next = 0;
	return sub;
}
typedef struct vlist_t {
	sublist_t* head;
	int last_size, ofs;
} vlist_t, *vlist;
vlist v_new()
{
	vlist v = (vlist)malloc(sizeof(vlist_t));
	v->head = sublist_new(1);
	v->last_size = 1;
	v->ofs = 0;
	return v;
}
//Initializam Vlist
vlist v = v_new();
void v_del(vlist v)
{
	sublist_t *s;
	while (v->head) {
		s = v->head->next;
		free(v->head);
		v->head = s;
	}
	free(v);
}
inline int v_size(vlist v)
{
	return v->last_size * 2 - v->ofs - 2;
}
int* v_addr(vlist v, int idx)
{
	sublist_t *s = v->head;
	int top = v->last_size, i = idx + v->ofs;

	if (i + 2 >= (top * 2)) {
		fprintf(stderr, "!: idx %d out of range\n", idx);
		abort();
	}
	while (s && i >= top) {
		s = s->next, i = i ^ top;
		top = top / 2;
	}
	return s->buf + i;
}
inline int v_elem(vlist v, int idx)
{
	return *v_addr(v, idx);
}
int* v_add(vlist v, int x)
{
	sublist_t* s;
	int *p;

	if (!v->ofs) {
		if (!(s = sublist_new(v->last_size * 2))) {
			fprintf(stderr, "?: alloc failure\n");
			return 0;
		}
		v->ofs = (v->last_size *= 2);
		s->next = v->head;
		v->head = s;
	}
	v->ofs--;
	p = v->head->buf + v->ofs;
	*p = x;
	return p;
}
void v_delete(vlist v)
{
	sublist_t* s;
	int x;

	if (v->last_size == 1 && v->ofs == 1) {
		fprintf(stderr, "!: empty list\n");
		abort();
	}
	v->ofs++;
	if (v->ofs == v->last_size) {
		v->ofs = 0;
		if (v->last_size > 1) {
			s = v->head, v->head = s->next;
			v->last_size /= 2;
			free(s);
		}
	}

}
//-------------------------------------------------------------------------------


//Procedura fereastra principala
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void Paint_Card(HWND hwnd);

//Fereastra de insertie
void Regist_Insertion_Wnd();
void Afiseaza_Card(int i);

//Procedura card
LRESULT CALLBACK CArdProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void Rgister_Card();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_hInstance = hInstance;
	_nCmdShow = nCmdShow;

	WNDCLASSW wc;
	//Atribuim parametrii ferestrei
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszClassName = L"MainWnd";
	wc.hInstance = hInstance;
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc.lpszMenuName = NULL;
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//Inregistram fereastra
	RegisterClassW(&wc);
	//Creem fereastra
	_hwnd = CreateWindowW(L"MainWnd", L"Card++",WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE, 0, 0, 1210, 550, NULL, NULL, hInstance, NULL);
	//Afisam fereastra
	ShowWindow(_hwnd, nCmdShow);
	UpdateWindow(_hwnd);
	//Message loopul de unde asteptam introducerea de noi comenzi
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {

	case WM_PAINT:
		Paint_Card(hwnd);
		break;

	case WM_CREATE:
	{
		CreateWindow("BUTTON", "Adauga Card",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			20, 20, 157, 30,
			hwnd, (HMENU)IDC_MAIN_INSERTION, _hInstance, NULL);
		CreateWindow("BUTTON", "CopyRight",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			177, 20, 157, 30,
			hwnd, (HMENU)IDC_MAIN_CpyRIGHT, _hInstance, NULL);
		CreateWindow("BUTTON", "Resetare Date",
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			334, 20, 157, 30,
			hwnd, (HMENU)IDC_MAIN_RESET_CTL, _hInstance, NULL);
	}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MAIN_INSERTION:
			if (!check_WndAdauga)
			{
				check_WndAdauga = true;
				Regist_Insertion_Wnd();
				hwnd_InsertWindow = CreateWindowW(L"InsertionWindow", NULL, WS_CHILD | WS_VISIBLE, 50, 150, 400, 200, hwnd, NULL, _hInstance, NULL);
			}
			else
			{
				check_WndAdauga = false;
				DestroyWindow(hwnd_InsertWindow);
			}
			break;

			break;
		case IDC_MAIN_RESET_CTL:
			for (int i = 0; i < nr_carduri; i++)
				DestroyWindow(carduri[i]);
			nr_carduri = 0;
			iteratie_card = 0;
			break;
		case IDC_MAIN_CpyRIGHT:
			MessageBox(hwnd, "                               All rights reserved to\n                                      Mihailescu\nFolosirea neautorizata a acestor date se poate sanctiona penal! ", "About", MB_ICONEXCLAMATION);
			break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
void Paint_Card(HWND hwnd){
	hdc = BeginPaint(hwnd, &ps);

	HPEN hPenOld;
	HPEN hLinePen;
	COLORREF qLineColor;

	//Delimitari linia verticala
	qLineColor = RGB(0, 0, 255);
	hLinePen = CreatePen(PS_SOLID, 7, qLineColor);
	hPenOld = (HPEN)SelectObject(hdc, hLinePen);
	MoveToEx(hdc, 500, 0, NULL);
	LineTo(hdc, 500, 550);

	//Distrugem cardurile initiale
	for (int i = 0; i < nr_carduri - 1; i++)
		DestroyWindow(carduri[i]);

	Rgister_Card();
	for (int i = 0; i < nr_carduri; i++)
	{
		if (i >= 0 && i <= 2) iteratie_card = i,
			carduri[i] = CreateWindowW(L"CArdClass", NULL, WS_CHILD | WS_VISIBLE, 520 + 220 * i, 20, 200, 125, _hwnd, NULL, _hInstance, NULL);
		else if (i >= 3 && i <= 5) iteratie_card = i,
			carduri[i] = CreateWindowW(L"CArdClass", NULL, WS_CHILD | WS_VISIBLE, 520 + 220 * (i - 3), 180, 200, 125, _hwnd, NULL, _hInstance, NULL);
		else if (i >= 6 && i <= 8) iteratie_card = i,
			carduri[i] = CreateWindowW(L"CArdClass", NULL, WS_CHILD | WS_VISIBLE, 520 + 220 * (i - 6), 340, 200, 125, _hwnd, NULL, _hInstance, NULL);
	}
	
	SelectObject(hdc, hPenOld);
	DeleteObject(hLinePen);
	EndPaint(hwnd, &ps);
}

//Fereastra in care adaugam carduri
LRESULT CALLBACK Wnd_InserareCard(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		CreateWindowW(L"static", L"Nume Utilizator: ", WS_CHILD | WS_VISIBLE, 10, 20, 140, 25, hwnd, (HMENU)1, NULL, NULL);
		_NUME = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 138, 19, 258, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		CreateWindowW(L"static", L"Banca: ", WS_CHILD | WS_VISIBLE, 10, 50, 65, 25, hwnd, (HMENU)1, NULL, NULL);
		_Banca = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 70, 50, 326, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		CreateWindowW(L"static", L"Data Expirarii:", WS_CHILD | WS_VISIBLE, 10, 80, 150, 25, hwnd, (HMENU)1, NULL, NULL);
		_Data = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 123, 80, 272, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		CreateWindowW(L"static", L"ID Card:", WS_CHILD | WS_VISIBLE, 10, 110, 80, 25, hwnd, (HMENU)1, NULL, NULL);
		_ID = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 75, 110, 320, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		CreateWindowW(L"static", L"*PIN: ", WS_CHILD | WS_VISIBLE, 80, 150, 80, 25, hwnd, (HMENU)1, NULL, NULL);
		_PIN = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 135, 150, 110, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		HGDIOBJ hfDefault = GetStockObject(DEFAULT_GUI_FONT);
		HWND hwndButton = CreateWindowEx(NULL, "BUTTON", "Adauga", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 260, 150, 120, 25, hwnd, (HMENU)IDC_MAIN_INSERT_BUTTON, GetModuleHandle(NULL), NULL);
	}
		break;

	case WM_KEYDOWN:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MAIN_INSERT_BUTTON:
		{
			if (nr_carduri == 9)
			{
				MessageBox(NULL, "Maxim 9 carduri de credit", "ERROR", MB_ICONERROR);
			}
			else
			{
				//Luam datele din chenar
				SendMessage(_NUME, WM_GETTEXT, sizeof(card[nr_carduri].Nume_Proprietar) / sizeof(char), reinterpret_cast<LPARAM>(card[nr_carduri].Nume_Proprietar));
				SendMessage(_Banca, WM_GETTEXT, sizeof(card[nr_carduri].Banca) / sizeof(char), reinterpret_cast<LPARAM>(card[nr_carduri].Banca));
				SendMessage(_ID, WM_GETTEXT, sizeof(card[nr_carduri].ID_Card) / sizeof(char), reinterpret_cast<LPARAM>(card[nr_carduri].ID_Card));
				SendMessage(_Data, WM_GETTEXT, sizeof(card[nr_carduri].Data_Expiratii) / sizeof(char), reinterpret_cast<LPARAM>(card[nr_carduri].Data_Expiratii));

				char buff[10];
				SendMessage(_PIN, WM_GETTEXT, sizeof(buff) / sizeof(char), reinterpret_cast<LPARAM>(buff));

				if (strlen(card[nr_carduri].Nume_Proprietar) == 0)
				{
					MessageBoxW(hwnd, L"Trebuie introdus un nume de Utilizator!", L"ERROR", MB_ICONERROR);
				}
				else if (CHECKPIN(buff))
				{
					MessageBoxW(hwnd, L"Formatul pinului nu este corect!", L"ERROR", MB_ICONERROR);
				}
				else
				{
					//Adaugam pinurile in Vlist
					v_add(v, atoi(buff));
					//Afisam cardurile		
					Afiseaza_Card(nr_carduri);
					nr_carduri++;
					InvalidateRect(_hwnd, NULL, TRUE);
				}
			}
		}
			break;
		}
		break;
	case WM_DESTROY:
	{
		check_WndAdauga = false;
		DestroyWindow(hwnd_InsertWindow);
		return 0;
	}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
void Regist_Insertion_Wnd()
{
	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbClsExtra = NULL;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = NULL;
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wClass.hIcon = NULL;
	wClass.hIconSm = NULL;
	wClass.hInstance = _hInstance;
	wClass.lpfnWndProc = (WNDPROC)Wnd_InserareCard;
	wClass.lpszClassName = "InsertionWindow";
	wClass.lpszMenuName = NULL;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wClass);
}
void Afiseaza_Card(int i){
	//CreateWindow("static", elevi_catalog[i].nume, WS_CHILD | WS_VISIBLE, 65, 125 + 32 * i, 120, 25, _hwnd, (HMENU)1, NULL, NULL);
	//CreateWindow("static", elevi_catalog[i].prenume, WS_CHILD | WS_VISIBLE, 225, 125 + 32 * i, 130, 25, _hwnd, (HMENU)1, NULL, NULL);
	//CreateWindow("static", elevi_catalog[i].nota_rom, WS_CHILD | WS_VISIBLE, 405, 125 + 32 * i, 50, 25, _hwnd, (HMENU)1, NULL, NULL);
	//CreateWindow("static", elevi_catalog[i].nota_mate, WS_CHILD | WS_VISIBLE, 467, 125 + 32 * i, 50, 25, _hwnd, (HMENU)1, NULL, NULL);
	//CreateWindow("static", elevi_catalog[i].nota_info, WS_CHILD | WS_VISIBLE, 530, 125 + 32 * i, 50, 25, _hwnd, (HMENU)1, NULL, NULL);
	//CreateWindow("static", elevi_catalog[i].media, WS_CHILD | WS_VISIBLE, 592, 125 + 32 * i, 50, 25, _hwnd, (HMENU)1, NULL, NULL);
}

//Procedura Carduri
LRESULT CALLBACK CArdProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT:
	{
		HDC hdc;
		PAINTSTRUCT Ps;
		hdc = BeginPaint(hwnd, &Ps);

		HPEN hPenOld;
		HPEN hLinePen;
		COLORREF qLineColor;
		HBRUSH hBrush, holdBrush;

		RECT rect;
		GetWindowRect(hwnd, &rect);

		qLineColor = RGB(222, 222, 222);
		hLinePen = CreatePen(PS_SOLID, 4, qLineColor);
		hPenOld = (HPEN)SelectObject(hdc, hLinePen);
		hBrush = CreateSolidBrush(RGB(55, 5, 155));
		holdBrush = (HBRUSH)SelectObject(hdc, hBrush);

		RoundRect(hdc, 2, 2, rect.right - rect.left - 2, rect.bottom - rect.top - 2, 5, 5);

	}
		break;

	case WM_CREATE:
		CreateWindow("static", card[iteratie_card].Nume_Proprietar, WS_CHILD | WS_VISIBLE, 7, 10, 185, 20, hwnd, (HMENU)1, NULL, NULL);
		CreateWindow("static", card[iteratie_card].ID_Card, WS_CHILD | WS_VISIBLE, 7, 40, 185, 20, hwnd, (HMENU)1, NULL, NULL);
		CreateWindow("static", card[iteratie_card].Data_Expiratii, WS_CHILD | WS_VISIBLE, 72, 85, 115, 20, hwnd, (HMENU)1, NULL, NULL);
		break;

	case WM_LBUTTONUP:
		MessageBeep(MB_OK);
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
void Rgister_Card()
{
	HBRUSH hbrush = CreateSolidBrush(RGB(0, 0, 0));
	WNDCLASSW rwc = { 0 };

	rwc.lpszClassName = L"CArdClass";
	rwc.hbrBackground = hbrush;
	rwc.lpfnWndProc = CArdProc;
	rwc.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassW(&rwc);
}




