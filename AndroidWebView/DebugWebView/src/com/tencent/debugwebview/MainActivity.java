package com.tencent.debugwebview;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Scanner;

import org.apache.http.Header;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.util.EntityUtils;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.ActionBar;
import android.support.v7.app.ActionBarActivity;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.webkit.ConsoleMessage;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;

import com.tencent.debugwebview.tools.MLogger;
import com.tencent.debugwebview.tools.MSharedPreference;

public class MainActivity extends ActionBarActivity implements OnClickListener {
    /**
     * views for main operation
     */
    // main webView
    private WebView mWebView;
    // progress bar indicating the loading of web page in the webView
    private ProgressBar mProgressBar;
    // menu icon in action bar indicating the server condition
    private  MenuItem serverMenuItem;
    // editText to input URL in action bar
    private EditText urlEditTextInActionBar;
    // editText to input URL in dialog
    private EditText urlEditTextIndialog;
    // button to clear URL
    private ImageView clearUrlBtn;
    
    /**
     * views for the pop up window to input IP and port
     */
    View ipPortView;
    PopupWindow ipPortPopupWindow;

    /**
     * variables for main operation
     */
    private Handler mHandler;
    private static final int SHOW_MESSAGE = 1001;
    private static final int SHOW_SERVER_CONNECTED = 1002;
    private static final int SHOW_SERVER_DISCONNECTED = 1003;

    /**
     * variables for the WebView
     */
    private static final String DEFAULT_URL = "http://www.qq.com";
    private String userAgentString;
    private WebViewClient mWebViewClient;
    private WebChromeClient mWebChromeClient;

    /**
     * variables for Internet
     */
    private boolean isConnecting;
    // thread to monitor data from server
    private Runnable monitorServerRunnable;

    /**
     * variables for operation between the client and server
     */
    private String ip, port;
    String msgText;
    int fileCount;
    String debugJs = "debugBreakPoint";
    String debugFunction = "var debugBreakPoint=function(i){while(Debug.isBreakPoint(i)){try{var s=Debug.debug();eval(s)}catch(e){}}};";

    static BufferedReader mBufferedReader = null;
    static PrintWriter mPrintWriter = null;
    static PrintStream mPrintStreamWriter = null;
    private Thread mThreadClient = null;
    private Socket mSocketClient = null;
    
    /**
     * other variables
     */
    private boolean doubleBackToExitPressedOnce = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initVariables();
        initViews();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (isConnecting) {
            disconnect();
        }
        int nPid = android.os.Process.myPid();
        android.os.Process.killProcess(nPid);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        serverMenuItem = menu.findItem(R.id.action_server);
        
