package com.function3d.petalot

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.net.wifi.WifiManager
import android.os.Handler
import android.os.Looper

/**
 * Finds PETALOT machines on the local network over mDNS.
 *
 * The firmware calls MDNS.addService("http", "tcp", 80) with hostname
 * "petalot", so every machine shows up as _http._tcp with "petalot" in its
 * name. Two machines therefore announce the SAME instance name, so results are
 * collected by address: keying by name would make the second one overwrite the
 * first. Discovery only works in station mode; in AP mode the device answers at
 * 192.168.4.1 and ConnectActivity detects it from the network's gateway.
 */
class Discovery(context: Context) {

    private val nsd = context.applicationContext
        .getSystemService(Context.NSD_SERVICE) as NsdManager
    private val multicast = (context.applicationContext
        .getSystemService(Context.WIFI_SERVICE) as WifiManager)
        .createMulticastLock("petalot-mdns")
        .apply { setReferenceCounted(false) }

    private val main = Handler(Looper.getMainLooper())
    private val urls = LinkedHashSet<String>()            // one per machine
    private val urlByName = HashMap<String, String>()     // to handle service lost
    private val attempts = HashMap<String, Int>()
    private val queue = ArrayDeque<NsdServiceInfo>()
    private var resolving = false
    private var listener: NsdManager.DiscoveryListener? = null
    private var onChange: ((List<String>) -> Unit)? = null

    val isScanning: Boolean get() = listener != null

    /** Starts a scan; [onChange] receives the current url list on every find. */
    fun start(onChange: (List<String>) -> Unit) {
        if (listener != null) return
        this.onChange = onChange
        urls.clear()
        urlByName.clear()
        attempts.clear()
        if (!multicast.isHeld) multicast.acquire()

        val l = object : NsdManager.DiscoveryListener {
            override fun onDiscoveryStarted(serviceType: String) = Unit
            override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) = stop()
            override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) = stop()
            override fun onDiscoveryStopped(serviceType: String) = Unit

            override fun onServiceFound(info: NsdServiceInfo) {
                if (info.serviceName.lowercase().contains("petalot")) resolve(info)
            }

            override fun onServiceLost(info: NsdServiceInfo) {
                val url = urlByName.remove(info.serviceName) ?: return
                // Another machine may still map to the same address.
                if (urlByName.containsValue(url)) return
                if (urls.remove(url)) emit()
            }
        }
        listener = l
        @Suppress("DEPRECATION")
        nsd.discoverServices("_http._tcp.", NsdManager.PROTOCOL_DNS_SD, l)
    }

    fun stop() {
        listener?.let { runCatching { nsd.stopServiceDiscovery(it) } }
        listener = null
        onChange = null
        queue.clear()
        resolving = false
        if (multicast.isHeld) runCatching { multicast.release() }
    }

    /** Resolves one service at a time: NsdManager rejects concurrent resolves. */
    @Suppress("DEPRECATION")
    private fun pump() {
        if (resolving || queue.isEmpty()) return
        val info = queue.removeFirst()
        resolving = true
        nsd.resolveService(info, object : NsdManager.ResolveListener {
            override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
                // Resolution fails routinely; retry a couple of times before
                // giving up, or the machine never shows up.
                val key = serviceInfo.serviceName
                val n = attempts.getOrDefault(key, 0)
                if (n < 3) {
                    attempts[key] = n + 1
                    queue.addLast(serviceInfo)
                }
                resolving = false
                pump()
            }

            override fun onServiceResolved(serviceInfo: NsdServiceInfo) {
                resolving = false
                val host = serviceInfo.host?.hostAddress
                if (host != null) {
                    val port = if (serviceInfo.port > 0) serviceInfo.port else 80
                    val url = if (port == 80) "http://$host" else "http://$host:$port"
                    urlByName[serviceInfo.serviceName] = url
                    if (urls.add(url)) emit()
                }
                pump()
            }
        })
    }

    private fun resolve(info: NsdServiceInfo) {
        queue.addLast(info)
        pump()
    }

    private fun emit() {
        val snapshot = urls.toList()
        main.post { onChange?.invoke(snapshot) }
    }
}
