#include "my_handler.h"
#include "cefclient/string_util.h"
#include <sstream>
#include <stdio.h>
#include <string>
#include "network\MelodyProxy.h"
#include "cefclient/resource.h"
#include "cefclient/resource_util.h"
#include "transparent_wnd.h"
#include "include/cef_download_handler.h"
#include "system.h"
#include "base64.h"
#include "filter_handler.h"
#include "network\TcpServer.h"
extern HINSTANCE hInst;
extern int CDECL MessageBoxPrintf (TCHAR * szCaption, TCHAR * szFormat, ...)  ;

MyHandler::MyHandler():ClientHandler(){
	win=NULL;
}
MyHandler::~MyHandler(){
}

CefRefPtr<CefDisplayHandler> MyHandler::GetDisplayHandler()
{
	return this;
}

CefRefPtr<CefRenderHandler> MyHandler::GetRenderHandler()
{
	return this; 
}
bool MyHandler::OnBeforeMenu(CefRefPtr<CefBrowser> browser, const CefMenuInfo& menuInfo) {
	if(menuInfo.typeFlags==1){
		return true;
	}
	else{
		return false;
	}
}
CefRefPtr<CefMenuHandler> MyHandler::GetMenuHandler() {
	return this;
}

bool MyHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                               KeyEventType type,
                               int code,
                               int modifiers,
                               bool isSystemKey,
                               bool isAfterJavaScript) {
  REQUIRE_UI_THREAD();
  if(!win)return false;
	TransparentWnd* winHandler=(TransparentWnd*)win;
  if ((code == 123 || code==120)&&type == KEYEVENT_RAWKEYDOWN) {
    // Special handling for the space character if a form element does not have
    // focus.
    if (type == KEYEVENT_RAWKEYDOWN&&winHandler->enableDevelop) {
		GetBrowser()->ShowDevTools();
		return true;
    }
	else{
		string s="var e = new CustomEvent('AlloyDesktopShowDev');dispatchEvent(e);";
		winHandler->ExecJS(s);
		return true;
	}
  }
  if(code == 116&&type == KEYEVENT_RAWKEYDOWN){
	if(winHandler->enableRefresh) {
		winHandler->ReloadIgnoreCache();
		return true;
	}
	else{
		string s="var e = new CustomEvent('AlloyDesktopRefresh');dispatchEvent(e);";
		winHandler->ExecJS(s);
	}
  }
  return false;
}

bool MyHandler::GetDownloadHandler(CefRefPtr<CefBrowser> browser,
                                       const CefString& mimeType,
                                       const CefString& fileName,
                                       int64 contentLength,
                                       CefRefPtr<CefDownloadHandler>& handler)
{
  REQUIRE_UI_THREAD();

  // Create the handler for the file download.
  handler = CreateDownloadHandler(this, fileName, contentLength);

  // Close the browser window if it is a popup with no other document contents.
  if (!browser->HasDocument()){
	if(win){
		TransparentWnd* winHandler=(TransparentWnd*)win;
		//delete winHandler;
	}
    //browser->CloseBrowser();
  }

  return true;
}

void* MyHandler::GetWin(){
	return (void *)this->win;
}

void MyHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame)
{
}

void MyHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
	std::stringstream ss;
	string s="var e = new CustomEvent('AlloyDesktopReady');"
	"setTimeout('dispatchEvent(e);',0);";
	ss << "var handler="<<(long)this->win<<";"<<s;
	frame->ExecuteJavaScript(ss.str(), "", 0);
}

bool MyHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                            const CefPopupFeatures& popupFeatures,
                            CefWindowInfo& windowInfo,
                            const CefString& url,
                            CefRefPtr<CefClient>& client,
                            CefBrowserSettings& settings,
							CefRefPtr<CefBrowser>& newBrowser)
{
	std::string urlStr = url;
	if(urlStr.find("chrome-devtools:") == std::string::npos) {
		TransparentWnd* tp=(TransparentWnd*)this->win;
		tp->RunApp("browser/index.app",url);
	}
	else{
		return false;
	}
	return true;
}

bool MyHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& failedUrl,
                                CefString& errorText)
{
  if(errorCode == ERR_CACHE_MISS) {
    // Usually caused by navigating to a page with POST data via back or
    // forward buttons.
    errorText = "<html><head><title>Expired Form Data</title></head>"
                "<body><h1>Expired Form Data</h1>"
                "<h2>Your form request has expired. "
                "Click reload to re-submit the form data.</h2></body>"
                "</html>";
  } else {
    // All other messages.
    std::stringstream ss;
    ss <<       "<html><head><title>Load Failed</title></head>"
                "<body><h1>Load Failed</h1>"
                "<h2>Load of URL " << std::string(failedUrl) <<
                " failed with error code " << static_cast<int>(errorCode) <<
                ".</h2></body>"
                "</html>";
    errorText = ss.str();
  }
  return true;
}

void MyHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  REQUIRE_UI_THREAD();

  AutoLock lock_scope(this);
  if(!m_Browser.get())
  {
    // We need to keep the main child window, but not popup windows
    m_Browser = browser;
    m_BrowserHwnd = browser->GetWindowHandle();
  }
}
bool MyHandler::GetViewRect(CefRefPtr<CefBrowser> browser,
                           CefRect& rect)
  {
    REQUIRE_UI_THREAD();

    // The simulated screen and view rectangle are the same. This is necessary
    // for popup menus to be located and sized inside the view.
	if(win){
		TransparentWnd* winHandler=(TransparentWnd*)win;
		rect.x = winHandler->x;
		rect.y = winHandler->y;
		rect.width = width;
		rect.height = height;
	}
    return true;
  }
bool MyHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                            int viewX,
                            int viewY,
                            int& screenX,
                            int& screenY)
{
	REQUIRE_UI_THREAD();

	// Convert the point from view coordinates to actual screen coordinates.
	POINT screen_pt = {viewX, viewY};
	if(win){
		TransparentWnd* winHandler=(TransparentWnd*)win;
		screen_pt.x+=winHandler->x;
		screen_pt.y+=winHandler->y;
	}	
	screenX = screen_pt.x;
	screenY = screen_pt.y;
	return true;
}

void MyHandler::SetSize(int width, int height)
{
  m_Browser->SetSize(PET_VIEW, width, height);
  this->width=width;
  this->height=height;
}

void MyHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                       PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer)
{
	if(win){
		TransparentWnd* winHandler=(TransparentWnd*)win;
		winHandler->Render(buffer);
	}
	return;
}
bool MyHandler::OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text) {
	if(win){
		TransparentWnd* winHandler=(TransparentWnd *)win;
		winHandler->ShowTip(text);
		//winHandler->ExecJS(ss.str());
		return true;
	}
	return false;
}

