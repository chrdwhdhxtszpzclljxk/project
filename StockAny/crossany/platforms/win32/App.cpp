#include "../app.h"
#include <Windows.h>
#include <math.h>
#include "../CCGL.h"
#include "../uibase.h"

NS_CROSSANY_BEGIN

#define MAX_LOADSTRING 100
#define _T(a) TEXT(a)
App * App::sm_pSharedApp = NULL;

// ȫ�ֱ���: 
static HINSTANCE hInst;								// ��ǰʵ��
const static TCHAR* szTitle = L"crossany_wnd";					// �������ı�
const static TCHAR* szWindowClass = L"crossany_wnd";			// ����������

static BOOL painted = FALSE;
int64_t frames = 0;
HGLRC hRC = NULL;     // ������ɫ������  
HDC hDC = NULL;           // ˽��GDI�豸������  
HWND hWnd = NULL;     // �������ǵĴ��ھ��  
HINSTANCE hInstance;    // ��������ʵ��  
bool keys[256];         // ���ڼ������̵�����  
bool active = TRUE;       // ���ڵĻ��־��ȱʡΪTRUE  
bool fullscreen = TRUE;   // ȫ����־ȱʡ�趨��ȫ��ģʽ  
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);// WndProc�Ķ���  
GLvoid ReSizeGLScene(GLsizei width, GLsizei height){// ���ò���ʼ��GL���ڴ�С  
	if (height == 0) height = 1;// ��ֹ����� // ��Height��Ϊ1  
	glMatrixMode(GL_PROJECTION);// ѡ��ͶӰ����  
	glLoadIdentity();// ����ͶӰ����  
	//gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);	// ���㴰�ڵ���۱���  
	gluOrtho2D(0, width, 0, height);
	glViewport(0, 0, width, height);// ���õ�ǰ���ӿ�(Viewport)  
	glMatrixMode(GL_MODELVIEW);// ѡ��ģ�͹۲����  
	//glLoadIdentity();// ����ģ�͹۲����  
}
int InitGL(GLvoid){// �˴���ʼ��OpenGL������������  
	glShadeModel(GL_SMOOTH);// ������Ӱƽ��  
	glClearColor(0.65f, 0.65f, 0.65f, 0.0f);// ��ɫ����  
	glClearDepth(1.0f);// ������Ȼ���  
	glEnable(GL_DEPTH_TEST);// ������Ȳ���  
	glDepthFunc(GL_LEQUAL);// ������Ȳ��Ե�����  
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);// ������ϸ��͸������  
	return TRUE;// ��ʼ�� OK  
}
int DrawGLScene(GLvoid){// �����￪ʼ�������еĻ���  
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// �����Ļ����Ȼ���  
	//glLoadIdentity();// ���õ�ǰ��ģ�͹۲����  
	// �����µ�����  
	glColor4f(1.0f, 0.0f, 0.0f,0.5f);
	uibase ui;
	//ui.drawrc(20,20,200,200);
	//ui.drawln(200, 200, 400, 400);
	//ui.drawpt(300, 300);
	ui.fillrc(20, 20, 300, 300);
	OutputDebugString(_T("DrawGLScene %d\r\n"));// , frames);
	return TRUE;// һ��OK  
}
GLvoid KillGLWindow(GLvoid){// �������ٴ���  
	if (fullscreen){// ����ȫ��ģʽ��  
		ChangeDisplaySettings(NULL, 0);// �ǵĻ����л�������  
		ShowCursor(TRUE);// ��ʾ���ָ��  
	}
	if (hRC){// ӵ����ɫ��������  
		if (!wglMakeCurrent(NULL, NULL)) MessageBox(NULL, _T("Release Of DC And RC Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);// �ܷ��ͷ�DC��RC������  
		if (!wglDeleteContext(hRC))	MessageBox(NULL, _T("Release Rendering Context Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);// �ܷ�ɾ��RC  
		hRC = NULL;// ��RC��ΪNULL  
	}
	if (hDC && !ReleaseDC(hWnd, hDC)){// �ܷ��ͷ�DC  
		MessageBox(NULL, _T("Release Device Context Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hDC = NULL;// ��DC��ΪNULL  
	}
	if (hWnd && !DestroyWindow(hWnd)){// �ܷ����ٴ���  
		MessageBox(NULL, _T("Could Not Release hWnd."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;// ��hWnd��ΪNULL  
	}
	if (!UnregisterClass(_T("OpenGL"), hInstance)){// �ܷ�ע��������  
		MessageBox(NULL, _T("Could Not Unregister Class."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;// ��hInstance��ΪNULL  
	}
}

BOOL CreateGLWindow(TCHAR* title, int width, int height, int bits, bool fullscreenflag){
	GLuint PixelFormat;                     // �������ƥ��Ľ��  
	WNDCLASS wc;                            // ������ṹ  
	DWORD dwExStyle;                        // ��չ���ڷ��  
	DWORD dwStyle;                          // ���ڷ��  
	RECT WindowRect;                        // ȡ�þ��ε����ϽǺ����½ǵ�����ֵ  
	WindowRect.left = (long)0;                // ��Left��Ϊ 0  
	WindowRect.right = (long)width;       // ��Right��ΪҪ��Ŀ��  
	WindowRect.top = (long)0;             // ��Top��Ϊ 0  
	WindowRect.bottom = (long)height; // ��Bottom��ΪҪ��ĸ߶�  
	fullscreen = fullscreenflag;              // ����ȫ��ȫ����־  
	hInstance = GetModuleHandle(NULL);                              // ȡ�ô��ڵ�ʵ��  
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;      // �ƶ�ʱ�ػ�����Ϊ����ȡ��DC  
	wc.lpfnWndProc = (WNDPROC)WndProc;                         // WndProc������Ϣ  
	wc.cbClsExtra = 0;                                                      // �޶��ⴰ������  
	wc.cbWndExtra = 0;                                                      // �޶��ⴰ������  
	wc.hInstance = hInstance;                                           // ����ʵ��  
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);                     // װ��ȱʡͼ��  
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);                   // װ�����ָ��  
	wc.hbrBackground = NULL;                                                // GL����Ҫ����  
	wc.lpszMenuName = NULL;                                             // ����Ҫ�˵�  
	wc.lpszClassName = _T("OpenGL");                                        // �趨������  

	if (!RegisterClass(&wc)){                                                    // ����ע�ᴰ����  
		MessageBox(NULL, _T("Failed To Register The Window Class."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// �˳�������FALSE  
	}
	if (fullscreen){// Ҫ����ȫ��ģʽ��  
		DEVMODE dmScreenSettings;                                           // �豸ģʽ  
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));   // ȷ���ڴ����  
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);           // Devmode �ṹ�Ĵ�С  
		dmScreenSettings.dmPelsWidth = width;                           // ��ѡ��Ļ���  
		dmScreenSettings.dmPelsHeight = height;                         // ��ѡ��Ļ�߶�  
		dmScreenSettings.dmBitsPerPel = bits;                               // ÿ������ѡ��ɫ�����  
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		// ����������ʾģʽ�����ؽ����ע: CDS_FULLSCREEN ��ȥ��״̬����  
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
			// ��ģʽʧ�ܣ��ṩ����ѡ��˳����ڴ��������С�  
			if (MessageBox(NULL, _T("The Requested Fullscreen Mode Is Not Supported By/nYour Video Card. Use Windowed Mode Instead?"), _T("NeHe GL"), 
				MB_YESNO | MB_ICONEXCLAMATION) == IDYES){
				fullscreen = FALSE;// ѡ�񴰿�ģʽ(Fullscreen=FALSE)  
			}else{
				// Pop Up A Message Box Letting User Know The Program Is Closing.  
				MessageBox(NULL, _T("Program Will Now Close."), _T("ERROR"), MB_OK | MB_ICONSTOP);
				return FALSE;//�˳�������FALSE  
			}
		}
	}
	if (fullscreen){// �Դ���ȫ��ģʽ��  
		dwExStyle = WS_EX_APPWINDOW;  // ��չ������  
		dwStyle = WS_POPUP;                   // ������  
		ShowCursor(FALSE);                      // �������ָ��  
	}else{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;   // ��չ������  
		dwStyle = WS_OVERLAPPEDWINDOW;                                // ������  
	}
	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);// �������ڴﵽ����Ҫ��Ĵ�С  
	if (!(hWnd = CreateWindowEx(dwExStyle,           // ��չ������  
		_T("OpenGL"),                                           // ������  
		title,                                                      // ���ڱ���  
		WS_CLIPSIBLINGS |                                   // ����Ĵ���������  
		WS_CLIPCHILDREN |                                   // ����Ĵ���������  
		dwStyle,                                                    // ѡ��Ĵ�������  
		0, 0,                                                        // ����λ��  
		WindowRect.right - WindowRect.left,               // ��������õĴ��ڿ��  
		WindowRect.bottom - WindowRect.top,           // ��������õĴ��ڸ߶�  
		NULL,                                                       // �޸�����  
		NULL,                                                       // �޲˵�  
		hInstance,                                              // ʵ��  
		NULL)))                                                 // ����WM_CREATE�����κζ���  
	{
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Window Creation Error."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	static PIXELFORMATDESCRIPTOR pfd = {//pfd ���ߴ���������ϣ���Ķ���  
		sizeof(PIXELFORMATDESCRIPTOR),          //������ʽ�������Ĵ�С  
		1,                                                      // �汾��  
		PFD_DRAW_TO_WINDOW |                        // ��ʽ����֧�ִ���  
		PFD_SUPPORT_OPENGL |                        // ��ʽ����֧��OpenGL  
		PFD_DOUBLEBUFFER,                               // ����֧��˫����  
		PFD_TYPE_RGBA,                                  // ���� RGBA ��ʽ  
		bits,                                                   // ѡ��ɫ�����  
		0, 0, 0, 0, 0, 0,                                   // ���Ե�ɫ��λ  
		0,                                                      // ��Alpha����  
		0,                                                      // ����Shift Bit  
		0,                                                      // �޾ۼ�����  
		0, 0, 0, 0,                                         // ���Ծۼ�λ  
		16,                                                 // 16λ Z-���� (��Ȼ���)  
		0,                                                      // ��ģ�建��  
		0,                                                      // �޸�������  
		PFD_MAIN_PLANE,                             // ����ͼ��  
		0,                                                      // ����  
		0, 0, 0                                             // ���Բ�����  
	};
	if (!(hDC = GetDC(hWnd))){// ȡ���豸��������ô  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Can't Create A GL Device Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd))){// Windows �ҵ���Ӧ�����ظ�ʽ����  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Can't Find A Suitable PixelFormat."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	if (!SetPixelFormat(hDC, PixelFormat, &pfd)){// �ܹ��������ظ�ʽô  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Can't Set The PixelFormat."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	if (!(hRC = wglCreateContext(hDC))){// �ܷ�ȡ����ɫ������  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Can't Create A GL Rendering Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	if (!wglMakeCurrent(hDC, hRC)){// ���Լ�����ɫ������  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Can't Activate The GL Rendering Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	ShowWindow(hWnd, SW_SHOW);       // ��ʾ����  
	SetForegroundWindow(hWnd);      // ����������ȼ�  
	SetFocus(hWnd);                         // ���ü��̵Ľ������˴���  
	ReSizeGLScene(width, height);       // ����͸�� GL ��Ļ  
	if (!InitGL()){// ��ʼ���½���GL����  
		KillGLWindow();// ������ʾ��  
		MessageBox(NULL, _T("Initialization Failed."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;// ���� FALSE  
	}
	return TRUE;// �ɹ�  
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam) {                // ���ӵ���Ϣ����  
	switch (uMsg){// ���Windows��Ϣ  
	case WM_ACTIVATE:{// ���Ӵ��ڼ�����Ϣ  
		if (!HIWORD(wParam)) active = TRUE;// �����С��״̬  // �����ڼ���״̬  
		else active = FALSE;// �����ټ���  
		return 0;// ������Ϣѭ��  
	}break;
	case WM_SYSCOMMAND:{// �ж�ϵͳ����Intercept System Commands  
		switch (wParam){// ���ϵͳ����Check System Calls  
		case SC_SCREENSAVE:// ����Ҫ����?  
		case SC_MONITORPOWER:// ��ʾ��Ҫ����ڵ�ģʽ?  
			return 0;// ��ֹ����  
		}
		break;// �˳�  
	}break;
	case WM_CLOSE:{// �յ�Close��Ϣ?  
		PostQuitMessage(0);// �����˳���Ϣ  
		return 0;
	}break;
	case WM_KEYDOWN:{// �м�����ô?  
		keys[wParam] = TRUE;// ����ǣ���ΪTRUE  
		return 0;// ����  
	}break;
	case WM_KEYUP:{// �м��ſ�ô?  
		keys[wParam] = FALSE;// ����ǣ���ΪFALSE  
		return 0;// ����  
	}break;
	case WM_SIZE:{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);	// ��DefWindowProc��������δ�������Ϣ��  
}


App* App::me() { 
	if (sm_pSharedApp == NULL) {
		sm_pSharedApp = new App();
		if (sm_pSharedApp != NULL) return sm_pSharedApp;
	}
	return sm_pSharedApp;
}
App::App(){
	//CA_ASSERT(!sm_pSharedApp); sm_pSharedApp = this;
}
App::~App() {}// { CA_ASSERT(this == sm_pSharedApp); sm_pSharedApp = NULLPTR; }

int App::run() {
	MSG msg; BOOL done = FALSE;     // �����˳�ѭ����Bool ����  
	// ��ʾ�û�ѡ������ģʽ  
	//if (MessageBox(NULL, _T("Would You Like To Run In Fullscreen Mode?"), _T("Start FullScreen?"), MB_YESNO | MB_ICONQUESTION) == IDNO)
		fullscreen = FALSE; // ����ģʽ  
	// ����OpenGL����  
	if (!CreateGLWindow(L"NeHe's OpenGL Framework", 640, 480, 16, fullscreen)) return 0;// ʧ���˳�  
	while (!done){// ����ѭ��ֱ�� done=TRUE  
		painted = FALSE;
		if (GetMessage(&msg, NULL, 0, 0) == 0)	break;		// ����Ϣ�ڵȴ���?  
		if (msg.message == WM_PAINT){ painted = TRUE; }
		TranslateMessage(&msg); // ������Ϣ  
		DispatchMessage(&msg);  // ������Ϣ  
		if (active){// ���򼤻��ô?  		// ���Ƴ���������ESC��������DrawGLScene()���˳���Ϣ  
			if (keys[VK_ESCAPE]) done = TRUE;// ESC ������ô?  // ESC �����˳��ź�  
			else{// �����˳���ʱ��ˢ����Ļ  
				if (painted){// ���û����Ϣ 					
					DrawGLScene();// ���Ƴ���  
					frames++;
					SwapBuffers(hDC);// �������� (˫����)  
				}
			}
		}
		if (keys[VK_F1]){// �����û�����F1����ȫ��ģʽ�ʹ���ģʽ���л�  
			keys[VK_F1] = FALSE;// ���ǣ�ʹ��Ӧ��Key�����е�ֵΪ FALSE  
			KillGLWindow();// ���ٵ�ǰ�Ĵ���  
			fullscreen = !fullscreen; // �л� ȫ�� / ���� ģʽ  
			// �ؽ� OpenGL ����  
			if (!CreateGLWindow(L"NeHe's OpenGL Framework", 640, 480, 16, fullscreen))	return 0;// �������δ�ܴ����������˳�  
		}
	}
	// �رճ���  
	KillGLWindow();// ���ٴ���  
	delete this;
	return (msg.wParam);// �˳�����  
}



NS_CROSSANY_END