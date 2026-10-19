package com.function3d.petalot

import android.content.Context

/**
 * Remembers the machines the user has connected to, together with the Wi-Fi
 * they were reached from. That matters for the AP address: 192.168.4.1 is the
 * same for every PETALOT, so the SSID tells them apart.
 */
object DeviceStore {

    private const val PREFS = "petalot"
    private const val KEY_LIST = "devices"
    private const val KEY_LAST = "last"
    private const val MAX = 20

    data class Device(val url: String, val ssid: String)

    /** Adds the scheme if missing and drops the trailing slash. */
    fun normalize(raw: String): String {
        var s = raw.trim()
        if (!s.startsWith("http://") && !s.startsWith("https://")) s = "http://$s"
        return s.trimEnd('/')
    }

    private fun prefs(ctx: Context) = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    fun list(ctx: Context): MutableList<Device> =
        (prefs(ctx).getString(KEY_LIST, "") ?: "")
            .split('\n')
            .filter { it.isNotBlank() }
            .map { line ->
                // Lines saved before the SSID existed have no tab.
                val parts = line.split('\t')
                Device(parts[0], if (parts.size > 1) parts[1] else "")
            }
            .toMutableList()

    fun remember(ctx: Context, url: String, ssid: String) {
        val target = normalize(url)
        val l = list(ctx)
        // Never drop a known SSID just because it could not be read this time.
        val previous = l.firstOrNull { it.url == target }?.ssid ?: ""
        val keep = if (ssid.isBlank()) previous else ssid
        l.removeAll { it.url == target }
        l.add(0, Device(target, keep))
        while (l.size > MAX) l.removeAt(l.size - 1)
        prefs(ctx).edit()
            .putString(KEY_LIST, l.joinToString("\n") { it.url + "\t" + it.ssid })
            .putString(KEY_LAST, target)
            .apply()
    }

    fun remove(ctx: Context, url: String) {
        val l = list(ctx)
        l.removeAll { it.url == url }
        val e = prefs(ctx).edit()
            .putString(KEY_LIST, l.joinToString("\n") { it.url + "\t" + it.ssid })
        if (prefs(ctx).getString(KEY_LAST, null) == url) e.putString(KEY_LAST, l.firstOrNull()?.url ?: "")
        e.apply()
    }

    fun last(ctx: Context): String? =
        prefs(ctx).getString(KEY_LAST, null)?.takeIf { it.isNotBlank() }
}
