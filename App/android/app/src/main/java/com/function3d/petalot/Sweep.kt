package com.function3d.petalot

import android.content.Context
import android.net.Network
import android.os.Handler
import android.os.Looper
import java.net.HttpURLConnection
import java.net.InetSocketAddress
import java.net.Socket
import java.net.URL
import java.util.Collections
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicInteger

/**
 * Finds machines by sweeping the local /24.
 *
 * Needed because the firmware names every machine "petalot" (MDNS.begin with a
 * fixed hostname), so two machines on the same LAN both answer as
 * petalot.local and mDNS can only report one address. A sweep is the only way
 * to list them all.
 *
 * Two passes, so the common case is quick: a fast pass with a short timeout
 * reports the machines that answer promptly, then a slower pass retries only
 * the addresses that stayed silent (a busy ESP can take longer).
 */
object Sweep {

    private const val FAST_MS = 350
    private const val SLOW_MS = 1100
    private const val HTTP_MS = 2500
    private const val THREADS = 48

    private val main = Handler(Looper.getMainLooper())

    /** Callbacks are delivered on the main thread. */
    fun find(ctx: Context, onFound: (String) -> Unit, onDone: () -> Unit) {
        val network = Net.wifiNetwork(ctx)
        val own = network?.let { Net.localIpv4(ctx, it) }
        if (network == null || own == null) {
            main.post(onDone)
            return
        }

        val base = own and -256 // the /24 we are on
        val hosts = (1..254).map { base + it }.filter { it != own }
        val found = Collections.synchronizedSet(HashSet<Int>())
        val silent = Collections.synchronizedList(ArrayList<Int>())

        pass(network, hosts, FAST_MS, found, silent, onFound) {
            // Second chance for the addresses that did not answer quickly.
            pass(network, silent.toList(), SLOW_MS, found, null, onFound) {
                main.post(onDone)
            }
        }
    }

    private fun pass(
        network: Network,
        hosts: List<Int>,
        connectMs: Int,
        found: MutableSet<Int>,
        silent: MutableList<Int>?,
        onFound: (String) -> Unit,
        onDone: () -> Unit,
    ) {
        if (hosts.isEmpty()) {
            onDone()
            return
        }
        val pool = Executors.newFixedThreadPool(THREADS)
        val pending = AtomicInteger(hosts.size)
        for (ip in hosts) {
            pool.execute {
                try {
                    val address = Net.toAddress(ip)
                    // Cheap filter: only hosts with port 80 open are candidates.
                    Socket().use { socket ->
                        network.bindSocket(socket)
                        socket.connect(InetSocketAddress(address, 80), connectMs)
                    }
                    if (found.add(ip)) {
                        val url = "http://$address"
                        if (isPetalot(network, url)) main.post { onFound(url) }
                    }
                } catch (_: Exception) {
                    silent?.add(ip)
                } finally {
                    if (pending.decrementAndGet() == 0) {
                        pool.shutdown()
                        onDone()
                    }
                }
            }
        }
    }

    private fun isPetalot(network: Network, base: String): Boolean {
        // A slow ESP can miss the first request; the host is already known to be
        // up, so a second try is cheap and avoids losing the machine.
        repeat(2) {
            if (isPetalotOnce(network, base)) return true
        }
        return false
    }

    private fun isPetalotOnce(network: Network, base: String): Boolean = try {
        val target = URL("$base/get")
        val conn = network.openConnection(target) as HttpURLConnection
        conn.connectTimeout = HTTP_MS
        conn.readTimeout = HTTP_MS
        conn.requestMethod = "GET"
        val code = conn.responseCode
        val body = if (code == 200) {
            val buf = ByteArray(4096)
            val read = conn.inputStream.use { it.read(buf) }
            if (read > 0) String(buf, 0, read) else ""
        } else {
            ""
        }
        conn.disconnect()
        // /get always carries the firmware version; 401 means auth is on.
        code == 401 || (code == 200 && body.contains("\"version\""))
    } catch (_: Exception) {
        false
    }
}
