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
 * Two steps, to keep it quick: a TCP connect to port 80 to see who is up, then
 * a /get on the few responders to confirm the answer comes from a PETALOT.
 */
object Sweep {

    private const val CONNECT_MS = 500
    private const val HTTP_MS = 2500
    private const val THREADS = 24

    private val main = Handler(Looper.getMainLooper())

    fun find(ctx: Context, onFound: (String) -> Unit, onDone: () -> Unit) {
        val network = Net.wifiNetwork(ctx)
        val own = network?.let { Net.localIpv4(ctx, it) }
        if (network == null || own == null) {
            main.post(onDone)
            return
        }

        val base = own and -256 // the /24 we are on
        val alive = Collections.synchronizedSet(LinkedHashSet<Int>())
        val pool = Executors.newFixedThreadPool(THREADS)
        val pending = AtomicInteger(0)

        for (i in 1..254) {
            val ip = base + i
            if (ip == own) continue
            pending.incrementAndGet()
            pool.execute {
                try {
                    Socket().use { socket ->
                        network.bindSocket(socket)
                        socket.connect(InetSocketAddress(Net.toAddress(ip), 80), CONNECT_MS)
                        alive.add(ip)
                    }
                } catch (_: Exception) {
                    // Nothing listening (or filtered): not a candidate.
                } finally {
                    if (pending.decrementAndGet() == 0) {
                        pool.shutdown()
                        main.post { confirm(network, alive.toList(), onFound, onDone) }
                    }
                }
            }
        }
    }

    /** Keeps only the addresses that answer /get like a PETALOT. */
    private fun confirm(
        network: Network,
        hosts: List<Int>,
        onFound: (String) -> Unit,
        onDone: () -> Unit,
    ) {
        if (hosts.isEmpty()) {
            main.post(onDone)
            return
        }
        val pool = Executors.newFixedThreadPool(THREADS)
        val pending = AtomicInteger(hosts.size)
        for (ip in hosts) {
            pool.execute {
                val url = "http://${Net.toAddress(ip)}"
                if (isPetalot(network, url)) main.post { onFound(url) }
                if (pending.decrementAndGet() == 0) {
                    pool.shutdown()
                    main.post(onDone)
                }
            }
        }
    }

    private fun isPetalot(network: Network, base: String): Boolean = try {
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
