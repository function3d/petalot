package com.function3d.petalot

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.net.wifi.WifiManager
import android.os.Build

/**
 * Network helpers. The SSID needs the location permission; the default gateway
 * does not, and it is what tells us the PETALOT AP is the network we are on.
 */
object Wifi {

    private const val UNKNOWN = "<unknown ssid>"

    fun ssid(ctx: Context): String {
        return try {
            val wm = ctx.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
            @Suppress("DEPRECATION")
            val raw = wm.connectionInfo?.ssid ?: ""
            val s = raw.trim('"')
            if (s.isEmpty() || s == UNKNOWN || s.startsWith("<")) "" else s
        } catch (_: Exception) {
            ""
        }
    }

    /**
     * True when the current Wi-Fi routes through [apAddress]. In AP mode the
     * firmware's DHCP hands out that gateway, so no permission or HTTP request
     * is needed to know the machine is right there.
     */
    fun onPetalotAp(ctx: Context, apAddress: String): Boolean {
        return try {
            val cm = ctx.applicationContext
                .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
            val net = cm.activeNetwork ?: return false
            val caps = cm.getNetworkCapabilities(net) ?: return false
            if (!caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) return false
            val link = cm.getLinkProperties(net) ?: return false

            if (link.routes.any { it.gateway?.hostAddress == apAddress }) return true
            if (link.dnsServers.any { it.hostAddress == apAddress }) return true
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
                link.dhcpServerAddress?.hostAddress == apAddress
            ) {
                return true
            }
            false
        } catch (_: Exception) {
            false
        }
    }
}
