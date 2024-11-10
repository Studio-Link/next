package link.studio.app

import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.activity.ComponentActivity
import androidx.activity.enableEdgeToEdge
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

private var refused_fail = 0

private class MyWebViewClient : WebViewClient() {
    override fun onReceivedError(
        view: WebView?,
        request: WebResourceRequest?,
        error: WebResourceError?
    ) {
        if (error?.errorCode == WebViewClient.ERROR_CONNECT && refused_fail++ < 10) {
            Thread.sleep(100)
            view?.reload()
        }
        println("WebView:onReceivedError: " + error?.description)
        super.onReceivedError(view, request, error)
    }

    override fun shouldOverrideUrlLoading(view: WebView?, request: WebResourceRequest?): Boolean {
        val url = request?.url.toString()
        if (Uri.parse(url).host == "localhost") {
            // This is your website, so don't override. Let your WebView load
            // the page.
            return false
        }

        Intent(Intent.ACTION_VIEW, Uri.parse(url)).apply {
            view?.context?.startActivity(this)
        }
        return true
    }
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        if (ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED && ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            val permissions = arrayOf(android.Manifest.permission.RECORD_AUDIO, android.Manifest.permission.WRITE_EXTERNAL_STORAGE, android.Manifest.permission.READ_EXTERNAL_STORAGE)
            ActivityCompat.requestPermissions(this, permissions,0)
        }

        Api.slStart(filesDir.absolutePath)
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val myWebView = WebView(this)
        setContentView(myWebView)
        myWebView.settings.javaScriptEnabled = true
        myWebView.loadUrl("http://localhost:9999/")
        myWebView.webViewClient = MyWebViewClient()
    }

    init {
        System.loadLibrary("api")
    }
}