package com.function3d.petalot

import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.text.InputType
import android.view.ViewGroup
import android.webkit.HttpAuthHandler
import android.webkit.ValueCallback
import android.webkit.WebChromeClient
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.Toast

/**
 * Thin shell around the device's own web UI: it loads http://<device>/ in a
 * WebView, so every feature and language comes from the firmware itself.
 */
class MainActivity : Activity() {

    private lateinit var web: WebView
    private lateinit var progress: ProgressBar
    private var deviceHost: String = ""
    private var deviceUrl: String = ""
    private var loadFailed = false
    private var languageReloaded = false
    private var fileChooser: ValueCallback<Array<Uri>>? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val url = intent.getStringExtra(EXTRA_URL)
            ?: DeviceStore.last(this)
            ?: run {
                startActivity(Intent(this, ConnectActivity::class.java))
                finish()
                return
            }

        buildUi()
        load(url)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        intent.getStringExtra(EXTRA_URL)?.let { load(it) }
    }

    private fun load(url: String) {
        deviceUrl = url
        deviceHost = Uri.parse(url).host ?: ""
        loadFailed = false
        languageReloaded = false
        web.loadUrl(url)
    }

    /**
     * Makes the web UI open in the app's language. The firmware picks
     * navigator.language (system locale) and keeps its own choice in
     * localStorage, so the app language is pushed once and then left alone:
     * a language picked inside the web UI survives until the app language
     * changes again.
     */
    private fun syncWebLanguage() {
        val want = appLanguage()
        web.evaluateJavascript(
            """(function(){try{
                 var want='$want';
                 var applied=localStorage.getItem('petalot.applang');
                 if(applied!==want){
                   localStorage.setItem('petalot.lang',want);
                   localStorage.setItem('petalot.applang',want);
                   return document.documentElement.lang===want?'ok':'reload';
                 }
                 return 'ok';
               }catch(e){return 'ok';}})()""",
            ValueCallback { result ->
                if (result.contains("reload") && !languageReloaded) {
                    languageReloaded = true
                    web.reload()
                }
            }
        )
    }

    /** Same 12 languages as the firmware web UI. */
    private fun appLanguage(): String {
        val tag = resources.configuration.locales[0].language
        return if (tag in LANGUAGES) tag else "en"
    }

    @SuppressLint("SetJavaScriptEnabled")
    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(getColor(R.color.bg))
        }

        progress = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 100
            visibility = ProgressBar.GONE
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(3))
        }

        web = WebView(this).apply {
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f)
            setBackgroundColor(getColor(R.color.bg))
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.mediaPlaybackRequiresUserGesture = false
            webViewClient = client()
            webChromeClient = chrome()
        }

        root.addView(progress)
        root.addView(web)
        setContentView(root)
    }

    private fun client() = object : WebViewClient() {
        override fun shouldOverrideUrlLoading(
            view: WebView,
            request: WebResourceRequest,
        ): Boolean = handleUrl(request.url)

        override fun onReceivedError(
            view: WebView,
            request: WebResourceRequest,
            error: WebResourceError,
        ) {
            if (request.isForMainFrame) {
                loadFailed = true
                // Reachability is the usual cause; back goes to the device list.
                Toast.makeText(this@MainActivity, R.string.load_error, Toast.LENGTH_LONG).show()
            }
        }

        override fun onPageFinished(view: WebView, url: String) {
            // Remember which Wi-Fi actually reached the device, not the one that
            // was current when the user tapped it.
            if (!loadFailed && url.startsWith(deviceUrl)) {
                DeviceStore.remember(this@MainActivity, deviceUrl, Wifi.ssid(this@MainActivity))
                syncWebLanguage()
            }
        }

        override fun onReceivedHttpAuthRequest(
            view: WebView,
            handler: HttpAuthHandler,
            host: String,
            realm: String,
        ) {
            // Only used when the firmware is built with WEB_PASSWORD.
            authDialog(handler, host)
        }
    }

    private fun chrome() = object : WebChromeClient() {
        override fun onProgressChanged(view: WebView, newProgress: Int) {
            progress.progress = newProgress
            progress.visibility = if (newProgress >= 100) ProgressBar.GONE else ProgressBar.VISIBLE
        }

        override fun onShowFileChooser(
            view: WebView,
            callback: ValueCallback<Array<Uri>>,
            params: FileChooserParams,
        ): Boolean {
            fileChooser?.onReceiveValue(null)
            fileChooser = callback
            // The UI accepts .bin/.bin.gz, which providers rarely map to a MIME
            // type, so offer every file instead of filtering invisibly.
            val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "*/*"
            }
            return try {
                startActivityForResult(intent, REQ_FILE)
                true
            } catch (_: Exception) {
                fileChooser = null
                false
            }
        }
    }

    /** Keeps device pages in-app; everything else leaves to the browser. */
    private fun handleUrl(u: Uri): Boolean {
        val scheme = u.scheme ?: return false
        if (scheme != "http" && scheme != "https") {
            return try {
                startActivity(Intent(Intent.ACTION_VIEW, u))
                true
            } catch (_: Exception) {
                true
            }
        }
        if (u.host != deviceHost) {
            try {
                startActivity(Intent(Intent.ACTION_VIEW, u))
            } catch (_: Exception) {
                Toast.makeText(this, u.toString(), Toast.LENGTH_SHORT).show()
            }
            return true
        }
        return false
    }

    private fun authDialog(handler: HttpAuthHandler, host: String) {
        val box = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(24), dp(8), dp(24), 0)
        }
        val user = EditText(this).apply { hint = getString(R.string.auth_user) }
        val pass = EditText(this).apply {
            hint = getString(R.string.auth_password)
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_PASSWORD
        }
        box.addView(user)
        box.addView(pass)
        AlertDialog.Builder(this)
            .setTitle(getString(R.string.auth_title, host))
            .setView(box)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                handler.proceed(user.text.toString(), pass.text.toString())
            }
            .setNegativeButton(android.R.string.cancel) { _, _ -> handler.cancel() }
            .setOnCancelListener { handler.cancel() }
            .show()
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == REQ_FILE) {
            fileChooser?.onReceiveValue(
                WebChromeClient.FileChooserParams.parseResult(resultCode, data)
            )
            fileChooser = null
            return
        }
        @Suppress("DEPRECATION")
        super.onActivityResult(requestCode, resultCode, data)
    }

    @Suppress("OVERRIDE_DEPRECATION")
    override fun onBackPressed() {
        // The web UI is a single page; letting the WebView go back just reloads
        // the same (or the previous device's) page, so back always returns to
        // the device list. Leaving the app is done from there.
        startActivity(Intent(this, ConnectActivity::class.java))
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        const val EXTRA_URL = "url"
        private const val REQ_FILE = 1001
        private val LANGUAGES =
            setOf("en", "es", "pt", "fr", "de", "it", "zh", "cs", "ru", "tr", "ja", "ko")
    }
}
