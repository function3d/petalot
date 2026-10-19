package com.function3d.petalot

import android.content.Context
import android.net.Network
import android.os.Handler
import android.os.Looper
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors

/**
 * Reachability check for the device list.
 *
 * Requests are forced over the Wi-Fi interface: the PETALOT AP has no Internet,
 * and with mobile data on, Android would otherwise send the socket over the
 * default (mobile) network, where 192.168.4.1 is unreachable. Falls back to the
 * default network if the Wi-Fi-bound attempt fails.
 */
object Probe {

    private const val TIMEOUT = 4000
    private val pool = Executors.newFixedThreadPool(4)
    private val main = Handler(Looper.getMainLooper())

    /** True when the device answers /get at all (an auth error still counts). */
    fun check(ctx: Context, url: String, onResult: (Boolean) -> Unit) {
        pool.execute {
            val online = tryRequest(ctx, url)
            main.post { onResult(online) }
        }
    }

    private fun tryRequest(ctx: Context, base: String): Boolean {
        val candidates = ArrayList<Network?>(2)
        Net.wifiNetwork(ctx)?.let { candidates.add(it) }
        candidates.add(null) // default network as a fallback

        for (net in candidates) {
            try {
                val target = URL("$base/get")
                val conn = (net?.openConnection(target) ?: target.openConnection())
                    as HttpURLConnection
                conn.connectTimeout = TIMEOUT
                conn.readTimeout = TIMEOUT
                conn.requestMethod = "GET"
                val code = conn.responseCode
                conn.disconnect()
                // 401 means the firmware was built with a password.
                return code in 200..499
            } catch (_: Exception) {
                // Try the next candidate network.
            }
        }
        return false
    }
}