void MyHandler::OnHttpResponse(const CefString& url, 
	const CefString& mimeType, 
	int responseCode, 
	int64 contentLength, 
	int64 startTime, 
	int64 endTime) {
	string s=url.ToString();
	if(s.find("AlloyDesktop_download=")!=string::npos){
		this->contentLength=contentLength;
	}
}

void MyHandler::OnResourceResponse(CefRefPtr<CefBrowser> browser,
                                  const CefString& url,
                                  CefRefPtr<CefResponse> response,
                                  CefRefPtr<CefContentFilter>& filter) {
	string s=url.ToString();
	string s1="AlloyDesktop_download=";
	if(s.find(s1)!=string::npos){
		int index1=s.find(s1);
		s=s.substr(index1+s1.length());
		ClientFilterHandler * myfilter=new ClientFilterHandler();
		myfilter->SetWinHandler((void *)this->win,s,contentLength);
		filter=myfilter;
	}
}

bool MyHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefRequest> request,
                                     CefString& redirectUrl,
                                     CefRefPtr<CefStreamReader>& resourceStream,
                                     CefRefPtr<CefResponse> response,
                                     int loadFlags)
{
  REQUIRE_IO_THREAD();
  std::string url = request->GetURL();

  if(url == "http://tests/test") {
    // Show the uiapp contents
    //resourceStream = GetBinaryResourceReader(IDS_TEST);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } 
  if(url == "http://tests/test1") {
    // Show the uiapp contents
    //resourceStream = GetBinaryResourceReader(IDS_TEST1);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } 
  else if(strstr(url.c_str(), "/book512.png") != NULL) {
    // Any time we find "ps_logo2.png" in the URL substitute in our own image
    //resourceStream = GetBinaryResourceReader(IDS_BG);
    response->SetMimeType("image/png");
    response->SetStatus(200);
  }
  else if(url == "http://tests/uiapp") {
    resourceStream = GetBinaryResourceReader(IDS_UIPLUGIN);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/osrapp") {
    // Show the osrapp contents
    resourceStream = GetBinaryResourceReader(IDS_OSRPLUGIN);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/localstorage") {
    // Show the localstorage contents
    resourceStream = GetBinaryResourceReader(IDS_LOCALSTORAGE);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/xmlhttprequest") {
    // Show the xmlhttprequest HTML contents
    resourceStream = GetBinaryResourceReader(IDS_XMLHTTPREQUEST);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/domaccess") {
    // Show the domaccess HTML contents
    resourceStream = GetBinaryResourceReader(IDS_DOMACCESS);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(strstr(url.c_str(), "/logoball.png") != NULL) {
    // Load the "logoball.png" image resource.
    resourceStream = GetBinaryResourceReader(IDS_LOGOBALL);
    response->SetMimeType("image/png");
    response->SetStatus(200);
  } else if(url == "http://tests/modalmain") {
    resourceStream = GetBinaryResourceReader(IDS_MODALMAIN);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/modaldialog") {
    resourceStream = GetBinaryResourceReader(IDS_MODALDIALOG);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/transparency") {
    resourceStream = GetBinaryResourceReader(IDS_TRANSPARENCY);
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(strstr(url.c_str(), ".app") != NULL){
    response->SetMimeType("text/html");
    response->SetStatus(200);
  } else if(url == "http://tests/plugin") {
    std::string html =
        "<html><body>\n"
        "Client Plugin loaded by Mime Type:<br>\n"
        "<embed type=\"application/x-client-plugin\" width=600 height=40>\n"
        "<br><br>Client Plugin loaded by File Extension:<br>\n"
        "<embed src=\"test.xcp\" width=600 height=40>\n"
        // Add some extra space below the plugin to allow scrolling.
        "<div style=\"height:1000px;\">&nbsp;</div>\n"
        "</body></html>";
  
    resourceStream =
        CefStreamReader::CreateForData((void*)html.c_str(), html.size());
    response->SetMimeType("text/html");
    response->SetStatus(200);
  }
  //����Ϊ��������
  {
	  CefRequest::HeaderMap hm;
	  request->GetHeaderMap(hm);
	  CefRequest::HeaderMap::iterator it=hm.find("Referers");
	  //CefString s("");
	  if(it!=hm.end()){
		  CefRequest::HeaderMap::iterator it2=hm.find("Referrer");
		  it2->second=it->second;
		  hm.erase(it);
	  }
	  request->SetHeaderMap(hm);
  }
  return false;
}

void MyHandler::SetWin(long win){
	this->win=win;
}

void MyHandler::NotifyDownloadBegin(const CefString& fileName, int64 contentLength)
{
  SetLastDownloadFile(fileName);
  //SendNotification(NOTIFY_DOWNLOAD_COMPLETE);
}

void MyHandler::NotifyDownloadComplete(const CefString& fileName)
{
  SetLastDownloadFile(fileName);
  SendNotification(NOTIFY_DOWNLOAD_COMPLETE);
}

void MyHandler::NotifyDownloadError(const CefString& fileName)
{
  SetLastDownloadFile(fileName);
  SendNotification(NOTIFY_DOWNLOAD_ERROR);
}

void MyHandler::SendNotification(NotificationType type)
{
  UINT id;
  switch(type)
  {
  case NOTIFY_CONSOLE_MESSAGE:
    id = ID_WARN_CONSOLEMESSAGE;
    break;
  case NOTIFY_DOWNLOAD_COMPLETE:
    id = ID_WARN_DOWNLOADCOMPLETE;
    break;
  case NOTIFY_DOWNLOAD_ERROR:
    id = ID_WARN_DOWNLOADERROR;
    break;
  default:
    return;
  }
  PostMessage(m_MainHwnd, WM_COMMAND, id, 0);
}

bool MyHandler::OnDragStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
  REQUIRE_UI_THREAD();

  // Forbid dragging of image files.
  if (dragData->IsFile()) {
    std::string fileExt = dragData->GetFileExtension();
    //if (fileExt == ".png" || fileExt == ".jpg" || fileExt == ".gif")
      //return true;
  }

  return false;
}

bool MyHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
  REQUIRE_UI_THREAD();

  // Forbid dragging of link URLs.
  if (dragData->IsLink())
    return true;

  return false;
}


void MyHandler::OnCursorChange(CefRefPtr<CefBrowser> browser,CefCursorHandle cursor)
{
	REQUIRE_UI_THREAD();

	// Change the plugin window's cursor.
	if(win){
		SetClassLong(((TransparentWnd *)win)->renderWindow, GCL_HCURSOR,
			static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
		SetCursor(cursor);
	}
}

void MyHandler::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefDOMNode> node)
{
  REQUIRE_UI_THREAD();

  // Set to true if a form element has focus.
  m_bFormElementHasFocus = (node.get() && node->IsFormControlElement());
}

// Execute with the specified argument list and return value.  Return true if
// the method was handled.
bool MyHandler::Execute(const CefString& name,
	CefRefPtr<CefV8Value> object,
	const CefV8ValueList& arguments,
	CefRefPtr<CefV8Value>& retval,
	CefString& exception)
{
	if(arguments.size() < 1)
		return false;

	TransparentWnd* winHandler=(TransparentWnd*)static_cast<long>(arguments[0]->GetIntValue());
	if(name == "close")
	{
		if(winHandler){
			if(winHandler->downloadHandler){
				//winHandler->downloadHandler->SetWinHandler(NULL);
				winHandler->downloadHandler=NULL;
			}
		}
		delete winHandler;
		return true;
	}
	else if(name == "drag")
	{
		winHandler->StartDrag();
		/*winHandler->Drag();*/
		return true;
	}
	else if(name == "enableDrag")
	{
		winHandler->EnableDrag();
		return true;
	}
	else if(name == "render")
	{
		winHandler->Render();
		return true;
	}
	else if(name == "stopDrag")
	{
		winHandler->isDrag=false;
		return true;
	}
	else if(name == "loadUrl")
	{
		CefString url = arguments[1]->GetStringValue();
		winHandler->SetUrl(url);
		return true;
	}
	else if(name == "setSize")
	{
		int w = static_cast<int>(arguments[1]->GetIntValue());
		int h = static_cast<int>(arguments[2]->GetIntValue());
		winHandler->SetSize(w, h);
		return true;
	}
	else if(name=="getSaveName"){
		CefString s = arguments[1]->GetStringValue();
		retval = CefV8Value::CreateString(winHandler->GetSaveName(s));
		return true;
	}
	else if(name=="getOpenName"){
		CefString s = arguments[1]->GetStringValue();
		retval = CefV8Value::CreateString(winHandler->GetOpenName(s));
		return true;
	}
	else if(name=="getOpenNames"){
		CefString s = arguments[1]->GetStringValue();
		retval = CefV8Value::CreateString(winHandler->GetOpenNames(s));
		return true;
	}
	else if(name=="getFileSize"){
		CefString s = arguments[1]->GetStringValue();
		retval = CefV8Value::CreateDouble(GetFileSize(winHandler->TranslatePath(s).ToWString().data()));
		return true;
	}
	else if(name=="getCurrentDirectory"){
		retval = CefV8Value::CreateString(winHandler->GetCurrentDirectory());
		return true;
	}
	else if(name=="getFolder"){
		retval = CefV8Value::CreateString(winHandler->GetFolder());
		return true;
	}
	else if(name == "move")
	{
		int x = static_cast<int>(arguments[1]->GetIntValue());
		int y = static_cast<int>(arguments[2]->GetIntValue());
		winHandler->Move(x, y);
		return true;
	}
	else if(name == "quit")
	{
		PostQuitMessage(0);
		return true;
	}
	else if(name == "max")
	{
		winHandler->Max();
		return true;
	}
	else if(name == "mini")
	{
		winHandler->Mini();
		return true;
	}
	else if(name == "hide")
	{
		winHandler->Hide();
		return true;
	}
	else if(name == "restore")
	{
		winHandler->Restore();
		return true;
	}
	else if(name == "setTopMost")
	{
		winHandler->SetTopMost();
		return true;
	}
	else if(name == "setWindowStyle")
	{
		UINT exStyle = static_cast<int>(arguments[1]->GetIntValue());
		winHandler->SetWindowStyle(exStyle);
		return true;
	}
	else if(name == "createWindow")
	{
		CefString url = arguments[1]->GetStringValue();
		UINT exStyle = static_cast<int>(arguments[2]->GetIntValue());
		TransparentWnd* win=new TransparentWnd();
		bool isTransparent = arguments[3]->GetBoolValue();
		CefString s = arguments[4]->GetStringValue();
		win->SetReadyHandler(s);
		HINSTANCE hInstance = GetModuleHandle(0);
		win->CreateBrowserWindow(url, exStyle, isTransparent);
		retval = CefV8Value::CreateInt((long)(win));
		//winHandler->CreateBrowser(url);
		return true;
	}
	else if(name == "createWindowBase")
	{
		CefString url = arguments[1]->GetStringValue();
		UINT exStyle = static_cast<int>(arguments[2]->GetIntValue());
		TransparentWnd* win=new TransparentWnd();
		bool isTransparent = arguments[3]->GetBoolValue();
		CefString s = arguments[4]->GetStringValue();
		win->SetReadyHandler(s);
		HINSTANCE hInstance = GetModuleHandle(0);
		win->CreateBrowserWindowBase(url, exStyle, isTransparent);
		retval = CefV8Value::CreateInt((long)(win));
		//winHandler->CreateBrowser(url);
		return true;
	}
	else if(name == "createBrowser")
	{
		CefString url = arguments[1]->GetStringValue();
		winHandler->CreateBrowser(url);
	}
	else if(name == "browse")
	{
		CefString url = arguments[1]->GetStringValue();
		winHandler->Browse(url);
	}
	else if(name == "toImage")
	{
		CefString path = arguments[1]->GetStringValue();
		winHandler->ToImage(path);
	}
	else if(name == "toImageEx")
	{
		CefString path = arguments[1]->GetStringValue();
		int x = static_cast<int>(arguments[2]->GetIntValue());
		int y = static_cast<int>(arguments[3]->GetIntValue());
		int width = static_cast<int>(arguments[4]->GetIntValue());
		int height = static_cast<int>(arguments[5]->GetIntValue());
		winHandler->ToImageEx(path,x,y,width,height);
	}
	else if(name == "bringToTop")
	{
		winHandler->BringToTop();
	}
	else if(name == "focus")
	{
		winHandler->Focus();
	}
	else if(name == "getPos")
	{
		retval = CefV8Value::CreateObject(NULL,NULL);
		// Add a string parameter to the new V8 object.
		retval->SetValue("x", CefV8Value::CreateInt(winHandler->x),V8_PROPERTY_ATTRIBUTE_NONE);
		// Add a function to the new V8 object.
		retval->SetValue("y", CefV8Value::CreateInt(winHandler->y),V8_PROPERTY_ATTRIBUTE_NONE);
		return true;
	}
	else if(name == "getScreenSize")
	{
		retval = CefV8Value::CreateObject(NULL,NULL);
		// Add a string parameter to the new V8 object.
		retval->SetValue("width", CefV8Value::CreateInt(GetSystemMetrics(SM_CXFULLSCREEN)),V8_PROPERTY_ATTRIBUTE_NONE);
		// Add a function to the new V8 object.
		retval->SetValue("height", CefV8Value::CreateInt(GetSystemMetrics(SM_CYFULLSCREEN)+GetSystemMetrics(SM_CYCAPTION)),V8_PROPERTY_ATTRIBUTE_NONE);
		return true;
	}
	else if(name == "readFile")
	{
		CefString s = arguments[1]->GetStringValue().ToString();
		retval = CefV8Value::CreateString(winHandler->ReadFile(s));
		// Add a string parameter to the new V8 object.
		return true;
	}
	else if(name == "writeFile")
	{
		CefString path = arguments[1]->GetStringValue().ToString();
		CefString s = arguments[2]->GetStringValue().ToString();
		retval = CefV8Value::CreateBool(winHandler->WriteFile(path,s.ToString().c_str()));
		// Add a string parameter to the new V8 object.
		return true;
	}
	else if(name=="runApp"){
		CefString appName = arguments[1]->GetStringValue();
		CefString param = arguments[2]->GetStringValue();
		TransparentWnd* win=new TransparentWnd();
		win->RunAppIn(appName,param,winHandler->GetUrl());
		retval = CefV8Value::CreateInt((long)(win));
		return true;
	}
	else if(name=="runAppEx"){
		CefString appName = arguments[1]->GetStringValue();
		CefString param = arguments[2]->GetStringValue();
		winHandler->RunApp(appName,param,winHandler->GetUrl());
	}
	else if(name=="ready"){
		winHandler->Ready();
	}
	else if(name=="reload"){
		winHandler->Reload();
	}
	else if(name=="reloadIgnoreCache"){
		winHandler->ReloadIgnoreCache();
	}
	else if(name=="readStringEx"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int l = static_cast<int>(arguments[2]->GetIntValue());
		CHAR *s1=new CHAR[l+1];
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->ReadString(s1,NULL,l);
		CefString s(s1);
		retval=CefV8Value::CreateString(s);
		delete []s1;
		return true;
	}
	else if(name=="writeStringEx"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefString s = arguments[2]->GetStringValue();
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->WriteString(s.ToString().c_str());
	}
	else if(name=="readWStringEx"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int l = static_cast<int>(arguments[2]->GetIntValue());
		TCHAR *s1=new TCHAR[l+1];
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->ReadWString(s1,NULL,l*2);
		CefString s(s1);
		retval=CefV8Value::CreateString(s);
		delete []s1;
		return true;
	}
	else if(name=="writeWStringEx"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefString s = arguments[2]->GetStringValue();
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->WriteWString(s.ToWString().data());
	}
	else if(name=="readGB"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int l = static_cast<int>(arguments[2]->GetIntValue());
		CHAR *s1=new CHAR[l+1];
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->ReadStringSimple(s1,l);
		CefString s;
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, s1, -1, NULL, 0);
		WCHAR *s2=new WCHAR[dwNum];
		::MultiByteToWideChar(CP_ACP,0,s1,-1,s2,dwNum);
		s=s2;
		retval=CefV8Value::CreateString(s);
		delete []s2;
		delete []s1;
		return true;
	}
	else if(name=="readString"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int l = static_cast<int>(arguments[2]->GetIntValue());
		CHAR *s1=new CHAR[l+1];
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->ReadStringSimple(s1,l);
		CefString s;
		s=s1;
		retval=CefV8Value::CreateString(s);
		delete []s1;
		return true;
	}
	else if(name=="writeString"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefString s = arguments[2]->GetStringValue();
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->WriteStringSimple(s.ToString().c_str());
	}
	else if(name=="readWString"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int l = static_cast<int>(arguments[2]->GetIntValue());
		TCHAR *s1=new TCHAR[l+1];
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->ReadWStringSimple(s1,l*2);
		CefString s(s1);
		retval=CefV8Value::CreateString(s);
		//delete s1;
		return true;
	}
	else if(name=="writeWString"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefString s = arguments[2]->GetStringValue();
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		pStream->WriteWStringSimple(s.ToWString().data());
	}
	else if(name=="utf82gb"){
		CefString s = arguments[1]->GetStringValue();
		char* s1=U2G(s.ToString().c_str());
		retval=CefV8Value::CreateString(s1);
		delete s1;
		return true;
	}
	else if(name=="gb2utf8"){
		CefString s = arguments[1]->GetStringValue();
		char* s1=G2U(s.ToString().c_str());
		retval=CefV8Value::CreateString(s1);
		delete s1;
		return true;
	}
	else if(name=="setStreamPos"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		int i = static_cast<int>(arguments[2]->GetIntValue());
		pStream->SetPosition(i);
	}
	else if(name=="getStreamPos"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		int b=pStream->GetPosition();
		retval=CefV8Value::CreateInt(b);
		return true;
	}
	else if(name=="readInt"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		int b=pStream->ReadULong();
		retval=CefV8Value::CreateInt(b);
		return true;
	}
	else if(name=="writeInt"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int i = static_cast<int>(arguments[2]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		retval=CefV8Value::CreateBool(pStream->WriteULong(i)>0);
		return true;
	}
	else if(name=="readByte"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		BYTE b=pStream->ReadByte();
		retval=CefV8Value::CreateInt(b);
		return true;
	}
	else if(name=="writeByte"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		BYTE i = static_cast<BYTE>(arguments[2]->GetIntValue());
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		retval=CefV8Value::CreateBool(pStream->WriteByte(i)>0);
		return true;
	}
	else if(name=="writeBytes"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefRefPtr<CefV8Value> value=arguments[2];
		if(value->IsArray()){
			int len=value->GetArrayLength();
			BYTE* p=new BYTE[len];
			for (int i = 0; i < len; ++i) {
				p[i]=value->GetValue(i)->GetIntValue();
			}
			AmfStream* pStream=winHandler->pStream;
			if(id){
				pStream=((AmfStream*)id);
			}
			retval=CefV8Value::CreateBool(pStream->WriteBytes(p,len)>0);
			delete []p;
		}
		else if (value->IsObject()) {
			int len=value->GetValue("length")->GetIntValue();
			BYTE* p=new BYTE[len];
			for (int i = 0; i < len; ++i) {
				p[i]=value->GetValue(i)->GetIntValue();
			}
			AmfStream* pStream=winHandler->pStream;
			if(id){
				pStream=((AmfStream*)id);
			}
			retval=CefV8Value::CreateBool(pStream->WriteBytes(p,len)>0);
			delete []p;
		}
		return true;
	}
	else if(name=="readBytes"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int len=arguments[2]->GetIntValue();
		AmfStream* pStream=winHandler->pStream;
		if(id){
			pStream=((AmfStream*)id);
		}
		retval=CefV8Value::CreateArray();
		BYTE* p=new BYTE[len];
		pStream->ReadBytes(p,len);
		for (int i = 0; i < len; ++i) {
			retval->SetValue(i,CefV8Value::CreateInt(p[i]));	
		}
		delete []p;
		return true;
	}
	else if(name=="enableTransparent"){
		UINT exStyle = static_cast<int>(arguments[1]->GetIntValue());
		winHandler->EnableTransparent(exStyle);
	}
	else if(name == "getSize")
	{
		retval = CefV8Value::CreateObject(NULL,NULL);
		// Add a string parameter to the new V8 object.
		retval->SetValue("width", CefV8Value::CreateInt(winHandler->width),V8_PROPERTY_ATTRIBUTE_NONE);
		// Add a function to the new V8 object.
		retval->SetValue("height", CefV8Value::CreateInt(winHandler->height),V8_PROPERTY_ATTRIBUTE_NONE);
		return true;
	}
	else if(name == "setTitle")
	{
		CefString title = arguments[1]->GetStringValue();
		winHandler->SetTitle(title);
	}
	else if(name == "showDev"){
		CefBrowser* browser=winHandler->g_handler->GetBrowser();
		browser->ShowDevTools();
	}
	else if(name == "shutdown"){
		bool flag=arguments[1]->GetBoolValue();
		Shutdown(flag);
	}
	else if(name == "reboot"){
		bool flag=arguments[1]->GetBoolValue();
		Reboot(flag);
	}
	else if(name == "logoff"){
		bool flag=arguments[1]->GetBoolValue();
		Logoff(flag);
	}
	else if(name == "createTcpServer"){
		TcpServer* tcpServer=new TcpServer();
		tcpServer->winHandler=winHandler;
		retval=CefV8Value::CreateInt((long)tcpServer);
		tcpServer->init();
		return true;
	}
	else if(name == "shutdownTcpServer"){
		TcpServer* server = (TcpServer*)static_cast<int>(arguments[1]->GetIntValue());
		delete server;
	}
	else if(name=="sendTcpMsg"){
		TcpServer* server = (TcpServer*)static_cast<int>(arguments[1]->GetIntValue());
		CefString msg=arguments[2]->GetStringValue();
		server->sendMsg(msg.ToString().c_str());
	}
	else if(name == "connect"){
		CefString ip=arguments[1]->GetStringValue();
		CefString uid=arguments[2]->GetStringValue();
		winHandler->p2p.Connect(ip.ToString().c_str(),uid.ToString().c_str());
	}
	else if(name == "connectByHost"){
		CefString hostName=arguments[1]->GetStringValue();
		CefString uid=arguments[2]->GetStringValue();
		winHandler->p2p.ConnectByHost(hostName.ToString().c_str(),uid.ToString().c_str());
	}
	else if(name == "getUsers"){
		winHandler->GetUsers();
	}
	else if(name == "sendMessage"){
		CefString userName=arguments[1]->GetStringValue();
		CefString msg=arguments[2]->GetStringValue();
		winHandler->p2p.SendMessageTo(userName.ToString().c_str(),msg.ToString().c_str());
	}
	else if(name == "sendMsgToServer"){
		CefString msg=arguments[1]->GetStringValue();
		winHandler->p2p.SendMessageToServer(msg.ToString().c_str());
	}
	else if(name == "sendMsgToIP"){
		CefString ip=arguments[1]->GetStringValue();
		short port=arguments[2]->GetIntValue();
		CefString msg=arguments[3]->GetStringValue();
		winHandler->p2p.SendMessageToEx(ip.ToString().c_str(),port,msg.ToString().c_str());
	}
	else if(name == "setTaskIcon"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		CefString path=arguments[2]->GetStringValue();
		CefString title=arguments[3]->GetStringValue();
		winHandler->SetTaskIcon(id,path,title);
	}
	else if(name == "findFiles"){
		CefString path=arguments[1]->GetStringValue();
		bool flag=arguments[2]->GetBoolValue();
		retval=CefV8Value::CreateString(find(winHandler->TranslatePath(path).ToWString(),flag));
		return true;
	}
	else if(name == "deleteDir"){
		CefString path=arguments[1]->GetStringValue();
		retval=CefV8Value::CreateBool(DeleteDirectory(winHandler->TranslatePath(path).ToWString()));
		return true;
	}
	else if(name == "deleteDirFiles"){
		CefString path=arguments[1]->GetStringValue();
		retval=CefV8Value::CreateBool(DeleteDirectoryFiles(winHandler->TranslatePath(path).ToWString()));
		return true;
	}
	else if(name == "createDir"){
		CefString path=arguments[1]->GetStringValue();
		retval=CefV8Value::CreateBool(CreateDirectory(winHandler->TranslatePath(path).ToWString()));
		return true;
	}
	else if(name == "getLastMessage"){
		retval=CefV8Value::CreateString(winHandler->getLastMessage());
		return true;
	}
	else if(name == "getMessage"){
		CefString guid=arguments[1]->GetStringValue();
		retval=CefV8Value::CreateString(winHandler->getMessage(guid));
		return true;
	}
	else if(name == "createMemory"){
		CefString name=arguments[1]->GetStringValue();
		CefString filename=arguments[2]->GetStringValue();
		int size = static_cast<int>(arguments[3]->GetIntValue());
		retval=CefV8Value::CreateInt((int)winHandler->CreateMemory(name,filename,size));
		return true;
	}
	else if(name=="download"){
		CefString url=arguments[1]->GetStringValue();
		CefString filename=arguments[2]->GetStringValue();
		winHandler->Download(url,filename);
	}
	else if(name == "deleteMemory"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		winHandler->DeleteMemory((CSFMServer*)id);
		return true;
	}
	else if(name == "createStream"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		retval=CefV8Value::CreateInt((int)winHandler->CreateStream((CSFMServer*)id));
		return true;
	}
	else if(name == "deleteStream"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		winHandler->DeleteStream((AmfStream*)id);
		return true;
	}
	else if(name == "delTaskIcon"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		winHandler->DelTaskIcon(id);
	}
	else if(name == "saveImageFromStream"){
		int id = static_cast<int>(arguments[1]->GetIntValue());
		int width = static_cast<int>(arguments[2]->GetIntValue());
		int height = static_cast<int>(arguments[3]->GetIntValue());
		CefString path=arguments[4]->GetStringValue();
		AmfStream* pStream=(AmfStream*)id;
		winHandler->SaveImageFromStream(path,pStream,width,height);
	}
	else if(name == "saveImageFromBase64"){
		CefString s=arguments[1]->GetStringValue();
		CefString path=arguments[2]->GetStringValue();
		string s1=s.ToString();
		int index=s1.find(',');
		s1=s1.substr(index+1);
		int imageSize = int((s1.length()/3)+1)*4;
		char* t=new char[imageSize];
		base64_decode(s1.c_str(),s1.length(), t, &imageSize); // using the base64
		CSFMServer *ps=winHandler->CreateMemory("test",path,imageSize);
		AmfStream *ps1=winHandler->CreateStream(ps);
		ps1->WriteBytes((PBYTE)t,imageSize);
		delete ps1;
		delete ps;
		delete t;
		/*Bitmap* pbm=GetImageFromBase64(s1);
		SaveBitmap(pbm,path);
		delete pbm;*/
	}
	else if(name == "getIP"){
		CefString ip=TcpServer::GetLocalIP();
		retval = CefV8Value::CreateString(ip);
		return true;
	}
	else if(name == "getIPAndPort"){
		CefString ip=winHandler->p2p.IP;
		unsigned short port=winHandler->p2p.port;
		retval = CefV8Value::CreateObject(NULL,NULL);
		// Add a string parameter to the new V8 object.
		retval->SetValue("ip", CefV8Value::CreateString(ip),V8_PROPERTY_ATTRIBUTE_NONE);
		// Add a function to the new V8 object.
		retval->SetValue("port", CefV8Value::CreateInt(port),V8_PROPERTY_ATTRIBUTE_NONE);
		return true;
	}
	else if(name=="shellExecute"){
		CefString operation=arguments[1]->GetStringValue();
		CefString path=arguments[2]->GetStringValue();
		CefString params=arguments[3]->GetStringValue();
		CefString directory=arguments[4]->GetStringValue();
		int showCmd = static_cast<int>(arguments[5]->GetIntValue());
		ShellExecute(NULL,operation.ToWString().data(),path.ToWString().data(),params.ToWString().data(),directory.ToWString().data(),showCmd);
	}
	else if(name=="winExec"){
		CefString cmd=arguments[1]->GetStringValue();
		int showCmd = static_cast<int>(arguments[2]->GetIntValue());
		WinExec(cmd.ToString().data(),showCmd);
	}
	else if(name=="execJS"){
		CefString js=arguments[1]->GetStringValue();
		try{
			winHandler->ExecJS(js);
		}
		catch(Exception e){
			e;
		}
	}
	else if(name=="startProxy"){
		int port=static_cast<int>(arguments[1]->GetIntValue());
		MelodyProxy* melodyProxy=new MelodyProxy(winHandler,port,true);
		melodyProxy->StartProxyServer();
		retval=CefV8Value::CreateInt((long)melodyProxy);
	}
	else if(name=="replaceRequest"){
		int handler=static_cast<int>(arguments[1]->GetIntValue());
		CefString request=arguments[2]->GetStringValue();
		winHandler->replaceRequest(request, (LPVOID)handler);
	}
	else if(name=="replaceResponse"){
		int handler=static_cast<int>(arguments[1]->GetIntValue());
		CefString content=arguments[2]->GetStringValue();
		winHandler->replaceResponse(content, (LPVOID)handler);
	}
	else if(name=="response"){
		int handler=static_cast<int>(arguments[1]->GetIntValue());
		CefString content=arguments[2]->GetStringValue();
		winHandler->response(content, (LPVOID)handler);
	}
	else if(name=="cancelReplaceResponse"){
		int handler=static_cast<int>(arguments[1]->GetIntValue());
		winHandler->cancelReplaceResponse((LPVOID)handler);
	}
	else if(name=="cancelResponse"){
		int handler=static_cast<int>(arguments[1]->GetIntValue());
		winHandler->cancelResponse((LPVOID)handler);
	}
	else if(name=="closeProxy"){
		MelodyProxy* proxy=(MelodyProxy *)static_cast<long>(arguments[1]->GetIntValue());
		proxy->CloseServer();
	}
	return false;
}

void InitCallback()
{
  // Register a V8 extension with the below JavaScript code that calls native
  // methods implemented in ClientV8ExtensionHandler.
  std::string code = "var AlloyDesktop;"
    "if (!AlloyDesktop)"
    "  AlloyDesktop = {};"
    "(function() {"
    "  AlloyDesktop.close = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function close(handler);"
    "    return close(handler);"
    "  };"
    "  AlloyDesktop.stopDrag = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function stopDrag(handler);"
	"    return stopDrag(handler);"
    "  };"
    "  AlloyDesktop.setSize = function(w,h,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setSize(handler,w,h);"
	"    return setSize(handler,w,h);"
    "  };"
    "  AlloyDesktop.move = function(x,y,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function move(handler,x,y);"
	"    return move(handler,x,y);"
    "  };"
    "  AlloyDesktop.max = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function max(handler);"
	"    return max(handler);"
    "  };"
    "  AlloyDesktop.hide = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function hide(handler);"
	"    return hide(handler);"
    "  };"
    "  AlloyDesktop.mini = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function mini(handler);"
	"    return mini(handler);"
    "  };"
    "  AlloyDesktop.restore = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function restore(handler);"
	"    return restore(handler);"
    "  };"
    "  AlloyDesktop.drag = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function drag(handler);"
	"    return drag(handler);"
    "  };"
    "  AlloyDesktop.render = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function render(handler);"
	"    return render(handler);"
    "  };"
    "  AlloyDesktop.bringToTop = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function bringToTop(handler);"
	"    return bringToTop(handler);"
    "  };"
    "  AlloyDesktop.focus = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function focus(handler);"
	"    return focus(handler);"
    "  };"
    "  AlloyDesktop.loadUrl = function(url,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function loadUrl(handler,url);"
	"    return loadUrl(handler,url);"
    "  };"
    "  AlloyDesktop.getPos = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getPos(handler);"
	"    return getPos(handler);"
    "  };"
    "  AlloyDesktop.getSize = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getSize(handler);"
	"    return getSize(handler);"
    "  };"
    "  AlloyDesktop.enableDrag = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function enableDrag(handler);"
	"    return enableDrag(handler);"
    "  };"
    "  AlloyDesktop.quit = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function quit(handler);"
	"    return quit(handler);"
    "  };"
    "  AlloyDesktop.ready = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function ready(handler);"
	"    return ready(handler);"
    "  };"
    "  AlloyDesktop.createWindow = function(url,exStyle,isTransparent,readyHandler,handler) {"
	"    handler=handler?handler:window['handler'];"
	"    readyHandler=readyHandler||'';"
    "    native function createWindow(handler,url,exStyle,isTransparent,readyHandler);"
	"    return createWindow(handler,url,exStyle,isTransparent,readyHandler);"
    "  };"
    "  AlloyDesktop.createWindowBase = function(url,exStyle,isTransparent,readyHandler,handler) {"
	"    handler=handler?handler:window['handler'];"
	"    readyHandler=readyHandler||'';"
    "    native function createWindowBase(handler,url,exStyle,isTransparent,readyHandler);"
	"    return createWindowBase(handler,url,exStyle,isTransparent,readyHandler);"
    "  };"
    "  AlloyDesktop.createBrowser = function(url,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function createBrowser(handler,url);"
	"    return createBrowser(handler,url);"
    "  };"
    "  AlloyDesktop.browse = function(url,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function browse(handler,url);"
	"    return browse(handler,url);"
    "  };"
    "  AlloyDesktop.setTitle = function(title,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setTitle(handler,title);"
	"    return setTitle(handler,title);"
    "  };"
    "  AlloyDesktop.getImage = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getImage(handler);"
	"    return getImage(handler);"
    "  };"
    "  AlloyDesktop.showDev = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function showDev(handler);"
	"    return showDev(handler);"
    "  };"
    "  AlloyDesktop.readFile = function(path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readFile(handler,path);"
	"    return readFile(handler,path);"
    "  };"
    "  AlloyDesktop.writeFile = function(path,s,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeFile(handler,path,s);"
	"    return writeFile(handler,path,s);"
    "  };"
    "  AlloyDesktop.getSaveName = function(filename,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getSaveName(handler,filename);"
	"    return getSaveName(handler,filename);"
    "  };"
    "  AlloyDesktop.runApp = function(appName,param,handler) {"
	"    handler=handler?handler:window['handler'];"
	"	 param=param?param:'';"
    "    native function runApp(handler,appName,param);"
	"    return runApp(handler,appName,param);"
    "  };"
    "  AlloyDesktop.runAppEx = function(appName,param,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function runAppEx(handler,appName,param);"
	"    return runAppEx(handler,appName,param);"
    "  };"
    "  AlloyDesktop.download = function(url,filename,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function download(handler,url,filename);"
	"    return download(handler,url,filename);"
    "  };"
    "  AlloyDesktop.getOpenName = function(filename,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getOpenName(handler,filename);"
	"    return getOpenName(handler,filename);"
    "  };"
    "  AlloyDesktop.getOpenNames = function(filename,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getOpenNames(handler,filename);"
	"    return JSON.parse(getOpenNames(handler,filename).replace(/\\\\/g,'\\\\\\\\'));"
    "  };"
    "  AlloyDesktop.getFileSize = function(filename,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getFileSize(handler,filename);"
	"    return getFileSize(handler,filename);"
    "  };"
    "  AlloyDesktop.getFolder = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getFolder(handler);"
	"    return getFolder(handler);"
    "  };"
    "  AlloyDesktop.reload = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function reload(handler);"
	"    return reload(handler);"
    "  };"
    "  AlloyDesktop.reloadIgnoreCache = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function reloadIgnoreCache(handler);"
	"    return reloadIgnoreCache(handler);"
    "  };"
    "  AlloyDesktop.setTopMost = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setTopMost(handler);"
	"    return setTopMost(handler);"
    "  };"
    "  AlloyDesktop.setWindowStyle = function(exStyle,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setWindowStyle(handler,exStyle);"
	"    return setWindowStyle(handler,exStyle);"
    "  };"
    "  AlloyDesktop.getSharePos = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getSharePos(handler);"
	"    return getSharePos(handler);"
    "  };"
    "  AlloyDesktop.setSharePos = function(i) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setSharePos(handler,i);"
	"    return setSharePos(handler,i);"
    "  };"
    "  AlloyDesktop.toImageEx = function(path,x,y,width,height,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function toImageEx(handler,path,x,y,width,height);"
	"    return toImageEx(handler,path,x,y,width,height);"
    "  };"
    "  AlloyDesktop.toImage = function(path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function toImage(handler,path);"
	"    return toImage(handler,path);"
    "  };"
    "  AlloyDesktop.shutdown = function(flag,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function shutdown(handler,flag);"
	"    return shutdown(handler,flag);"
    "  };"
    "  AlloyDesktop.reboot = function(flag,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function reboot(handler,flag);"
	"    return reboot(handler,flag);"
    "  };"
    "  AlloyDesktop.logoff = function(flag,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function logoff(handler,flag);"
	"    return logoff(handler,flag);"
    "  };"
    "  AlloyDesktop.connect = function(ip,uid,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function connect(handler,ip,uid);"
	"    return connect(handler,ip,uid);"
    "  };"
    "  AlloyDesktop.connectByHost = function(hostName,uid,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function connectByHost(handler,hostName,uid);"
	"    return connectByHost(handler,hostName,uid);"
    "  };"
    "  AlloyDesktop.getUsers = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getUsers(handler);"
	"    return getUsers(handler);"
    "  };"
    "  AlloyDesktop.sendMessage = function(userName,msg,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function sendMessage(handler,userName,msg);"
	"    return sendMessage(handler,userName,msg);"
    "  };"
    "  AlloyDesktop.sendMsgToIP = function(ip,port,msg,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function sendMsgToIP(handler,ip,port,msg);"
	"    return sendMsgToIP(handler,ip,port,msg);"
    "  };"
    "  AlloyDesktop.sendMsgToServer = function(msg,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function sendMsgToServer(handler,msg);"
	"    return sendMsgToServer(handler,msg);"
    "  };"
    "  AlloyDesktop.getIPAndPort = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getIPAndPort(handler);"
	"    return getIPAndPort(handler);"
    "  };"
    "  AlloyDesktop.setTaskIcon = function(id,path,title,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setTaskIcon(handler,id,path,title);"
	"    return setTaskIcon(handler,id,path,title);"
    "  };"
    "  AlloyDesktop.delTaskIcon = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function delTaskIcon(handler,id);"
	"    return delTaskIcon(handler,id);"
    "  };"
    "  AlloyDesktop.findFiles = function(path,flag,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function findFiles(handler,path,flag);"
	"    return JSON.parse(findFiles(handler,path,flag));"
    "  };"
    "  AlloyDesktop.deleteDir = function(path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function deleteDir(handler,path);"
	"    return deleteDir(handler,path);"
    "  };"
    "  AlloyDesktop.createDir = function(path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function createDir(handler,path);"
	"    return createDir(handler,path);"
    "  };"
    "  AlloyDesktop.deleteDirFiles = function(path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function deleteDirFiles(handler,path);"
	"    return deleteDirFiles(handler,path);"
    "  };"
    "  AlloyDesktop.getCurrentDirectory = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getCurrentDirectory(handler);"
	"    return getCurrentDirectory(handler);"
    "  };"
    "  AlloyDesktop.createMemory = function(name,filename,size,handler) {"
	"    handler=handler?handler:window['handler'];"
	"	 filename=filename?filename:'';"
    "    native function createMemory(handler,name,filename,size);"
	"    return createMemory(handler,name,filename,size);"
    "  };"
    "  AlloyDesktop.deleteMemory = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function deleteMemory(handler,id);"
	"    return deleteMemory(handler,id);"
    "  };"
    "  AlloyDesktop.createStream = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function createStream(handler,id);"
	"    return createStream(handler,id);"
    "  };"
    "  AlloyDesktop.deleteStream = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function deleteStream(handler,id);"
	"    return deleteStream(handler,id);"
    "  };"
    "  AlloyDesktop.readString = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readString(handler,id,l);"
	"    return readString(handler,id,l);"
    "  };"
    "  AlloyDesktop.readGB = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readGB(handler,id,l);"
	"    return readGB(handler,id,l);"
    "  };"
    "  AlloyDesktop.writeString = function(s,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeString(handler,id,s);"
	"    return writeString(handler,id,s);"
    "  };"
    "  AlloyDesktop.readWString = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readWString(handler,id,l);"
	"    return readWString(handler,id,l);"
    "  };"
    "  AlloyDesktop.writeWString = function(s,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeWString(handler,id,s);"
	"    return writeWString(handler,id,s);"
    "  };"
    "  AlloyDesktop.readStringEx = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readStringEx(handler,id,l);"
	"    return readStringEx(handler,id,l);"
    "  };"
    "  AlloyDesktop.writeStringEx = function(s,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeStringEx(handler,id,s);"
	"    return writeStringEx(handler,id,s);"
    "  };"
    "  AlloyDesktop.readWStringEx = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readWStringEx(handler,id,l);"
	"    return readWStringEx(handler,id,l);"
    "  };"
    "  AlloyDesktop.writeWStringEx = function(s,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeWStringEx(handler,id,s);"
	"    return writeWStringEx(handler,id,s);"
    "  };"
    "  AlloyDesktop.readInt = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readInt(handler,id);"
	"    return readInt(handler,id);"
    "  };"
    "  AlloyDesktop.writeInt = function(i,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeInt(handler,id,i);"
	"    return writeInt(handler,id,i);"
    "  };"
    "  AlloyDesktop.readByte = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readByte(handler,id);"
	"    return readByte(handler,id);"
    "  };"
    "  AlloyDesktop.writeByte = function(b,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeByte(handler,id,b);"
	"    return writeByte(handler,id,b);"
    "  };"
    "  AlloyDesktop.readBytes = function(l,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function readBytes(handler,id,l);"
	"    return readBytes(handler,id,l);"
    "  };"
    "  AlloyDesktop.writeBytes = function(arr,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function writeBytes(handler,id,arr);"
	"    return writeBytes(handler,id,arr);"
    "  };"
    "  AlloyDesktop.setStreamPos = function(pos,id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function setStreamPos(handler,id,pos);"
	"    return setStreamPos(handler,id,pos);"
    "  };"
    "  AlloyDesktop.getStreamPos = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getStreamPos(handler,id);"
	"    return getStreamPos(handler,id);"
    "  };"
    "  AlloyDesktop.getStreamPos = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getStreamPos(handler,id);"
	"    return getStreamPos(handler,id);"
    "  };"
    "  AlloyDesktop.saveImageFromStream = function(id,width,height,path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function saveImageFromStream(handler,id,width,height,path);"
	"    return saveImageFromStream(handler,id,width,height,path);"
    "  };"
    "  AlloyDesktop.saveImageFromBase64 = function(s,path,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function saveImageFromBase64(handler,s,path);"
	"    return saveImageFromBase64(handler,s,path);"
    "  };"
    "  AlloyDesktop.getScreenSize = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getScreenSize(handler);"
	"    return getScreenSize(handler);"
    "  };"
    "  AlloyDesktop.shellExecute = function(operation,path,params,directory,cmdShow,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function shellExecute(handler,operation,path,params,directory,cmdShow);"
	"    return shellExecute(handler,operation,path,params,directory,cmdShow);"
    "  };"
    "  AlloyDesktop.winExec = function(cmd,cmdShow,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function winExec(handler,cmd,cmdShow);"
	"    return winExec(handler,cmd,cmdShow);"
    "  };"
    "  AlloyDesktop.execJS = function(js,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function execJS(handler,js);"
	"    return execJS(handler,js);"
    "  };"
    "  AlloyDesktop.createTcpServer = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function createTcpServer(handler);"
	"    return createTcpServer(handler);"
    "  };"
    "  AlloyDesktop.shutdownTcpServer = function(id,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function shutdownTcpServer(handler,id);"
	"    return shutdownTcpServer(handler,id);"
    "  };"
    "  AlloyDesktop.sendTcpMsg = function(id,msg,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function sendTcpMsg(handler,id,msg);"
	"    return sendTcpMsg(handler,id,msg);"
    "  };"
    "  AlloyDesktop.execJS = function(js,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function execJS(handler,js);"
	"    return execJS(handler,js);"
    "  };"
    "  AlloyDesktop.getIP = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getIP(handler);"
	"    return getIP(handler);"
    "  };"
    "  AlloyDesktop.getLastMessage = function(handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getLastMessage(handler);"
	"    return getLastMessage(handler);"
    "  };"
     "  AlloyDesktop.getMessage = function(timestamp,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function getMessage(handler,timestamp);"
	"    return getMessage(handler,timestamp);"
    "  };"
   "  AlloyDesktop.startProxy = function(port,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function startProxy(handler,port);"
	"    return startProxy(handler,port);"
    "  };"
    "  AlloyDesktop.closeProxy = function(proxy,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function closeProxy(handler,proxy);"
	"    return closeProxy(handler,proxy);"
    "  };"
    "  AlloyDesktop.replaceRequest = function(responseHandler,request,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function replaceRequest(handler,responseHandler,request);"
	"    return replaceRequest(handler,responseHandler,request);"
    "  };"
    "  AlloyDesktop.replaceResponse = function(responseHandler,response,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function replaceResponse(handler,responseHandler,response);"
	"    return replaceResponse(handler,responseHandler,response);"
    "  };"
    "  AlloyDesktop.response = function(responseHandler,responseContent,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function response(handler,responseHandler,responseContent);"
	"    return response(handler,responseHandler,responseContent);"
    "  };"
    "  AlloyDesktop.cancelReplaceResponse = function(responseHandler,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function cancelReplaceResponse(handler,responseHandler);"
	"    return cancelReplaceResponse(handler,responseHandler);"
    "  };"
    "  AlloyDesktop.cancelResponse = function(responseHandler,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function cancelResponse(handler,responseHandler);"
	"    return cancelResponse(handler,responseHandler);"
    "  };"
    "  AlloyDesktop.gb2utf8 = function(s,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function gb2utf8(handler,s);"
	"    return gb2utf8(handler,s);"
    "  };"
    "  AlloyDesktop.utf82gb = function(s,handler) {"
	"    handler=handler?handler:window['handler'];"
    "    native function utf82gb(handler,s);"
	"    return utf82gb(handler,s);"
    "  };"
	"})();";
	CefRegisterExtension("callback/test", code, new MyHandler());
}
