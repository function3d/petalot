package com.function3d.petalot

import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Handler
import android.os.Looper
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors

/**
 * Update check against the manifest in the repository (App/dist/app-latest.json),
 * the same idea as the firmware's latest.json. The app never installs anything
 * itself: it offers the download and lets Android's installer do the rest, so
 * the APK has to be signed with the same key as the installed one.
 */
object Updates {

    private const val MANIFEST =
        "https://raw.githubusercontent.com/function3d/petalot/master/App/dist/app-latest.json"

    private val pool = Executors.newSingleThreadExecutor()
    private val main = Handler(Looper.getMainLooper())

    data class Latest(val versionName: String, val versionCode: Int, val url: String)

    /** Silent check: only shows a dialog when there is something newer. */
    fun checkQuietly(ctx: Context) {
        pool.execute {
            val latest = fetch() ?: return@execute
            if (latest.versionCode <= currentVersionCode(ctx)) return@execute
            main.post { offer(ctx, latest) }
        }
    }

    private fun currentVersionCode(ctx: Context): Int = try {
        ctx.packageManager.getPackageInfo(ctx.packageName, 0).longVersionCode.toInt()
    } catch (_: Exception) {
        0
    }

    private fun fetch(): Latest? = try {
        val conn = URL(MANIFEST).openConnection() as HttpURLConnection
        conn.connectTimeout = 5000
        conn.readTimeout = 5000
        val body = conn.inputStream.bufferedReader().use { it.readText() }
        conn.disconnect()
        val json = JSONObject(body)
        Latest(
            json.getString("versionName"),
            json.getInt("versionCode"),
            json.getString("url"),
        )
    } catch (_: Exception) {
        null
    }

    private fun offer(ctx: Context, latest: Latest) {
        AlertDialog.Builder(ctx)
            .setTitle(R.string.update_title)
            .setMessage(ctx.getString(R.string.update_message, latest.versionName))
            .setPositiveButton(R.string.update_install) { _, _ ->
                // The browser downloads it; Android then asks to install.
                runCatching {
                    ctx.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(latest.url)))
                }
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }
}
