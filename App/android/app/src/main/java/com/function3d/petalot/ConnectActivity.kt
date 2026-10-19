package com.function3d.petalot

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.net.ConnectivityManager
import android.net.Network
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.text.SpannableString
import android.text.Spanned
import android.text.style.ForegroundColorSpan
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ListView
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast

/**
 * Device picker. Machines found over mDNS appear on top, then the ones the
 * user connected to before, each with a dot: green reachable, red not.
 */
class ConnectActivity : Activity() {

    private lateinit var listView: ListView
    private lateinit var adapter: Rows
    private lateinit var status: TextView
    private lateinit var scanBar: ProgressBar
    private lateinit var input: EditText
    private lateinit var discovery: Discovery

    private val saved = mutableListOf<String>()
    private val found = mutableListOf<String>()
    private val swept = LinkedHashSet<String>()
    private val labels = mutableListOf<CharSequence>()
    private val urls = mutableListOf<String>()
    private val online = HashMap<String, Boolean>() // url -> reachable
    private val probing = HashSet<String>()
    private val removed = HashSet<String>() // deleted by the user, do not re-add
    private var apUrl: String? = null
    private var sweepId = 0
    private var sweepRunning = false

    // Lazily: getSystemService() is not usable before the activity is attached.
    private val connectivity by lazy { getSystemService(ConnectivityManager::class.java) }
    private val main = Handler(Looper.getMainLooper())
    private var currentNetwork: Network? = null
    private var searching = false
    private val stopSearching = Runnable {
        searching = false
        rebuild()
    }
    private val networkCallback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) {
            if (network == currentNetwork) return
            currentNetwork = network
            refreshNetwork()
        }

        override fun onLost(network: Network) {
            if (network != currentNetwork) return
            currentNetwork = null
            refreshNetwork()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        discovery = Discovery(this)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(getColor(R.color.bg))
            setPadding(dp(18), dp(24), dp(18), dp(18))
        }

        val title = TextView(this).apply {
            text = getString(R.string.connect_title)
            setTextColor(getColor(R.color.text))
            textSize = 26f
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        }
        val logo = ImageView(this).apply {
            setImageResource(R.drawable.ic_petalot)
            layoutParams = LinearLayout.LayoutParams(dp(34), dp(34)).apply {
                gravity = Gravity.CENTER_VERTICAL
            }
        }
        val version = TextView(this).apply {
            text = versionName()
            setTextColor(getColor(R.color.muted))
            textSize = 12f
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { marginStart = dp(8) }
        }
        val header = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            addView(logo)
            addView(title, LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { marginStart = dp(8) })
            addView(version)
        }

        status = TextView(this).apply {
            setTextColor(getColor(R.color.muted))
            textSize = 12f
            setPadding(0, dp(8), 0, dp(6))
        }

        scanBar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            isIndeterminate = true
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(3))
            visibility = ProgressBar.GONE
        }

        adapter = Rows()
        listView = ListView(this).apply {
            adapter = this@ConnectActivity.adapter
            setBackgroundColor(getColor(R.color.card))
            divider = null
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f)
            setOnItemClickListener { _, _, pos, _ ->
                val url = urls[pos]
                if (online[url] == false) {
                    Toast.makeText(
                        this@ConnectActivity, R.string.device_unreachable, Toast.LENGTH_SHORT
                    ).show()
                } else {
                    connect(url)
                }
            }
            setOnItemLongClickListener { _, _, pos, _ -> confirmRemove(urls[pos]); true }
        }

        input = EditText(this).apply {
            hint = getString(R.string.input_hint)
            setTextColor(getColor(R.color.text))
            setHintTextColor(getColor(R.color.muted))
            inputType = InputType.TYPE_TEXT_VARIATION_URI
            setSingleLine()
        }
        val connectBtn = Button(this).apply {
            text = getString(R.string.connect)
            setTextColor(getColor(R.color.text))
            setBackgroundColor(getColor(R.color.accent))
            setOnClickListener {
                val s = input.text.toString().trim()
                if (s.isNotEmpty()) connect(s) else
                    Toast.makeText(this@ConnectActivity, R.string.enter_address, Toast.LENGTH_SHORT).show()
            }
        }

        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(0, dp(10), 0, 0)
            addView(input, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
            addView(connectBtn)
        }

        val searchBtn = Button(this).apply {
            text = getString(R.string.search)
            setTextColor(getColor(R.color.accent))
            setBackgroundColor(0)
            setPadding(dp(8), 0, dp(4), 0)
            setOnClickListener { search() }
        }
        val statusRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            addView(status, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
            addView(searchBtn)
        }

        root.addView(header)
        root.addView(statusRow)
        root.addView(scanBar)
        root.addView(listView)
        root.addView(row)

        setContentView(root)
        Updates.checkQuietly(this)
    }

    override fun onStart() {
        super.onStart()
        // Registering the callback delivers onAvailable for the current network
        // immediately; record it first so it does not trigger a second search.
        currentNetwork = connectivity.activeNetwork
        connectivity.registerDefaultNetworkCallback(networkCallback)
        search()
    }

    override fun onStop() {
        main.removeCallbacks(stopSearching)
        searching = false
        sweepId++ // ignore whatever the sweep still reports
        sweepRunning = false
        runCatching { connectivity.unregisterNetworkCallback(networkCallback) }
        discovery.stop()
        scanBar.visibility = ProgressBar.GONE
        super.onStop()
    }

    /**
     * One pass: restart mDNS (its sockets are bound to an interface), sweep the
     * subnet, re-check every device and show the scanning bar for a bounded
     * window, so the UI never claims to be searching forever.
     */
    private fun search() {
        refreshApState()
        searching = true
        main.removeCallbacks(stopSearching)
        main.postDelayed(stopSearching, SCAN_MS)
        discovery.stop()
        found.clear()
        discovery.start { urls ->
            // Resolved by mDNS, so it answered a moment ago.
            urls.forEach { removed.remove(it); online[it] = true }
            found.clear()
            found.addAll(urls)
            rebuild()
        }

        // mDNS cannot tell two machines apart (same hostname in the firmware),
        // so the sweep is what actually finds all of them. Results accumulate:
        // a device that stops answering turns red, it does not vanish.
        startSweep()

        reloadSaved()
    }

    private fun startSweep() {
        if (sweepRunning) return
        sweepRunning = true
        val id = sweepId
        Sweep.find(this, { url ->
            if (id == sweepId) {
                // The sweep already got a valid /get from it, so it is
                // reachable right now: mark it green without another round
                // trip. reloadSaved() re-checks it later.
                removed.remove(url)
                swept.add(url)
                online[url] = true
                rebuild()
            }
        }, {
            if (id == sweepId) sweepRunning = false
        })
    }

    /**
     * Fires on every default-network change, so switching Wi-Fi (including to a
     * PETALOT AP) re-scans and re-checks reachability by itself.
     */
    private fun refreshNetwork() {
        main.post { search() }
    }

    private fun refreshApState() {
        apUrl = if (Wifi.onPetalotAp(this, AP_URL)) DeviceStore.normalize(AP_URL) else null
    }

    private fun versionName(): String = try {
        packageManager.getPackageInfo(packageName, 0).versionName ?: ""
    } catch (_: Exception) {
        ""
    }

    private fun reloadSaved() {
        val list = DeviceStore.list(this)
        // The last used device is the most likely target; keep it first.
        DeviceStore.last(this)?.let { last ->
            if (list.remove(last)) list.add(0, last)
        }
        saved.clear()
        saved.addAll(list)

        autofillAp()
        saved.forEach { probe(it) }
        swept.forEach { probe(it) }
        apUrl?.let { probe(it) }
        rebuild()
    }

    /**
     * On the PETALOT AP the machine is always at the AP address, so offer it.
     * Skipped when that address is already saved.
     */
    private fun autofillAp() {
        if (apUrl == null) return
        val target = DeviceStore.normalize(AP_URL)
        if (DeviceStore.list(this).any { it == target }) return
        if (input.text.isNullOrBlank()) input.setText(AP_URL)
    }

    private fun probe(url: String) {
        if (!probing.add(url)) return
        Probe.check(this, url) { reachable ->
            probing.remove(url)
            online[url] = reachable
            rebuild()
        }
    }

    private fun rebuild() {
        val discovered = LinkedHashSet<String>()
        discovered.addAll(found)
        discovered.addAll(swept)
        apUrl?.let { discovered.add(it) }
        discovered.removeAll(removed)

        urls.clear()
        labels.clear()
        // Devices found now go first, then saved ones not already listed.
        discovered.forEach {
            urls.add(it)
            // Reachability is checked for these too, so a machine that goes away
            // turns red instead of disappearing.
            labels.add(row(it, online[it]))
        }
        saved.forEach {
            if (it !in discovered) {
                urls.add(it)
                labels.add(row(it, online[it]))
            }
        }
        adapter.notifyDataSetChanged()

        scanBar.visibility = if (searching) View.VISIBLE else View.GONE

        status.text = when {
            discovered.isNotEmpty() -> getString(R.string.found_count, discovered.size)
            searching -> getString(R.string.scanning)
            saved.isEmpty() -> getString(R.string.saved_empty)
            else -> ""
        }
    }

    private fun row(url: String, reachable: Boolean?): CharSequence {
        val dot = when (reachable) {
            true -> getColor(R.color.accent)
            false -> getColor(R.color.danger)
            null -> getColor(R.color.muted)
        }
        val text = getColor(if (reachable == false) R.color.muted else R.color.text)

        val s = "●  " + hostPort(url)

        val span = SpannableString(s)
        span.setSpan(ForegroundColorSpan(dot), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
        span.setSpan(
            ForegroundColorSpan(text), 2, span.length, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
        )
        return span
    }

    private fun hostPort(url: String): String {
        val uri = Uri.parse(url)
        val host = uri.host ?: url
        val port = uri.port
        return if (port > 0 && port != 80) "$host:$port" else host
    }

    private fun connect(raw: String) {
        val url = DeviceStore.normalize(raw)
        removed.remove(url) // the user wants it back
        DeviceStore.remember(this, url)
        startActivity(
            Intent(this, MainActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP)
                .putExtra(MainActivity.EXTRA_URL, url)
        )
        finish()
    }

    private fun confirmRemove(url: String) {
        AlertDialog.Builder(this)
            .setTitle(R.string.remove_title)
            .setMessage(hostPort(url))
            .setPositiveButton(R.string.remove) { _, _ ->
                // Hide it everywhere so the stale mDNS/sweep result does not put
                // it straight back. It shows again if it answers later.
                removed.add(url)
                DeviceStore.remove(this, url)
                swept.remove(url)
                found.remove(url)
                online.remove(url)
                reloadSaved()
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    @Suppress("OVERRIDE_DEPRECATION")
    override fun onBackPressed() {
        AlertDialog.Builder(this)
            .setTitle(R.string.exit_title)
            .setPositiveButton(R.string.exit_ok) { _, _ -> finishAffinity() }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    /**
     * Row with an optional "x" on the right. It only appears for devices that
     * did not answer, so reachable ones cannot be removed by accident.
     */
    private inner class Rows : BaseAdapter() {
        override fun getCount(): Int = labels.size
        override fun getItem(position: Int): Any = labels[position]
        override fun getItemId(position: Int): Long = position.toLong()

        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val row = convertView as? LinearLayout ?: LinearLayout(this@ConnectActivity).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                setPadding(dp(16), dp(10), dp(6), dp(10))
                addView(TextView(this@ConnectActivity), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
                addView(TextView(this@ConnectActivity))
            }

            (row.getChildAt(0) as TextView).text = labels[position]

            val remove = row.getChildAt(1) as TextView
            if (online[urls[position]] == false) {
                remove.text = "✕"
                remove.contentDescription = getString(R.string.remove)
                remove.setTextColor(getColor(R.color.danger))
                remove.setPadding(dp(14), dp(6), dp(14), dp(6))
                remove.visibility = View.VISIBLE
                remove.setOnClickListener { confirmRemove(urls[position]) }
            } else {
                remove.setOnClickListener(null)
                remove.visibility = View.GONE
            }
            return row
        }
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val AP_URL = "192.168.4.1"
        private const val SCAN_MS = 10000L
    }
}