        /**
         * IP and port change frequently, so show the pop up window to set IP and port at the beginning,
         * use post delay or a exception will occur
         * It looks weird to add the operation here, the reason is the pup up window is just under the menu icon,
         * so, the menu icon must have initialized before the window to show
         */
        ipPortView.post(new Runnable() {
            @Override
            public void run() {
                ipPortPopupWindow.showAsDropDown(findViewById(R.id.action_server));                
            }
        });
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle presses on the action bar items
        switch (item.getItemId()) {
        case R.id.action_server:
            // show the dialog to set ip and port
            ipPortPopupWindow.showAsDropDown(findViewById(R.id.action_server));
            return true;
        case android.R.id.home:
            onBackKeyClick();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.url_input_editText_in_action_bar:
            // show the dialog to input URL
            showInputUrlDialog();
            break;

        case R.id.clear_url_imageView_in_dialog:
            // clear URL input
            urlEditTextIndialog.setText("");
        default:
            break;
        }
    }

    @Override
    public void onBackPressed() {
        onBackKeyClick();
    }

    /**
     * initialize variables used in this application
     */
    @SuppressLint("HandlerLeak")
    private void initVariables() {
        /**
         * handler
         */
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                case SHOW_MESSAGE:
                    if (msg.obj != null) {
                        Toast.makeText(MainActivity.this, msg.obj.toString(), Toast.LENGTH_LONG).show();
                    }
                    break;
                case SHOW_SERVER_CONNECTED:
                    serverMenuItem.setIcon(R.drawable.ic_action_server_connected);
                    break;
                case SHOW_SERVER_DISCONNECTED:
                    serverMenuItem.setIcon(R.drawable.ic_action_server_disconnected);
                    break;

                default:
                    break;
                }
            }
        };

        /**
         * WebViewClient
         */
        mWebViewClient = new WebViewClient() {
            @SuppressLint("NewApi")
            public WebResourceResponse shouldInterceptRequest(WebView view, String url) {
                MLogger.d(url);
                /*
                 * String s=doGet(url); s+="alert('xx')";
                 */
                /*
                 * if (fileCount == 0) {
                 * ++fileCount;
                 * String s=doGet(url);
                 * if(s==null){
                 * return null;
                 * }
                 * String html = debugHtml(s);
                 * try {
                 * return new WebResourceResponse("text/html", "utf-8",
                 * StringToInputStream(html));
                 * } catch (Exception e) {
                 * // TODO Auto-generated catch block
                 * e.printStackTrace();
                 * }
                 * } else
                 */

                if (url.indexOf(".js") != -1) {
                    String js = doGet(url);
                    if (js == null) {
                        return null;
                    }

                    int count = 0;
                    Scanner scanner = new Scanner(js);
                    StringBuilder resultJs = new StringBuilder(debugFunction);
                    while (scanner.hasNextLine()) {
                        String line = scanner.nextLine();
                        // process the line
                        ++count;
                        resultJs.append(debugJs + "(" + count + ");" + line + "\n");
                    }
                    // js=debugJs+js.replaceAll("\n", "\n"+debugJs);
                    MLogger.d(resultJs.toString());
                    try {
                        return new WebResourceResponse("text/javascript", "utf-8",
                                StringToInputStream(resultJs.toString()));
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                return null;
            }

            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                fileCount = 0;
            }
        };

        /**
         * WebChromeClient
         */
        mWebChromeClient = new WebChromeClient() {
            public void onProgressChanged(WebView view, int progress) {
                // when loading web page, show progress bar
                if (progress == 100) {
                    mProgressBar.setVisibility(View.GONE);
                    urlEditTextInActionBar.setText(mWebView.getUrl());
                } else {
                    if (mProgressBar.getVisibility() == View.GONE) {
                        mProgressBar.setVisibility(View.VISIBLE);
                    }
                    // show progress
                    mProgressBar.setProgress(progress);
                }
            }

            public void onConsoleMessage(String message, int lineNumber, String sourceID) {
                String s = message + " -- From line " + lineNumber + " of " + sourceID;
                MLogger.d(s);
                if (isConnecting) {
                    send(s);
                }
            }

            public boolean onConsoleMessage(ConsoleMessage cm) {
                String s = cm.message() + " -- From line " + cm.lineNumber() + " of " + cm.sourceId();
                MLogger.d(s);
                if (isConnecting) {
                    send(s);
                }
                return true;

            }
        };

        /**
         * Runnable
         */
        monitorServerRunnable = new Runnable() {
            public void run() {
                String sIp = ip;
                String sPort = port;

                if (TextUtils.isEmpty(sIp) || TextUtils.isEmpty(sPort)) {
                    msgText = "ip或port为空\n";
                    Message msg = Message.obtain();
                    msg.obj = msgText;
                    msg.what = SHOW_MESSAGE;
                    mHandler.sendMessage(msg);
                    return;
                }

                int port = Integer.parseInt(sPort);
                MLogger.d("ip : " + sIp + ", port : " + sPort);

                try {
                    // 连接服务器
                    mSocketClient = new Socket(sIp, port); // port number
                    // 取得输入、输出流
                    mBufferedReader = new BufferedReader(new InputStreamReader(mSocketClient.getInputStream()));

                    mPrintWriter = new PrintWriter(mSocketClient.getOutputStream(), true);
                    mPrintStreamWriter = new PrintStream(mSocketClient.getOutputStream(), true);

                    msgText = "已经连接server!\n"; // 消息换行
                    Message msg = Message.obtain();
                    msg.obj = msgText;
                    msg.what = SHOW_MESSAGE;
                    mHandler.sendMessage(msg);
                    isConnecting = true;
                    // change action bar menu icon
                    showServerState(SHOW_SERVER_CONNECTED);
                    
                    send("手机已经连接");
                } catch (Exception e) {
                    msgText = "连接IP异常:" + e.toString() + e.getMessage() + "\n";// 消息换行
                    Message msg = Message.obtain();
                    msg.what = SHOW_MESSAGE;
                    msg.obj = msgText;
                    mHandler.sendMessage(msg);
                    return;
                }

                char[] buffer = new char[4096];
                int count = 0;
                while (isConnecting) {
                    try {
                        if ((count = mBufferedReader.read(buffer)) > 0) {
                            msgText = getInfoBuff(buffer, count) + "\n";// 消息换行
                            Message msg = Message.obtain();
                            msg.what = SHOW_MESSAGE;
                            msg.obj = msgText;
                            mHandler.sendMessage(msg);
                            if (msgText.startsWith("js:")) {
                                String js = msgText.substring(3);
                                js = js.trim();
                                int index = js.lastIndexOf(";");
                                if (index == -1 || index == js.length() - 1) {
                                    if (js.indexOf("var ") == -1) {
                                        mWebView.loadUrl("javascript:var ssss=" + js + ";Debug.sendResponse(ssss);");
                                    } else {
                                        mWebView.loadUrl("javascript:" + js + ";");
                                    }
                                } else {
                                    mWebView.loadUrl("javascript:" + js.substring(0, index + 1) + "var ssss="
                                            + js.substring(index + 1) + ";Debug.sendResponse(ssss);");
                                }
                            }
                            if (msgText.startsWith("getHtml")) {
                                mWebView.loadUrl("javascript:Debug.sendResponse(document.documentElement.outerHTML)");
                            }
                        }
                    } catch (Exception e) {
                        try {
                            if (mSocketClient != null) {
                                mSocketClient.close();
                                mSocketClient = null;
                            }
                            isConnecting = false;
                            // change action bar menu icon
                            showServerState(SHOW_SERVER_DISCONNECTED);
                            
                        } catch (IOException e1) {
                            e1.printStackTrace();
                        }
                    }
                }
            }
        };
    }

    /**
     * initialize views
     */
    @SuppressLint("SetJavaScriptEnabled")
    private void initViews() {
        /**
         * set the action bar frame
         */
        ActionBar actionBar = getSupportActionBar();
        // enable the return button on the action bar
        actionBar.setDisplayHomeAsUpEnabled(true);
        // hide title on the action bar
        actionBar.setDisplayShowTitleEnabled(false);
        // add the URL input editText
        actionBar.setDisplayShowCustomEnabled(true);
        // get the layout
        LayoutInflater inflator = (LayoutInflater) this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View v = inflator.inflate(R.layout.action_bar_url_input, null);
        actionBar.setCustomView(v);

        /**
         * initialize views
         */
        mWebView = (WebView) findViewById(R.id.webview);
        mProgressBar = (ProgressBar) findViewById(R.id.progressBar);
        urlEditTextInActionBar = (EditText) findViewById(R.id.url_input_editText_in_action_bar);
        urlEditTextInActionBar.setOnClickListener(this);

        /**
         * set the attribute of the WebView
         */
        WebSettings mWebSettings = mWebView.getSettings();
        mWebSettings.setSupportZoom(false);
        mWebSettings.setJavaScriptCanOpenWindowsAutomatically(true);
        mWebSettings.setJavaScriptEnabled(true);
        mWebSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);

        mWebView.clearCache(true);
        mWebView.loadUrl(DEFAULT_URL);
        mWebView.setWebViewClient(mWebViewClient);
        mWebView.setWebChromeClient(mWebChromeClient);
        mWebView.addJavascriptInterface(new InjectObject(), "Debug");

        userAgentString = mWebSettings.getUserAgentString();
        
        // initialize the popupWindow where user can set the IP and port to connect to the server
        initIpPortPopupWindow();
    }

    /**
     * show a dialog where user can input the URL
     */
    private void showInputUrlDialog() {
        /**
         * initialize the dialog
         */
        final Dialog inputUrlDialog = new Dialog(this);
        inputUrlDialog.setCanceledOnTouchOutside(true);
        inputUrlDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        inputUrlDialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        // set the position
        WindowManager.LayoutParams para = inputUrlDialog.getWindow().getAttributes();
        para.x = 0;
        para.y = 0;
        para.width = WindowManager.LayoutParams.MATCH_PARENT;
        para.gravity = Gravity.TOP | Gravity.FILL_HORIZONTAL;
        // set layout
        inputUrlDialog.setContentView(R.layout.dialog_url_input);

        /**
         * initialize views in dialog
         */
        urlEditTextIndialog = (EditText) inputUrlDialog.findViewById(R.id.url_input_editText_in_dialog);
        urlEditTextIndialog.setText(urlEditTextInActionBar.getText().toString());
        urlEditTextIndialog.selectAll();
        urlEditTextIndialog.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                /**
                 * 暂时测到小米2的actionId为IME_ACTION_DONE，其余为IME_NULL
                 */
                if (actionId == EditorInfo.IME_NULL || actionId == EditorInfo.IME_ACTION_DONE) {
                    String url = urlEditTextIndialog.getText().toString();
                    urlEditTextInActionBar.setText(url);

                    if (!url.startsWith("http")) {
                        url = "http://" + url;
                    }
                    mWebView.loadUrl(url);
                    if (isConnecting) {
                        send(url);
                    }
                    inputUrlDialog.dismiss();
                }
                return false;
            }
        });
        clearUrlBtn = (ImageView) inputUrlDialog.findViewById(R.id.clear_url_imageView_in_dialog);
        clearUrlBtn.setOnClickListener(this);

        // show dialog
        inputUrlDialog.show();
    }

    /**
     * initialize the popupWindow where user can set the IP and port to connect to the server
     */
    private void initIpPortPopupWindow() {
        /**
         * initialize the pop up window
         */
        ipPortView = getLayoutInflater().inflate(R.layout.popup_ip_port_input, null);
        
        ipPortPopupWindow = new PopupWindow(ipPortView, LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT, true);
        ipPortPopupWindow.setTouchable(true);
        ipPortPopupWindow.setOutsideTouchable(true);
        ipPortPopupWindow.setBackgroundDrawable(new BitmapDrawable());

        /**
         * initialize views in pop up window
         */
        final EditText ipEditText = (EditText) ipPortView.findViewById(R.id.ip_editText);
        ipEditText.setText(MSharedPreference.get(this, MSharedPreference.IP, ""));
        final EditText portEditText = (EditText) ipPortView.findViewById(R.id.port_editText);
        portEditText.setText(MSharedPreference.get(this, MSharedPreference.PORT, ""));
        ImageView ipPortConnectImage = (ImageView) ipPortView.findViewById(R.id.ip_port_connect_imageView);
        final LinearLayout ipPortConnectBg = (LinearLayout) ipPortView.findViewById(R.id.ip_port_connect_linearLayout);
        ImageView ipPortDisconnectImage = (ImageView) ipPortView.findViewById(R.id.ip_port_disconnect_imageView);
        final LinearLayout ipPortDisconnectBg = (LinearLayout) ipPortView.findViewById(R.id.ip_port_disconnect_linearLayout);
        // click listener for button
        View.OnClickListener ipPortViewOnClickListener = new OnClickListener() {
            @Override
            public void onClick(View view) {
                switch (view.getId()) {
                case R.id.ip_port_connect_imageView:
                case R.id.ip_port_connect_linearLayout:
                    // set ip and port
                    ip = ipEditText.getText().toString();
                    MSharedPreference.save(MainActivity.this, MSharedPreference.IP, ip);
                    port = portEditText.getText().toString();
                    MSharedPreference.save(MainActivity.this, MSharedPreference.PORT, port);
                    // dismiss pop up window
                    ipPortPopupWindow.dismiss();

                    /**
                     * connect to server
                     */
                    mThreadClient = new Thread(monitorServerRunnable);
                    mThreadClient.start();
                    break;
                case R.id.ip_port_disconnect_imageView:
                case R.id.ip_port_disconnect_linearLayout:
                    // disconnect
                    if (isConnecting) {
                        disconnect();
                    }
                    // dismiss pop up window
                    ipPortPopupWindow.dismiss();
                    break;
                }
            }
        };
        // touch listener for button: when touch, change the color of background
        View.OnTouchListener ipPortViewOnTouchListener = new OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionevent) {
                switch (motionevent.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    switch (view.getId()) {
                    case R.id.ip_port_connect_imageView:
                    case R.id.ip_port_connect_linearLayout:
                        ipPortConnectBg.setBackgroundColor(getResources().getColor(R.color.light_blue_bg));
                        break;
                    case R.id.ip_port_disconnect_imageView:
                    case R.id.ip_port_disconnect_linearLayout:
                        ipPortDisconnectBg.setBackgroundColor(getResources().getColor(R.color.light_blue_bg));
                        break;
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    switch (view.getId()) {
                    case R.id.ip_port_connect_imageView:
                    case R.id.ip_port_connect_linearLayout:
                        ipPortConnectBg.setBackgroundColor(getResources().getColor(R.color.transparent));
                        break;
                    case R.id.ip_port_disconnect_imageView:
                    case R.id.ip_port_disconnect_linearLayout:
                        ipPortDisconnectBg.setBackgroundColor(getResources().getColor(R.color.transparent));
                        break;
                    }
                    break;
                }
                return false;
            }
        };
        /**
         * set on click listener
         */
        ipPortConnectImage.setOnClickListener(ipPortViewOnClickListener);
        ipPortConnectBg.setOnClickListener(ipPortViewOnClickListener);
        ipPortDisconnectImage.setOnClickListener(ipPortViewOnClickListener);
        ipPortDisconnectBg.setOnClickListener(ipPortViewOnClickListener);
        /**
         * set on touch listener
         */
        ipPortConnectImage.setOnTouchListener(ipPortViewOnTouchListener);
        ipPortConnectBg.setOnTouchListener(ipPortViewOnTouchListener);
        ipPortDisconnectImage.setOnTouchListener(ipPortViewOnTouchListener);
        ipPortDisconnectBg.setOnTouchListener(ipPortViewOnTouchListener);
    }

    private void disconnect() {
        isConnecting = false;
        // change action bar menu icon
        showServerState(SHOW_SERVER_CONNECTED);
        serverMenuItem.setIcon(R.drawable.ic_action_server_disconnected);
        try {
            if (mSocketClient != null) {
                if (mSocketClient.isConnected()) {
                    send("exit");
                }
                mSocketClient.close();
                mSocketClient = null;

                mPrintWriter.close();
                mPrintStreamWriter.close();
                mPrintStreamWriter = null;
                mPrintWriter = null;
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        mThreadClient.interrupt();
    }

    private String getInfoBuff(char[] buff, int count) {
        char[] temp = new char[count];
        for (int i = 0; i < count; i++) {
            temp[i] = buff[i];
        }
        return new String(temp);
    }

    /**
     * 将String转换成InputStream
     */
    public static InputStream StringToInputStream(String in) throws Exception {
        ByteArrayInputStream is = new ByteArrayInputStream(in.getBytes("utf-8"));
        return is;
    }

    public byte[] intToByte(int i) {
        byte[] abyte0 = new byte[4];
        abyte0[0] = (byte) (0xff & i);
        abyte0[1] = (byte) ((0xff00 & i) >> 8);
        abyte0[2] = (byte) ((0xff0000 & i) >> 16);
        abyte0[3] = (byte) ((0xff000000 & i) >> 24);
        return abyte0;
    }

    private void send(String s) {
        if (isConnecting) {
            // int l = s.getBytes().length;
            // mPrintStreamWriter.write(intToByte(l),0,4);
            mPrintWriter.print(s);// 发送给服务器
            mPrintWriter.flush();
        }
    }

    /**
     * Get请求
     */
    public String doGet(String url) {
        HttpParams httpParams = new BasicHttpParams();
        HttpConnectionParams.setConnectionTimeout(httpParams, 30000);
        HttpConnectionParams.setSoTimeout(httpParams, 30000);

        HttpClient httpClient = new DefaultHttpClient(httpParams);
        HttpGet httpGet = new HttpGet(url);
        httpGet.setHeader("User-Agent", userAgentString);
        try {
            HttpResponse response = httpClient.execute(httpGet);
            if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                MLogger.i("GET : Bad Request!");
            }
            Header[] headers = response.getHeaders("Content-Type");
            if (headers.length > 0 && headers[0].getValue().startsWith("image")) {
                return null;
            }
            String result = EntityUtils.toString(response.getEntity(), "UTF-8");
            return result;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }

    public String debugHtml(String html) {
        Scanner scanner = new Scanner(html);
        // js=debugFunction+js;
        boolean isScript = false;
        StringBuilder resultJs = new StringBuilder();
        boolean isFirst = true;
        int count = 1;
        while (scanner.hasNextLine()) {
            String line = scanner.nextLine();
            int index = line.indexOf("<script");
            String line1 = "";
            if (index != -1) {
                int index2 = line.indexOf(">", index);
                if (line.indexOf("</script>", index) == index2 + 1) {
                    MLogger.d("source");
                } else {
                    if (isFirst) {
                        line1 = line.substring(0, index2 + 1) + debugFunction + debugJs + "(" + count + ");"
                                + line.substring(index2 + 1);
                        isFirst = false;
                    } else {
                        line1 = line.substring(0, index2 + 1) + debugJs + "(" + count + ");"
                                + line.substring(index2 + 1);
                    }
                    isScript = true;
                    if (line.indexOf("<", index2 + 1) != -1) {
                        isScript = false;
                    }
                }
            } else if (isScript) {
                if (line.indexOf("<") != -1) {
                    isScript = false;
                }
                line1 = debugJs + "(" + count + ");" + line;
            } else {
                line1 = line;
            }
            // process the line
            ++count;
            resultJs.append(line1);
        }
        send(resultJs.toString());
        return resultJs.toString();
    }
    
    private void onBackKeyClick() {
        if (mWebView.canGoBack()) {
            mWebView.stopLoading();
            mWebView.goBack();
        } else {
            // double click the back key and exit
            if (doubleBackToExitPressedOnce) {
                super.onBackPressed();
                return;
            }
            doubleBackToExitPressedOnce = true;
            Toast.makeText(this, getString(R.string.double_click_to_exit), Toast.LENGTH_SHORT).show();
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    doubleBackToExitPressedOnce = false;
                }
            }, 2000);
        }
    }
    
    /**
     * change the color of the action bar menu icon indicating different state of server
     * @param state <i>SHOW_SERVER_CONNECTED</i>or<i>SHOW_SERVER_DISCONNECTED</i>
     */
    private void showServerState(int state) {
        switch (state) {
        case SHOW_SERVER_CONNECTED:
        case SHOW_SERVER_DISCONNECTED:
            mHandler.sendEmptyMessage(state);
            break;

        default:
            break;
        }
    }

    public class InjectObject {
        /**
         * what the hell is this ??? call melody
         * 
         * @return
         */
        public String debug() {
            if (msgText.equals("")) {
                return msgText;
            } else {
                String s = msgText;
                msgText = "";
                return s;
            }
        }

        public boolean isBreakPoint(int i) {
            return false;
        }

        public void sendResponse(String s) {
            send(s);
        }
    }
}
