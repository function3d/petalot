package com.function3d.petalot

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Build

/**
 * Detects the PETALOT access point without asking for any permission: in AP
 * mode the firmware's DHCP hands out 192.168.4.1 as the gateway/DNS/DHCP
 * server, which is readable from the current network's LinkProperties.
 */
object Wifi {

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
