package com.function3d.petalot

import android.content.Context

/**
 * Remembers the machines the user has connected to. Most people own one
 * machine, but the list keeps several (a LAN address and the AP address, for
 * instance) and reconnects to the most recent one.
 */
object DeviceStore {

    private const val PREFS = "petalot"
    private const val KEY_LIST = "devices"
    private const val KEY_LAST = "last"
    private const val MAX = 20

    /** Adds the scheme if missing and drops the trailing slash. */
    fun normalize(raw: String): String {
        var s = raw.trim()
        if (!s.startsWith("http://") && !s.startsWith("https://")) s = "http://$s"
        return s.trimEnd('/')
    }

    private fun prefs(ctx: Context) = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    fun list(ctx: Context): MutableList<String> =
        (prefs(ctx).getString(KEY_LIST, "") ?: "")
            .split('\n')
            .filter { it.isNotBlank() }
            // Older builds stored "url<TAB>wifi"; keep the url only.
            .map { it.substringBefore('\t') }
            .toMutableList()

    fun remember(ctx: Context, url: String) {
        val target = normalize(url)
        val l = list(ctx)
        l.remove(target)
        l.add(0, target)
        while (l.size > MAX) l.removeAt(l.size - 1)
        prefs(ctx).edit()
            .putString(KEY_LIST, l.joinToString("\n"))
            .putString(KEY_LAST, target)
            .apply()
    }

    fun remove(ctx: Context, url: String) {
        val l = list(ctx)
        l.remove(url)
        val e = prefs(ctx).edit().putString(KEY_LIST, l.joinToString("\n"))
        if (prefs(ctx).getString(KEY_LAST, null) == url) e.putString(KEY_LAST, l.firstOrNull() ?: "")
        e.apply()
    }

    fun last(ctx: Context): String? =
        prefs(ctx).getString(KEY_LAST, null)?.takeIf { it.isNotBlank() }
}
