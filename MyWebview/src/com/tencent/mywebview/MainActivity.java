package com.tencent.mywebview;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.List;
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

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.webkit.ConsoleMessage;
import android.webkit.JsResult;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class MainActivity extends Activity implements OnClickListener {
    WebView webView;
    Button goBtn;
    EditText urlText;
    EditText ipText;
    String msgText;
    Button connectBtn;
    int fileCount = 0;
    static BufferedReader mBufferedReader = null;
    static PrintWriter mPrintWriter = null;
    static PrintStream mPrintStreamWriter = null;
    private Thread mThreadClient = null;
    private Socket mSocketClient = null;
    private boolean isConnecting = false;
    private JsResult jsResult;
    String userAgent;
    String debugJs = "debugBreakPoint";
    String debugFunction = "var debugBreakPoint=function(i){while(Debug.isBreakPoint(i)){try{var s=Debug.debug();eval(s)}catch(e){}}};";

    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        webView = (WebView) findViewById(R.id.webview);
        goBtn = (Button) findViewById(R.id.goBtn);
        connectBtn = (Button) findViewById(R.id.connectBtn);
        urlText = (EditText) findViewById(R.id.urlText);
        ipText = (EditText) findViewById(R.id.ipText);
        WebSettings webSettings = webView.getSettings();
        webSettings.setSupportZoom(false);
        webSettings.setJavaScriptCanOpenWindowsAutomatically(true);
        webSettings.setJavaScriptEnabled(true);
        webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
        userAgent = webSettings.getUserAgentString();
        webView.clearCache(true);
        webView.loadUrl("http://www.baidu.com");
        webView.setWebViewClient(client);
        webView.setWebChromeClient(chromeClient);
        webView.addJavascriptInterface(new InjectObject(), "Debug");
        goBtn.setOnClickListener(this);
        connectBtn.setOnClickListener(this);
        SharedPreferences userInfo = getSharedPreferences("userInfo", 0);
        String ip = userInfo.getString("serverIP", "");
        if (!TextUtils.isEmpty(ip)) {
            ipText.setText(ip);
        }
    }

    Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 0) {
            } else if (msg.what == 1) {
                if (msg.obj != null) {
                    Toast.makeText(MainActivity.this, msg.obj.toString(),
                            Toast.LENGTH_LONG).show();
                }

                // msgText="";
            }
        }
    };

    private String getInfoBuff(char[] buff, int count) {
        char[] temp = new char[count];
        for (int i = 0; i < count; i++) {
            temp[i] = buff[i];
        }
        return new String(temp);
    }

    // 线程:监听服务器发来的消息
    private Runnable mRunnable = new Runnable() {
        public void run() {
            String ip = ipText.getText().toString();
            if (ip.length() <= 0) {
                // Toast.makeText(mContext, "IP不能为空！",
                // Toast.LENGTH_SHORT).show();
                Message msg = new Message();
                msg.what = 1;
                mHandler.sendMessage(msg);
                return;
            }
            int start = ip.indexOf(":");
            if ((start == -1) || (start + 1 >= ip.length())) {
                Message msg = new Message();
                msg.what = 1;
                msg.obj = "";
                mHandler.sendMessage(msg);
                return;
            }
            String sIP = ip.substring(0, start);
            String sPort = ip.substring(start + 1);
            int port = Integer.parseInt(sPort);

            Log.d("gjz", "IP:" + sIP + ":" + port);

            try {
                // 连接服务器
                mSocketClient = new Socket(sIP, port); // portnum
                // 取得输入、输出流
                mBufferedReader = new BufferedReader(new InputStreamReader(
                        mSocketClient.getInputStream()));

                mPrintWriter = new PrintWriter(mSocketClient.getOutputStream(),
                        true);

                mPrintStreamWriter = new PrintStream(
                        mSocketClient.getOutputStream(), true);

                msgText = "已经连接server!\n";// 消息换行
                Message msg = new Message();
                msg.obj = msgText;
                msg.what = 1;
                mHandler.sendMessage(msg);
                String s = "hellohel";
                /*
                 * String s1=""; for(int i=0;i<513;++i){ s1+=s; }
                 */
                send(s);
            } catch (Exception e) {
                msgText = "连接IP异常:" + e.toString() + e.getMessage() + "\n";// 消息换行
                Message msg = new Message();
                msg.what = 1;
                msg.obj = msgText;
                mHandler.sendMessage(msg);
                return;
            }
            SharedPreferences userInfo = getSharedPreferences("userInfo", 0);
            userInfo.edit().putString("serverIP", ip).commit();

            char[] buffer = new char[4096];
            int count = 0;
            int error = 0;
            while (isConnecting) {
                try {
                    // if ( (recvMessageClient =
                    // mBufferedReaderClient.readLine()) != null )
                    if ((count = mBufferedReader.read(buffer)) > 0) {
                        msgText = getInfoBuff(buffer, count) + "\n";// 消息换行
                        Message msg = new Message();
                        msg.what = 1;
                        msg.obj = msgText;
                        mHandler.sendMessage(msg);
                        if (msgText.startsWith("js:")) {
                            String js=msgText.substring(3);
                            js=js.trim();
                            int index=js.lastIndexOf(";");
                            if(index==-1||index==js.length()-1){
                                if(js.indexOf("var ")==-1){
                                    webView.loadUrl("javascript:var ssss="+js+";Debug.sendResponse(ssss);");
                                }
                                else{
                                    webView.loadUrl("javascript:"+js+";");
                                }
                            }
                            else{
                                webView.loadUrl("javascript:"+js.substring(0,index+1)+"var ssss="+js.substring(index+1)+";Debug.sendResponse(ssss);");
                            }
                        }
                        if (msgText.startsWith("getHtml")) {
                            webView.loadUrl("javascript:Debug.sendResponse(document.documentElement.outerHTML)");
                        }
                    }
                } catch (Exception e) {
                    try {
                        if (mSocketClient != null) {
                            mSocketClient.close();
                            mSocketClient = null;
                        }
                        isConnecting = false;
                        connectBtn.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                connectBtn.setText("connect");
                                ipText.setEnabled(true);
                            }

                        });

                    } catch (IOException e1) {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }
                }
            }
        }
    };

    public byte[] intToByte(int i) {
        byte[] abyte0 = new byte[4];
        abyte0[0] = (byte) (0xff & i);
        abyte0[1] = (byte) ((0xff00 & i) >> 8);
        abyte0[2] = (byte) ((0xff0000 & i) >> 16);
        abyte0[3] = (byte) ((0xff000000 & i) >> 24);
        return abyte0;
    }

    private void send(String s) {
        if(isConnecting){
            int l = s.getBytes().length;
            // mPrintStreamWriter.write(intToByte(l),0,4);
            mPrintWriter.print(s);// 发送给服务器
            mPrintWriter.flush();
        }
    }

    WebChromeClient chromeClient = new WebChromeClient() {
        public void onProgressChanged(WebView view, int progress) {
            // Make the bar disappear after URL is loaded, and changes string to
            // Loading...
            urlText.setText("Loading..." + progress + "%");
            // Return the app name after finish loading
            if (progress == 100) {
                urlText.setText(webView.getUrl());
            }
        }

        public void onConsoleMessage(String message, int lineNumber,
                String sourceID) {
            String s = message + " -- From line " + lineNumber + " of "
                    + sourceID;
            Log.d("MyWebview", s);
            if (isConnecting) {
                send(s);
            }
        }

        /*
         * @Override public boolean onJsAlert(WebView view, String url, String
         * message, final JsResult result) { send("pause"); if(jsResult!=null){
         * return false; } jsResult=result; return true; return false; }
         */

        public boolean onConsoleMessage(ConsoleMessage cm) {
            String s = cm.message() + " -- From line " + cm.lineNumber()
                    + " of " + cm.sourceId();
            Log.d("MyWebview", s);
            if (isConnecting) {
                send(s);
            }
            return true;

        }
    };

    /**
     * 将String转换成InputStream
     * 
     * @param in
     * @return
     * @throws Exception
     */
    public static InputStream StringToInputStream(String in) throws Exception {

        ByteArrayInputStream is = new ByteArrayInputStream(in.getBytes("utf-8"));
        return is;
    }

    WebViewClient client = new WebViewClient() {
        public WebResourceResponse shouldInterceptRequest(WebView view,
                String url) {
            Log.d("MyWebview", url);
            /*
             * String s=doGet(url); s+="alert('xx')";
             */
            /*if (fileCount == 0) {
                ++fileCount;
                String s=doGet(url);
                if(s==null){
                    return null;
                }
                String html = debugHtml(s);
                try {
                    return new WebResourceResponse("text/html", "utf-8",
                            StringToInputStream(html));
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            } else */if (url.indexOf(".js") != -1) {
                String js = doGet(url);
                if(js==null){
                    return null;
                }
                int count = 0;
                StringBuilder resultJs = new StringBuilder(debugFunction);
                Scanner scanner = new Scanner(js);
                // js=debugFunction+js;
                while (scanner.hasNextLine()) {
                    String line = scanner.nextLine();
                    // process the line
                    ++count;
                    resultJs.append(debugJs + "(" + count + ");" + line + "\n");
                }
                // js=debugJs+js.replaceAll("\n", "\n"+debugJs);
                Log.d("MyWebview", resultJs.toString());
                try {
                    return new WebResourceResponse("text/javascript", "utf-8",
                            StringToInputStream(resultJs.toString()));
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            return null;
            // WebResourceResponse response=
        }
        public void onPageStarted(WebView view, String url, Bitmap favicon){
            fileCount=0;
        }
    };

    public class InjectObject {
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

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (isConnecting) {
            disConnect();
        }
        int nPid = android.os.Process.myPid();
        android.os.Process.killProcess(nPid);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    private void disConnect() {
        isConnecting = false;
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

        connectBtn.post(new Runnable() {

            @Override
            public void run() {
                // TODO Auto-generated method stub
                connectBtn.setText("connect");
                ipText.setEnabled(true);
            }

        });
    }

    @Override
    public void onClick(View view) {
        // TODO Auto-generated method stub
        if (view == goBtn) {
            String url = urlText.getText().toString();
            if (!url.startsWith("http")) {
                url = "http://" + url;
            }
            webView.loadUrl(url);
            if (isConnecting) {
                send(url);
            }
        } else if (view == connectBtn) {
            if (isConnecting) {
                disConnect();
            } else {
                isConnecting = true;
                connectBtn.setText("disconnect");
                ipText.setEnabled(false);

                mThreadClient = new Thread(mRunnable);
                mThreadClient.start();
            }
        }
    }

    /*
     * 
     */
    public String debugHtml(String html) {
        Scanner scanner = new Scanner(html);
        // js=debugFunction+js;
        boolean isScript = false;
        StringBuilder resultJs = new StringBuilder();
        boolean isFirst = true;
        int count=1;
        while (scanner.hasNextLine()) {
            String line = scanner.nextLine();
            int index = line.indexOf("<script");
            String line1 = "";
            if (index != -1) {
                int index2 = line.indexOf(">", index);
                if(line.indexOf("</script>", index)==index2+1){
                    Log.d("mywebview", "source");
                }
                else{
                    if(isFirst){
                        line1 = line.substring(0, index2 + 1) + debugFunction+ debugJs + "(" + count
                            + ");" + line.substring(index2 + 1);
                        isFirst = false;
                    }
                    else{
                        line1 = line.substring(0, index2 + 1) + debugJs + "(" + count
                                + ");" + line.substring(index2 + 1);
                    }
                    isScript = true;
                    if(line.indexOf("<", index2+1)!=-1){
                        isScript = false;
                    }
                }
            } else if (isScript) {
                if (line.indexOf("<") != -1) {
                    isScript = false;
                }
                line1 = debugJs + "(" + count + ");" + line;
            }
            else{
                line1=line;
            }
            // process the line
            ++count;
            resultJs.append(line1);
        }
        send(resultJs.toString());
        return resultJs.toString();
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
        httpGet.setHeader("User-Agent", userAgent);
        // String.format("%s/%s (Linux; Android %s; %s Build/%s)", MY_APP_NAME,
        // MY_APP_VERSION_NAME, Build.VERSION.RELEASE, // GET
        try {
            HttpResponse response = httpClient.execute(httpGet);
            if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                Log.i("GET", "Bad Request!");
            }
            Header[] headers=response.getHeaders("Content-Type");
            if(headers.length>0&&headers[0].getValue().startsWith("image")){
                return null;
            }
            String result = EntityUtils.toString(response.getEntity(), "UTF-8");
            return result;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }
}
