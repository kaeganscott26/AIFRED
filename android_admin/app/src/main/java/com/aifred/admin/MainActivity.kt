package com.aifred.admin

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.media.audiofx.Visualizer
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.OpenableColumns
import android.provider.Settings
import android.webkit.MimeTypeMap
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.asRequestBody
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import okio.ByteString
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.util.concurrent.TimeUnit
import java.util.UUID

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            AIFREDAdminApp()
        }
    }
}

enum class AdminTab {
    CHAT,
    UPLOAD,
    COMMAND
}

enum class UploadMode {
    CATALOG,
    REFERENCE,
    WEBSITE_ASSET
}

data class ChatMessage(val role: String, val text: String)

private fun MutableList<ChatMessage>.appendSessionChatMessage(message: ChatMessage) {
    add(message)
}

private fun MutableList<ChatMessage>.appendAssistantToken(token: String) {
    if (isNotEmpty() && last().role == "assistant") {
        val index = lastIndex
        this[index] = this[index].copy(text = this[index].text + token)
    } else {
        appendSessionChatMessage(ChatMessage("assistant", token))
    }
}

data class ModelCatalog(
    val models: List<String>,
    val activeModel: String
)

data class RegisteredAction(
    val id: String,
    val description: String
)

data class ChatWebhookSettings(
    val enabled: Boolean = false,
    val url: String = "",
    val secret: String = "",
    val events: List<String> = listOf("chat.completed", "chat.failed")
)

data class ChatContextSettings(
    val usePreviousResponseId: Boolean = true,
    val memoryWindowItems: String = "40",
    val summaryItems: String = "6",
    val maxPromptChars: String = "4000",
    val compactThreshold: String = "12"
)

data class ChatPromptSettings(
    val tone: String = "direct",
    val personalityMode: String = "professional_mentor",
    val systemPrefix: String = "",
    val systemSuffix: String = ""
)

data class ChatReasoningSettings(
    val enabled: Boolean = true,
    val effort: String = "low"
)

data class ChatResponseSettings(
    val verbosity: String = "low",
    val maxOutputTokens: String = "900"
)

data class ChatSettings(
    val transportMode: String = "websocket",
    val webhook: ChatWebhookSettings = ChatWebhookSettings(),
    val context: ChatContextSettings = ChatContextSettings(),
    val prompt: ChatPromptSettings = ChatPromptSettings(),
    val reasoning: ChatReasoningSettings = ChatReasoningSettings(),
    val response: ChatResponseSettings = ChatResponseSettings()
)

data class ChatSettingsResult(
    val ok: Boolean,
    val websocketUrl: String = "",
    val persistence: String = "",
    val settings: ChatSettings = ChatSettings(),
    val message: String = ""
)

data class SoundPackMeta(
    val packType: String,
    val title: String,
    val description: String,
    val bpm: String,
    val key: String,
    val tempo: String,
    val price: String
)

fun defaultPackPrice(packType: String): String {
    return when (packType.trim().lowercase()) {
        "single" -> "$2.99"
        "soundpack", "midipack", "drumpack", "samplepack" -> "$19.99"
        else -> "$19.99"
    }
}

data class AdminLoginResult(
    val ok: Boolean,
    val username: String,
    val sessionToken: String,
    val message: String
)

data class AdminFileReadResult(
    val ok: Boolean,
    val content: String,
    val message: String
)

data class TrackAnalysisMetrics(
    val loudness: Float = 0.5f,
    val dynamics: Float = 0.5f,
    val tone: Float = 0.5f,
    val stereo: Float = 0.5f,
    val transient: Float = 0.5f,
    val bpmLabel: String = "BPM N/A",
    val summary: String = "Waiting for playback analysis."
)

data class CatalogTrack(
    val key: String,
    val title: String,
    val bpm: String,
    val genre: String,
    val durationLabel: String,
    val streamUrl: String,
    val artworkUrl: String,
    val analysisMetrics: TrackAnalysisMetrics
)

data class WebsitePathPreset(
    val label: String,
    val path: String
)

private val WebsiteTextPresets = listOf(
    WebsitePathPreset("Home HTML", "website/index.html"),
    WebsitePathPreset("Styles", "website/styles.css"),
    WebsitePathPreset("App JS", "website/app.js"),
    WebsitePathPreset("Config", "website/config.js"),
    WebsitePathPreset("Catalog JSON", "website/assets/data/beat_catalog.json")
)

private val WebsiteAssetPresets = listOf(
    WebsitePathPreset("Mascot", "website/assets/brand/aifred-mascot.jpg"),
    WebsitePathPreset("Brand Art", "website/assets/brand/north3rnlight3r-brand.jpg"),
    WebsitePathPreset("Background", "website/assets/brand/north3rnlight3r-background.jpg")
)

private val ReferenceGenres = listOf("rap", "hip-hop", "edm", "dubstep", "pop", "rock")

private const val ADMIN_NOTIFICATION_CHANNEL = "AIFRED_admin_status"
private const val ADMIN_NOTIFICATION_ID = 2207
private const val DEFAULT_ADMIN_USERNAME = "North3rnLight3r"
private const val DEFAULT_ADMIN_PASSWORD = ""
private const val DEFAULT_ADMIN_PASSWORD_SHA256 = "c5c5188f8c698dfa5f956f4883f878a212d882fef0c8aed7c49a12c41d9ad8c5"
private const val ADMIN_PREFS_NAME = "AIFRED_admin_local_config"
private const val ADMIN_PREF_USERNAME = "admin_username"
private const val ADMIN_PREF_PASSWORD = "admin_password"
private val ChatTransportModes = listOf("websocket", "http")
private val ChatTonePresets = listOf("direct", "calm", "technical", "executive", "creative")
private val ChatReasoningEfforts = listOf("minimal", "low", "medium", "high")
private val ChatVerbosityLevels = listOf("low", "medium", "high")

private fun loadConfiguredAdminUsername(context: Context): String {
    val prefs = context.getSharedPreferences(ADMIN_PREFS_NAME, Context.MODE_PRIVATE)
    val stored = prefs.getString(ADMIN_PREF_USERNAME, "").orEmpty().trim()
    if (stored.isNotEmpty()) {
        return stored
    }
    val buildValue = BuildConfig.AIFRED_ADMIN_USERNAME.trim()
    return if (buildValue.isNotEmpty()) buildValue else DEFAULT_ADMIN_USERNAME
}

private fun loadConfiguredAdminPassword(context: Context): String {
    val prefs = context.getSharedPreferences(ADMIN_PREFS_NAME, Context.MODE_PRIVATE)
    val stored = prefs.getString(ADMIN_PREF_PASSWORD, "").orEmpty().trim()
    if (stored.isNotEmpty()) {
        return stored
    }
    val buildValue = BuildConfig.AIFRED_ADMIN_PASSWORD.trim()
    return if (buildValue.isNotEmpty()) buildValue else DEFAULT_ADMIN_PASSWORD
}

private fun saveConfiguredAdminCredentials(context: Context, username: String, password: String) {
    context.getSharedPreferences(ADMIN_PREFS_NAME, Context.MODE_PRIVATE)
        .edit()
        .putString(ADMIN_PREF_USERNAME, username.trim())
        .putString(ADMIN_PREF_PASSWORD, password.trim())
        .apply()
}

private fun sha256Hex(value: String): String {
    val digest = MessageDigest.getInstance("SHA-256").digest(value.toByteArray(Charsets.UTF_8))
    return digest.joinToString("") { "%02x".format(it) }
}

private fun localAdminLogin(username: String, password: String): AdminLoginResult {
    val normalizedUser = username.trim()
    val localUserOk = normalizedUser == DEFAULT_ADMIN_USERNAME
    val savedPasswordOk = password.isNotEmpty() && DEFAULT_ADMIN_PASSWORD.isNotEmpty() && password == DEFAULT_ADMIN_PASSWORD
    val hashOk = sha256Hex(password) == DEFAULT_ADMIN_PASSWORD_SHA256
    return if (localUserOk && (savedPasswordOk || hashOk)) {
        AdminLoginResult(
            ok = true,
            username = normalizedUser,
            sessionToken = "local-admin-${UUID.randomUUID()}",
            message = "admin offline"
        )
    } else {
        AdminLoginResult(ok = false, username = "", sessionToken = "", message = "invalid local admin credentials")
    }
}

private fun uploadModeLabel(mode: UploadMode): String {
    return when (mode) {
        UploadMode.CATALOG -> "Catalog Audio"
        UploadMode.REFERENCE -> "Reference Audio"
        UploadMode.WEBSITE_ASSET -> "Website Asset"
    }
}

private fun queryDisplayName(contentResolver: android.content.ContentResolver, uri: Uri): String {
    return runCatching {
        contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                cursor.getString(0).orEmpty()
            } else {
                ""
            }
        }.orEmpty()
    }.getOrDefault("")
}

private fun extensionForUri(contentResolver: android.content.ContentResolver, uri: Uri): String {
    val mime = contentResolver.getType(uri).orEmpty()
    val fromMime = MimeTypeMap.getSingleton().getExtensionFromMimeType(mime).orEmpty()
    if (fromMime.isNotBlank()) {
        return ".$fromMime"
    }
    val fallback = uri.lastPathSegment.orEmpty().substringAfterLast('.', "")
    return if (fallback.isNotBlank()) ".$fallback" else ""
}

private fun buildUploadFileName(
    contentResolver: android.content.ContentResolver,
    uri: Uri,
    fallbackStem: String
): String {
    val displayName = queryDisplayName(contentResolver, uri).trim()
    if (displayName.isNotBlank()) {
        return displayName
    }
    val extension = extensionForUri(contentResolver, uri)
    return "${fallbackStem}_${UUID.randomUUID()}$extension"
}

private fun firstMeaningfulLine(output: String): String {
    return output
        .lineSequence()
        .map { it.trim() }
        .firstOrNull { line ->
            line.isNotBlank() &&
                line.lowercase() != "stdout:" &&
                line.lowercase() != "stderr:" &&
                line.lowercase() != "error:"
        }
        .orEmpty()
}

private fun describeCommandResult(
    command: String,
    output: String,
    registeredActions: List<RegisteredAction>
): String {
    val normalized = command.trim()
    val detail = firstMeaningfulLine(output).ifBlank { "no output" }
    if (normalized.startsWith("action:")) {
        val actionId = normalized.removePrefix("action:").trim()
        val action = registeredActions.firstOrNull { it.id == actionId }
        val label = action?.description?.ifBlank { actionId } ?: actionId
        return "$label — $detail"
    }
    return "$normalized — $detail"
}

private fun jsonStringList(array: JSONArray?): List<String> {
    if (array == null) {
        return emptyList()
    }
    return buildList {
        for (index in 0 until array.length()) {
            val value = array.optString(index).trim()
            if (value.isNotEmpty()) {
                add(value)
            }
        }
    }
}

private fun parseChatSettings(payload: JSONObject?): ChatSettings {
    val settings = payload ?: JSONObject()
    val webhook = settings.optJSONObject("webhook") ?: JSONObject()
    val context = settings.optJSONObject("context") ?: JSONObject()
    val prompt = settings.optJSONObject("prompt") ?: JSONObject()
    val reasoning = settings.optJSONObject("reasoning") ?: JSONObject()
    val response = settings.optJSONObject("response") ?: JSONObject()
    return ChatSettings(
        transportMode = settings.optString("transport_mode", "websocket").ifBlank { "websocket" },
        webhook = ChatWebhookSettings(
            enabled = webhook.optBoolean("enabled", false),
            url = webhook.optString("url", ""),
            secret = webhook.optString("secret", ""),
            events = jsonStringList(webhook.optJSONArray("events")).ifEmpty { listOf("chat.completed", "chat.failed") }
        ),
        context = ChatContextSettings(
            usePreviousResponseId = context.optBoolean("use_previous_response_id", true),
            memoryWindowItems = context.optInt("memory_window_items", 40).toString(),
            summaryItems = context.optInt("summary_items", 6).toString(),
            maxPromptChars = context.optInt("max_prompt_chars", 4000).toString(),
            compactThreshold = context.optInt("compact_threshold", 12).toString()
        ),
        prompt = ChatPromptSettings(
            tone = prompt.optString("tone", "direct").ifBlank { "direct" },
            personalityMode = prompt.optString("personality_mode", "professional_mentor").ifBlank { "professional_mentor" },
            systemPrefix = prompt.optString("system_prefix", ""),
            systemSuffix = prompt.optString("system_suffix", "")
        ),
        reasoning = ChatReasoningSettings(
            enabled = reasoning.optBoolean("enabled", true),
            effort = reasoning.optString("effort", "low").ifBlank { "low" }
        ),
        response = ChatResponseSettings(
            verbosity = response.optString("verbosity", "low").ifBlank { "low" },
            maxOutputTokens = response.optInt("max_output_tokens", 900).toString()
        )
    )
}

private fun clampMetric(value: Float): Float = value.coerceIn(0f, 1f)

private fun parseTrackAnalysisMetrics(payload: JSONObject?, bpm: String): TrackAnalysisMetrics {
    val metrics = payload?.optJSONObject("analysis_metrics")
    if (metrics == null) {
        val bpmValue = bpm.toFloatOrNull()?.coerceIn(60f, 220f) ?: 100f
        return TrackAnalysisMetrics(
            loudness = 0.5f,
            dynamics = clampMetric((bpmValue - 60f) / 160f),
            tone = 0.5f,
            stereo = 0.5f,
            transient = clampMetric((bpmValue - 70f) / 150f),
            bpmLabel = "BPM ${bpm.ifBlank { "N/A" }}",
            summary = "Live playback bars will update from the current track signal."
        )
    }

    val rmsDb = metrics.optDouble("rms_db", -14.0).toFloat()
    val peakDbfs = metrics.optDouble("peak_dbfs", -1.0).toFloat()
    val crestDb = metrics.optDouble("crest_factor_db", 8.0).toFloat()
    val stereoWidth = metrics.optDouble("stereo_width", 0.5).toFloat()
    val transientDensity = metrics.optDouble("transient_density", 0.5).toFloat()
    val tilt = metrics.optDouble("spectral_tilt", 0.0).toFloat()
    val brightness = metrics.optDouble("brightness", 0.5).toFloat()
    return TrackAnalysisMetrics(
        loudness = clampMetric((rmsDb + 24f) / 24f),
        dynamics = clampMetric((crestDb - 3f) / 12f),
        tone = clampMetric(0.5f + (tilt * 0.08f) + ((brightness - 0.5f) * 0.4f)),
        stereo = clampMetric(stereoWidth),
        transient = clampMetric(transientDensity),
        bpmLabel = "BPM ${bpm.ifBlank { "N/A" }}",
        summary = "Peak ${"%.1f".format(peakDbfs)} dBFS • RMS ${"%.1f".format(rmsDb)} dB • Crest ${"%.1f".format(crestDb)} dB"
    )
}

private fun metricBars(track: CatalogTrack?, live: TrackAnalysisMetrics): List<Pair<String, Float>> {
    val base = track?.analysisMetrics ?: live
    return listOf(
        "Tone" to clampMetric((base.tone * 0.45f) + (live.tone * 0.55f)),
        "Dynamics" to clampMetric((base.dynamics * 0.35f) + (live.dynamics * 0.65f)),
        "Loudness" to clampMetric((base.loudness * 0.25f) + (live.loudness * 0.75f)),
        "Stereo" to clampMetric((base.stereo * 0.7f) + (live.stereo * 0.3f)),
        "Transient" to clampMetric((base.transient * 0.35f) + (live.transient * 0.65f))
    )
}

private fun analyseVisualizerFrame(
    waveform: ByteArray,
    fft: ByteArray,
    currentTrack: CatalogTrack?
): TrackAnalysisMetrics {
    if (waveform.isEmpty() && fft.isEmpty()) {
        return currentTrack?.analysisMetrics ?: TrackAnalysisMetrics()
    }

    var rms = currentTrack?.analysisMetrics?.loudness ?: 0.5f
    var dynamics = currentTrack?.analysisMetrics?.dynamics ?: 0.5f
    var transientMetric = currentTrack?.analysisMetrics?.transient ?: 0.5f
    if (waveform.isNotEmpty()) {
        var rmsSum = 0f
        var peak = 0f
        var transientSum = 0f
        var previous = 0f
        waveform.forEachIndexed { index, byte ->
            val sample = (((byte.toInt() and 0xFF) - 128) / 128f).coerceIn(-1f, 1f)
            rmsSum += sample * sample
            peak = maxOf(peak, kotlin.math.abs(sample))
            if (index > 0) {
                transientSum += kotlin.math.abs(sample - previous)
            }
            previous = sample
        }
        rms = kotlin.math.sqrt(rmsSum / waveform.size.coerceAtLeast(1))
        val crest = peak / (rms + 0.0001f)
        transientMetric = transientSum / waveform.size.coerceAtLeast(1)
        dynamics = clampMetric((crest - 1f) / 5f)
    }

    var low = 0f
    var high = 0f
    var total = 0f
    var binIndex = 0
    var idx = 2
    while (idx + 1 < fft.size) {
        val re = fft[idx].toInt().toFloat()
        val im = fft[idx + 1].toInt().toFloat()
        val magnitude = kotlin.math.sqrt((re * re) + (im * im))
        total += magnitude
        if (binIndex < ((fft.size / 4).coerceAtLeast(1))) {
            low += magnitude
        } else {
            high += magnitude
        }
        idx += 2
        binIndex += 1
    }
    val tone = if (total > 0f) high / total else (currentTrack?.analysisMetrics?.tone ?: 0.5f)
    val widthBase = currentTrack?.analysisMetrics?.stereo ?: 0.5f

    return TrackAnalysisMetrics(
        loudness = clampMetric(rms * 2.5f),
        dynamics = dynamics,
        tone = clampMetric(tone),
        stereo = clampMetric(widthBase),
        transient = clampMetric(transientMetric * 2.5f),
        bpmLabel = currentTrack?.analysisMetrics?.bpmLabel ?: "BPM ${currentTrack?.bpm ?: "N/A"}",
        summary = currentTrack?.analysisMetrics?.summary ?: "Live waveform and FFT metrics are active."
    )
}

private fun renderDashboardSummary(raw: String): String {
    val payload = runCatching { JSONObject(raw) }.getOrNull() ?: return raw.ifBlank { "No site data available." }
    val traffic = payload.optJSONObject("traffic")
    val inquiries = payload.optJSONObject("inquiries")
    val sales = payload.optJSONObject("sales")
    val logs = payload.optJSONObject("logs")
    val recentTraffic = traffic?.optJSONArray("recent")
    val latestInquiry = inquiries?.optJSONArray("latest")?.optJSONObject(0)
    val latestSale = sales?.optJSONArray("latest")?.optJSONObject(0)
    val latestEvents = logs?.optJSONArray("events")
    val latestAdmin = logs?.optJSONArray("adminlog")
    val lines = mutableListOf<String>()
    lines += "Snapshot time: ${payload.optString("snapshot_at", "unknown")}"
    lines += "Page visits: ${traffic?.optInt("page_views", 0) ?: 0}"
    lines += "API requests: ${traffic?.optInt("api_hits", 0) ?: 0}"
    lines += "Audio plays: ${traffic?.optInt("media_streams", 0) ?: 0}"
    lines += "Downloads: ${traffic?.optInt("downloads", 0) ?: 0}"
    lines += "Open inquiries: ${inquiries?.optInt("count", 0) ?: 0}"
    lines += "Recorded sales: ${sales?.optInt("count", 0) ?: 0}"
    val latestTrafficLine = recentTraffic?.optJSONObject(0)?.let { item ->
        "Latest traffic: ${item.optString("kind", "event").replace('_', ' ')} on ${item.optString("path", "/")}"
    }
    val latestEventLine = latestEvents?.optJSONObject(0)?.let { item ->
        "Latest site event: ${item.optString("event_type", "event").replace('.', ' ')}"
    }
    val latestAdminLine = latestAdmin?.optJSONObject(0)?.let { item ->
        "Latest admin action: ${item.optString("command", item.optString("event_type", "admin event")).replace('.', ' ')}"
    }
    val latestInquiryLine = latestInquiry?.let { item ->
        "Latest inquiry: ${item.optString("name", "Someone")} asked about ${item.optString("message", "a message").take(90)}"
    }
    val latestSaleLine = latestSale?.let { item ->
        "Latest sale: ${item.optString("item_name", "item")} for ${item.optString("amount", "0.00")} ${item.optString("currency", "USD")}"
    }
    if (!latestTrafficLine.isNullOrBlank()) {
        lines += latestTrafficLine
    }
    if (!latestInquiryLine.isNullOrBlank()) {
        lines += latestInquiryLine
    }
    if (!latestSaleLine.isNullOrBlank()) {
        lines += latestSaleLine
    }
    if (!latestEventLine.isNullOrBlank()) {
        lines += latestEventLine
    }
    if (!latestAdminLine.isNullOrBlank()) {
        lines += latestAdminLine
    }
    return lines.joinToString("\n")
}

private fun hasFullFileAccessPermission(): Boolean {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        Environment.isExternalStorageManager()
    } else {
        true
    }
}

private fun hasPostNotificationPermission(context: Context): Boolean {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) ==
            PackageManager.PERMISSION_GRANTED
    } else {
        true
    }
}

private fun hasAudioCapturePermission(context: Context): Boolean {
    return ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) ==
        PackageManager.PERMISSION_GRANTED
}

private fun createAdminNotificationChannel(context: Context) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
        return
    }
    val channel = NotificationChannel(
        ADMIN_NOTIFICATION_CHANNEL,
        "AIFRED Admin Status",
        NotificationManager.IMPORTANCE_DEFAULT
    )
    channel.description = "Admin chat, upload, and command status updates"
    val manager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    manager.createNotificationChannel(channel)
}

private fun postAdminNotification(context: Context, title: String, text: String) {
    if (!hasPostNotificationPermission(context)) {
        return
    }
    val notification = NotificationCompat.Builder(context, ADMIN_NOTIFICATION_CHANNEL)
        .setSmallIcon(android.R.drawable.ic_dialog_info)
        .setContentTitle(title)
        .setContentText(text.take(180))
        .setPriority(NotificationCompat.PRIORITY_DEFAULT)
        .setAutoCancel(true)
        .build()
    NotificationManagerCompat.from(context).notify(ADMIN_NOTIFICATION_ID, notification)
}

private fun openFullFileAccessSettings(context: Context) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
        return
    }
    val packageUri = Uri.parse("package:${context.packageName}")
    val appScopeIntent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, packageUri)
    val fallbackIntent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
    val chosen = if (appScopeIntent.resolveActivity(context.packageManager) != null) appScopeIntent else fallbackIntent
    context.startActivity(chosen)
}

private val aifredBrandColors = darkColorScheme(
    primary = Color(0xFF18D2E7),
    onPrimary = Color(0xFF001116),
    secondary = Color(0xFF7BD6C8),
    onSecondary = Color(0xFF041211),
    tertiary = Color(0xFFF6C46D),
    background = Color(0xFF050B12),
    onBackground = Color(0xFFE9F6FF),
    surface = Color(0xFF0B1622),
    onSurface = Color(0xFFE1F2FF),
    surfaceVariant = Color(0xFF122433),
    onSurfaceVariant = Color(0xFFC2DCEF),
    outline = Color(0xFF2E5C72)
)

@Composable
fun AIFREDAdminApp() {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val client = remember { ApiClient(BuildConfig.AIFRED_BASE_URL, BuildConfig.AIFRED_API_TOKEN) }
    val mainExecutor = remember { ContextCompat.getMainExecutor(context) }
    val configuredAdminUser = remember(context) { loadConfiguredAdminUsername(context) }
    val configuredAdminPassword = remember(context) { loadConfiguredAdminPassword(context) }
    val mediaPlayer = remember {
        MediaPlayer().apply {
            setAudioAttributes(
                AudioAttributes.Builder()
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .build()
            )
        }
    }

    var activeTab by remember { mutableStateOf(AdminTab.CHAT) }
    var status by remember { mutableStateOf("online") }
    var fullFileAccess by remember { mutableStateOf(hasFullFileAccessPermission()) }
    var notificationAccess by remember { mutableStateOf(hasPostNotificationPermission(context)) }
    var audioCaptureAccess by remember { mutableStateOf(hasAudioCapturePermission(context)) }

    val chatMessages = remember { mutableStateListOf<ChatMessage>() }
    val chatSessionId = remember { "android-admin-${UUID.randomUUID()}" }
    var chatInput by remember { mutableStateOf("") }
    var chatModels by remember {
        mutableStateOf(
            listOf(
                "gpt-5.2"
            )
        )
    }
    var selectedChatModel by remember { mutableStateOf(chatModels.first()) }
    var chatSettings by remember { mutableStateOf(ChatSettings()) }
    var chatSettingsExpanded by remember { mutableStateOf(false) }
    var chatWebsocketUrl by remember { mutableStateOf("") }
    var chatSettingsPersistence by remember { mutableStateOf("") }
    var registeredActions by remember { mutableStateOf(listOf<RegisteredAction>()) }

    var uploadUri by remember { mutableStateOf<Uri?>(null) }
    var adminUser by remember(configuredAdminUser) { mutableStateOf(configuredAdminUser) }
    var adminPassword by remember(configuredAdminPassword) { mutableStateOf(configuredAdminPassword) }
    var adminSessionToken by remember { mutableStateOf("") }

    var uploadMode by remember { mutableStateOf(UploadMode.CATALOG) }
    var soundPackType by remember { mutableStateOf("soundpack") }
    var soundPackTitle by remember { mutableStateOf("") }
    var soundPackDescription by remember { mutableStateOf("") }
    var soundPackBpm by remember { mutableStateOf("") }
    var soundPackKey by remember { mutableStateOf("") }
    var soundPackTempo by remember { mutableStateOf("") }
    var soundPackPrice by remember { mutableStateOf(defaultPackPrice("soundpack")) }
    var referenceGenre by remember { mutableStateOf("rap") }
    var websiteAssetPath by remember {
        mutableStateOf("website/assets/brand/aifred-mascot.jpg")
    }

    var commandInput by remember { mutableStateOf("curl -s https://www.north3rnlight3r.com/api/v1/health") }
    var commandOutput by remember { mutableStateOf("") }
    var websiteFilePath by remember { mutableStateOf("website/index.html") }
    var websiteFileContent by remember { mutableStateOf("") }
    var websiteAdminOutput by remember { mutableStateOf("") }
    var saleItemName by remember { mutableStateOf("AIFRED VST3 Plugin") }
    var saleAmount by remember { mutableStateOf("29.99") }
    var saleCustomerEmail by remember { mutableStateOf("") }
    var siteDashboardSummary by remember { mutableStateOf("Admin login required for live site data.") }
    var catalogTracks by remember { mutableStateOf(listOf<CatalogTrack>()) }
    var selectedTrack by remember { mutableStateOf<CatalogTrack?>(null) }
    var playerStatus by remember { mutableStateOf("player idle") }
    var playbackProgress by remember { mutableStateOf(0f) }
    var isPlaying by remember { mutableStateOf(false) }
    var liveTrackMetrics by remember { mutableStateOf(TrackAnalysisMetrics()) }
    var visualizer by remember { mutableStateOf<Visualizer?>(null) }
    val notificationPermissionLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        notificationAccess = granted
        status = if (granted) "notifications enabled" else "notifications blocked"
    }
    val audioPermissionLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        audioCaptureAccess = granted
        status = if (granted) "audio visualizer enabled" else "audio visualizer permission denied"
    }

    val picker = rememberLauncherForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uploadUri = uri
        status = if (uri != null) "file selected" else "no file selected"
    }

    LaunchedEffect(Unit) {
        createAdminNotificationChannel(context)
        notificationAccess = hasPostNotificationPermission(context)
        fullFileAccess = hasFullFileAccessPermission()
        audioCaptureAccess = hasAudioCapturePermission(context)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !notificationAccess) {
            notificationPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
        }

        val catalog = withContext(Dispatchers.IO) { client.listModels() }
        if (catalog.models.isNotEmpty()) {
            chatModels = catalog.models
            selectedChatModel = if (catalog.activeModel.isNotBlank()) {
                catalog.activeModel
            } else {
                catalog.models.first()
            }
        }
        val chatSettingsResult = withContext(Dispatchers.IO) { client.getChatSettings() }
        if (chatSettingsResult.ok) {
            chatSettings = chatSettingsResult.settings
            chatWebsocketUrl = chatSettingsResult.websocketUrl
            chatSettingsPersistence = chatSettingsResult.persistence
        } else if (chatSettingsResult.message.isNotBlank()) {
            status = chatSettingsResult.message
        }
        registeredActions = withContext(Dispatchers.IO) { client.listActions() }
        catalogTracks = withContext(Dispatchers.IO) { client.listCatalogTracks() }
        if (selectedTrack == null && catalogTracks.isNotEmpty()) {
            selectedTrack = catalogTracks.first()
            liveTrackMetrics = catalogTracks.first().analysisMetrics
        }

        if (configuredAdminUser.isNotEmpty() && configuredAdminPassword.isNotEmpty()) {
            val result = withContext(Dispatchers.IO) {
                client.adminLogin(configuredAdminUser, configuredAdminPassword)
            }
            if (result.ok) {
                adminUser = result.username
                adminSessionToken = result.sessionToken
                status = "admin online: ${result.username}"
            } else {
                status = result.message
            }
        }

        client.connectChat(
            sessionId = chatSessionId,
            onReady = { provider ->
                chatMessages.appendSessionChatMessage(ChatMessage("system", "connected: $provider"))
                postAdminNotification(context, "AIFRED Admin", "Chat provider: $provider")
            },
            onToken = { token ->
                chatMessages.appendAssistantToken(token)
            },
            onIssue = { issue ->
                chatMessages.appendSessionChatMessage(ChatMessage("system", "issue: $issue"))
            },
            onError = { error ->
                chatMessages.appendSessionChatMessage(ChatMessage("system", "error: $error"))
                postAdminNotification(context, "AIFRED Admin Error", error)
            }
        )
    }

    DisposableEffect(Unit) {
        mediaPlayer.setOnCompletionListener {
            isPlaying = false
            playbackProgress = 0f
            playerStatus = "playback finished"
        }
        onDispose {
            visualizer?.release()
            client.closeChat(chatSessionId)
            chatMessages.clear()
            mediaPlayer.release()
        }
    }

    LaunchedEffect(isPlaying, selectedTrack?.key) {
        while (isPlaying) {
            val duration = mediaPlayer.duration.takeIf { it > 0 } ?: 0
            val position = mediaPlayer.currentPosition.coerceAtLeast(0)
            playbackProgress = if (duration > 0) {
                (position.toFloat() / duration.toFloat()).coerceIn(0f, 1f)
            } else {
                0f
            }
            kotlinx.coroutines.delay(200)
        }
    }

    LaunchedEffect(adminSessionToken) {
        if (adminSessionToken.isBlank()) {
            siteDashboardSummary = "Admin login required for live site data."
            return@LaunchedEffect
        }
        while (adminSessionToken.isNotBlank()) {
            val raw = withContext(Dispatchers.IO) { client.adminDashboardState(adminSessionToken) }
            siteDashboardSummary = renderDashboardSummary(raw)
            kotlinx.coroutines.delay(15000)
        }
    }

    MaterialTheme(colorScheme = aifredBrandColors) {
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            listOf(
                                Color(0xFF03070D),
                                Color(0xFF07121D),
                                Color(0xFF050B12)
                            )
                        )
                    )
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Image(
                        painter = painterResource(id = R.drawable.mascot_logo),
                        contentDescription = "North3rnLight3r mascot",
                        modifier = Modifier.size(44.dp)
                    )
                    Column {
                        Text(
                            text = "AIFRED Admin Command Center",
                            color = MaterialTheme.colorScheme.onBackground,
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Text(
                            text = "North3rnLight3r Control Plane",
                            color = MaterialTheme.colorScheme.secondary
                        )
                    }
                }
                Text(text = status, color = MaterialTheme.colorScheme.secondary)

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Button(
                        modifier = Modifier.weight(1f),
                        onClick = {
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                                notificationPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
                            } else {
                                notificationAccess = true
                            }
                        }
                    ) {
                        Text(if (notificationAccess) "Notifications: On" else "Enable Notifications")
                    }
                    Button(
                        modifier = Modifier.weight(1f),
                        onClick = {
                            openFullFileAccessSettings(context)
                            status = "opened full file access settings"
                        }
                    ) {
                        Text(if (fullFileAccess) "Files: Full Access" else "Enable Full File Access")
                    }
                    Button(
                        modifier = Modifier.weight(1f),
                        onClick = {
                            notificationAccess = hasPostNotificationPermission(context)
                            fullFileAccess = hasFullFileAccessPermission()
                            status = if (fullFileAccess) "access checks refreshed" else "full file access not granted"
                        }
                    ) {
                        Text("Refresh Access")
                    }
                }

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    TabButton(
                        label = "Chat",
                        selected = activeTab == AdminTab.CHAT,
                        modifier = Modifier.weight(1f),
                        onClick = { activeTab = AdminTab.CHAT }
                    )
                    TabButton(
                        label = "Upload",
                        selected = activeTab == AdminTab.UPLOAD,
                        modifier = Modifier.weight(1f),
                        onClick = { activeTab = AdminTab.UPLOAD }
                    )
                    TabButton(
                        label = "Command",
                        selected = activeTab == AdminTab.COMMAND,
                        modifier = Modifier.weight(1f),
                        onClick = { activeTab = AdminTab.COMMAND }
                    )
                }

                when (activeTab) {
                    AdminTab.CHAT -> {
                        ChatScreen(
                            tracks = catalogTracks,
                            selectedTrack = selectedTrack,
                            playerStatus = playerStatus,
                            isPlaying = isPlaying,
                            playbackProgress = playbackProgress,
                            visualMetrics = liveTrackMetrics,
                            onRefreshCatalog = {
                                scope.launch {
                                    playerStatus = "loading catalog"
                                    val tracks = withContext(Dispatchers.IO) { client.listCatalogTracks() }
                                    catalogTracks = tracks
                                    if (tracks.isNotEmpty()) {
                                        selectedTrack = selectedTrack?.let { current ->
                                            tracks.firstOrNull { it.key == current.key }
                                        } ?: tracks.first()
                                        liveTrackMetrics = selectedTrack?.analysisMetrics ?: TrackAnalysisMetrics()
                                    }
                                    playerStatus = if (tracks.isEmpty()) "catalog unavailable" else "catalog loaded"
                                }
                            },
                            onSelectTrack = { track ->
                                selectedTrack = track
                                liveTrackMetrics = track.analysisMetrics
                                playbackProgress = 0f
                                playerStatus = "selected ${track.title}"
                            },
                            onPlayTrack = {
                                val track = selectedTrack
                                if (track == null) {
                                    playerStatus = "select a track"
                                    return@ChatScreen
                                }
                                scope.launch(Dispatchers.Main) {
                                    runCatching {
                                        visualizer?.release()
                                        mediaPlayer.reset()
                                        mediaPlayer.setAudioAttributes(
                                            AudioAttributes.Builder()
                                                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                                                .setUsage(AudioAttributes.USAGE_MEDIA)
                                                .build()
                                        )
                                        mediaPlayer.setDataSource(track.streamUrl)
                                        mediaPlayer.setOnPreparedListener { player ->
                                            player.start()
                                            isPlaying = true
                                            playerStatus = "playing ${track.title}"
                                            val sessionId = player.audioSessionId
                                            if (!audioCaptureAccess) {
                                                audioPermissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                                            }
                                            if (audioCaptureAccess && sessionId != MediaPlayer.MEDIA_ERROR_UNKNOWN && sessionId != 0) {
                                                visualizer = Visualizer(sessionId).apply {
                                                    captureSize = Visualizer.getCaptureSizeRange()[1]
                                                    setDataCaptureListener(
                                                        object : Visualizer.OnDataCaptureListener {
                                                            override fun onWaveFormDataCapture(
                                                                visualizer: Visualizer?,
                                                                waveform: ByteArray?,
                                                                samplingRate: Int
                                                            ) {
                                                                val next = analyseVisualizerFrame(
                                                                    waveform ?: ByteArray(0),
                                                                    ByteArray(0),
                                                                    track
                                                                )
                                                                mainExecutor.execute {
                                                                    liveTrackMetrics = next.copy(
                                                                        stereo = track.analysisMetrics.stereo
                                                                    )
                                                                }
                                                            }

                                                            override fun onFftDataCapture(
                                                                visualizer: Visualizer?,
                                                                fft: ByteArray?,
                                                                samplingRate: Int
                                                            ) {
                                                                val next = analyseVisualizerFrame(
                                                                    ByteArray(0),
                                                                    fft ?: ByteArray(0),
                                                                    track
                                                                )
                                                                mainExecutor.execute {
                                                                    liveTrackMetrics = liveTrackMetrics.copy(
                                                                        tone = next.tone,
                                                                        summary = track.analysisMetrics.summary
                                                                    )
                                                                }
                                                            }
                                                        },
                                                        Visualizer.getMaxCaptureRate() / 2,
                                                        true,
                                                        true
                                                    )
                                                    enabled = true
                                                }
                                            }
                                        }
                                        mediaPlayer.setOnErrorListener { _, _, _ ->
                                            isPlaying = false
                                            playerStatus = "playback failed"
                                            true
                                        }
                                        mediaPlayer.prepareAsync()
                                        playerStatus = "buffering ${track.title}"
                                    }.onFailure { error ->
                                        isPlaying = false
                                        playerStatus = "playback error: ${error.message ?: "unknown error"}"
                                    }
                                }
                            },
                            onPauseTrack = {
                                if (mediaPlayer.isPlaying) {
                                    mediaPlayer.pause()
                                    isPlaying = false
                                    playerStatus = "paused ${selectedTrack?.title ?: "track"}"
                                }
                            },
                            onStopTrack = {
                                runCatching {
                                    if (mediaPlayer.isPlaying) {
                                        mediaPlayer.stop()
                                    }
                                    mediaPlayer.reset()
                                    visualizer?.release()
                                    visualizer = null
                                    isPlaying = false
                                    playbackProgress = 0f
                                    liveTrackMetrics = selectedTrack?.analysisMetrics ?: TrackAnalysisMetrics()
                                    playerStatus = "stopped"
                                }
                            },
                            messages = chatMessages,
                            availableModels = chatModels,
                            selectedModel = selectedChatModel,
                            onModelSelect = { selectedChatModel = it },
                            chatSettings = chatSettings,
                            chatWebsocketUrl = chatWebsocketUrl,
                            chatSettingsPersistence = chatSettingsPersistence,
                            adminSessionActive = adminSessionToken.isNotBlank(),
                            settingsExpanded = chatSettingsExpanded,
                            onToggleSettings = { chatSettingsExpanded = !chatSettingsExpanded },
                            onSettingsChange = { chatSettings = it },
                            onSaveSettings = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required for chat settings"
                                    return@ChatScreen
                                }
                                scope.launch {
                                    status = "saving chat settings"
                                    val result = withContext(Dispatchers.IO) {
                                        client.saveChatSettings(adminSessionToken, chatSettings)
                                    }
                                    if (result.ok) {
                                        chatSettings = result.settings
                                        chatWebsocketUrl = result.websocketUrl
                                        chatSettingsPersistence = result.persistence
                                        status = "chat settings saved"
                                        postAdminNotification(
                                            context,
                                            "Chat Settings Updated",
                                            "Transport ${result.settings.transportMode}, tone ${result.settings.prompt.tone}"
                                        )
                                    } else {
                                        status = result.message.ifBlank { "chat settings save failed" }
                                    }
                                }
                            },
                            chatInput = chatInput,
                            onInput = { chatInput = it },
                            onSend = {
                                val prompt = chatInput.trim()
                                if (prompt.isNotEmpty()) {
                                    chatMessages.appendSessionChatMessage(ChatMessage("user", prompt))
                                    chatInput = ""
                                    val shouldUseWebSocket = chatSettings.transportMode == "websocket"
                                    val sent = shouldUseWebSocket && client.sendChat(prompt, chatSessionId, selectedChatModel)
                                    if (!sent) {
                                        scope.launch {
                                            val directReply = withContext(Dispatchers.IO) {
                                                client.askChat(prompt, chatSessionId, selectedChatModel)
                                            }
                                            chatMessages.appendSessionChatMessage(ChatMessage("assistant", directReply))
                                        }
                                    }
                                }
                            }
                        )
                    }

                    AdminTab.UPLOAD -> {
                        UploadScreen(
                            selected = uploadUri,
                            adminUser = adminUser,
                            adminPassword = adminPassword,
                            uploadMode = uploadMode,
                            soundPackType = soundPackType,
                            soundPackTitle = soundPackTitle,
                            soundPackDescription = soundPackDescription,
                            soundPackBpm = soundPackBpm,
                            soundPackKey = soundPackKey,
                            soundPackTempo = soundPackTempo,
                            soundPackPrice = soundPackPrice,
                            referenceGenre = referenceGenre,
                            websiteAssetPath = websiteAssetPath,
                            onAdminUser = { adminUser = it },
                            onAdminPassword = { adminPassword = it },
                            onUploadMode = { uploadMode = it },
                            onSoundPackType = {
                                soundPackType = it
                                soundPackPrice = defaultPackPrice(it)
                            },
                            onSoundPackTitle = { soundPackTitle = it },
                            onSoundPackDescription = { soundPackDescription = it },
                            onSoundPackBpm = { soundPackBpm = it },
                            onSoundPackKey = { soundPackKey = it },
                            onSoundPackTempo = { soundPackTempo = it },
                            onSoundPackPrice = { soundPackPrice = it },
                            onReferenceGenre = { referenceGenre = it },
                            onWebsiteAssetPath = { websiteAssetPath = it },
                            onPick = {
                                fullFileAccess = hasFullFileAccessPermission()
                                if (!fullFileAccess) {
                                    status = "full file access required"
                                    openFullFileAccessSettings(context)
                                } else {
                                    picker.launch("*/*")
                                }
                            },
                            onSaveAdminCredentials = {
                                val username = adminUser.trim()
                                val password = adminPassword.trim()
                                if (username.isEmpty() || password.isEmpty()) {
                                    status = "admin credentials required"
                                    return@UploadScreen
                                }

                                saveConfiguredAdminCredentials(context, username, password)
                                status = "admin credentials saved locally"
                                postAdminNotification(context, "Admin Credentials Saved", "Saved locally for $username")
                            },
                            onLogin = {
                                val username = adminUser.trim()
                                val password = adminPassword.trim()
                                if (username.isEmpty() || password.isEmpty()) {
                                    status = "admin login required"
                                    return@UploadScreen
                                }

                                scope.launch {
                                    status = "admin login"
                                    val result = withContext(Dispatchers.IO) {
                                        client.adminLogin(username, password)
                                    }
                                    if (result.ok) {
                                        adminSessionToken = result.sessionToken
                                        status = "admin online: ${result.username}"
                                        postAdminNotification(context, "Admin Login", "Logged in as ${result.username}")
                                    } else {
                                        status = result.message
                                        postAdminNotification(context, "Admin Login Failed", result.message)
                                    }
                                }
                            },
                            onUpload = {
                                val uri = uploadUri
                                if (uri == null) {
                                    status = "select file first"
                                    return@UploadScreen
                                }

                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    return@UploadScreen
                                }

                                fullFileAccess = hasFullFileAccessPermission()
                                if (!fullFileAccess) {
                                    status = "full file access required"
                                    openFullFileAccessSettings(context)
                                } else {
                                    scope.launch {
                                        status = "uploading ${uploadModeLabel(uploadMode).lowercase()}"
                                        val result = withContext(Dispatchers.IO) {
                                            when (uploadMode) {
                                                UploadMode.CATALOG -> {
                                                    val meta = SoundPackMeta(
                                                        packType = soundPackType.trim(),
                                                        title = soundPackTitle.trim(),
                                                        description = soundPackDescription.trim(),
                                                        bpm = soundPackBpm.trim(),
                                                        key = soundPackKey.trim(),
                                                        tempo = soundPackTempo.trim(),
                                                        price = soundPackPrice.trim()
                                                    )
                                                    client.uploadSoundPack(
                                                        contentResolver = context.contentResolver,
                                                        uri = uri,
                                                        adminSessionToken = adminSessionToken,
                                                        meta = meta
                                                    )
                                                }
                                                UploadMode.REFERENCE -> {
                                                    client.uploadReferenceTrack(
                                                        contentResolver = context.contentResolver,
                                                        uri = uri,
                                                        adminSessionToken = adminSessionToken,
                                                        genre = referenceGenre.trim(),
                                                        title = soundPackTitle.trim()
                                                    )
                                                }
                                                UploadMode.WEBSITE_ASSET -> {
                                                    client.adminUploadFile(
                                                        contentResolver = context.contentResolver,
                                                        uri = uri,
                                                        adminSessionToken = adminSessionToken,
                                                        targetPath = websiteAssetPath.trim()
                                                    )
                                                }
                                            }
                                        }
                                        status = result
                                        postAdminNotification(context, uploadModeLabel(uploadMode), result)
                                    }
                                }
                            }
                        )
                    }

                    AdminTab.COMMAND -> {
                        CommandScreen(
                            commandInput = commandInput,
                            onInput = { commandInput = it },
                            output = commandOutput,
                            registeredActions = registeredActions,
                            adminSessionToken = adminSessionToken,
                            websiteFilePath = websiteFilePath,
                            websiteFileContent = websiteFileContent,
                            websiteOutput = websiteAdminOutput,
                            siteDashboardSummary = siteDashboardSummary,
                            saleItemName = saleItemName,
                            saleAmount = saleAmount,
                            saleCustomerEmail = saleCustomerEmail,
                            onWebsiteFilePath = { websiteFilePath = it },
                            onWebsiteFileContent = { websiteFileContent = it },
                            onSaleItemName = { saleItemName = it },
                            onSaleAmount = { saleAmount = it },
                            onSaleCustomerEmail = { saleCustomerEmail = it },
                            onRun = {
                                val cmd = commandInput.trim()
                                if (cmd.isNotEmpty()) {
                                    if (adminSessionToken.isBlank()) {
                                        status = "admin login required"
                                        commandOutput = "admin login required"
                                        return@CommandScreen
                                    }
                                    scope.launch {
                                        status = "running command"
                                        commandOutput = withContext(Dispatchers.IO) {
                                            client.runCommand(adminSessionToken, cmd)
                                        }
                                        val summary = describeCommandResult(cmd, commandOutput, registeredActions)
                                        status = summary
                                        postAdminNotification(context, "Admin Command", summary)
                                    }
                                }
                            },
                            onQuick = { quick ->
                                commandInput = quick
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    commandOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "running command"
                                    commandOutput = withContext(Dispatchers.IO) {
                                        client.runCommand(adminSessionToken, quick)
                                    }
                                    val summary = describeCommandResult(quick, commandOutput, registeredActions)
                                    status = summary
                                    postAdminNotification(context, "Admin Command", summary)
                                }
                            },
                            onLoadFile = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                val path = websiteFilePath.trim()
                                if (path.isEmpty()) {
                                    status = "file path required"
                                    websiteAdminOutput = "file path required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "loading website file"
                                    val result = withContext(Dispatchers.IO) {
                                        client.adminReadFile(adminSessionToken, path)
                                    }
                                    websiteAdminOutput = result.message
                                    if (result.ok) {
                                        websiteFileContent = result.content
                                        status = "loaded $path"
                                    } else {
                                        status = result.message
                                    }
                                }
                            },
                            onSaveFile = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                val path = websiteFilePath.trim()
                                if (path.isEmpty()) {
                                    status = "file path required"
                                    websiteAdminOutput = "file path required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "saving and deploying website file"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminWriteFile(adminSessionToken, path, websiteFileContent)
                                    }
                                    status = "saved $path and requested deploy"
                                }
                            },
                            onDeletePath = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                val path = websiteFilePath.trim()
                                if (path.isEmpty()) {
                                    status = "path required"
                                    websiteAdminOutput = "path required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "deleting path"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminDeletePath(adminSessionToken, path)
                                    }
                                    status = "deleted $path"
                                }
                            },
                            onListDir = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                val path = websiteFilePath.trim().ifBlank { "website" }
                                scope.launch {
                                    status = "listing files"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminListFiles(adminSessionToken, path)
                                    }
                                    status = "listed $path"
                                }
                            },
                            onUsePreset = { presetPath ->
                                websiteFilePath = presetPath
                            },
                            onLoadCatalog = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "loading catalog"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminCatalogList(adminSessionToken)
                                    }
                                    status = "catalog loaded"
                                }
                            },
                            onRemoveTrackByKey = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                val key = websiteFilePath.trim()
                                if (key.isEmpty()) {
                                    status = "track key required"
                                    websiteAdminOutput = "track key required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "removing catalog track"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminCatalogRemove(adminSessionToken, key)
                                    }
                                    status = "removed track $key"
                                }
                            },
                            onLoadInquiries = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "loading inquiries"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminInquiriesList(adminSessionToken)
                                    }
                                    status = "inquiries loaded"
                                }
                            },
                            onLoadLogs = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "loading admin log"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminLogsList(adminSessionToken)
                                    }
                                    status = "admin log loaded"
                                }
                            },
                            onLoadSales = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "loading sales"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminSalesList(adminSessionToken)
                                    }
                                    status = "sales loaded"
                                }
                            },
                            onRecordSale = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    websiteAdminOutput = "admin login required"
                                    return@CommandScreen
                                }
                                scope.launch {
                                    status = "recording sale"
                                    websiteAdminOutput = withContext(Dispatchers.IO) {
                                        client.adminRecordSale(
                                            adminSessionToken,
                                            saleItemName.trim(),
                                            saleAmount.trim(),
                                            saleCustomerEmail.trim()
                                        )
                                    }
                                    status = "sale recorded"
                                }
                            },
                            onRefreshDashboard = {
                                if (adminSessionToken.isBlank()) {
                                    status = "admin login required"
                                    siteDashboardSummary = "Admin login required for live site data."
                                    return@CommandScreen
                                }
                                scope.launch {
                                    val raw = withContext(Dispatchers.IO) {
                                        client.adminDashboardState(adminSessionToken)
                                    }
                                    siteDashboardSummary = renderDashboardSummary(raw)
                                    status = "site data refreshed"
                                }
                            }
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun TabButton(
    label: String,
    selected: Boolean,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    Button(
        onClick = onClick,
        modifier = modifier,
        colors = if (selected) {
            ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.primary,
                contentColor = MaterialTheme.colorScheme.onPrimary
            )
        } else {
            ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant,
                contentColor = MaterialTheme.colorScheme.onSurface
            )
        }
    ) {
        Text(label)
    }
}

@Composable
private fun ChoiceButtonGroup(
    label: String,
    options: List<String>,
    selected: String,
    onSelect: (String) -> Unit
) {
    Text(text = label, color = Color(0xFF9CD0EF))
    options.chunked(3).forEach { rowItems ->
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            rowItems.forEach { option ->
                Button(
                    onClick = { onSelect(option) },
                    modifier = Modifier.weight(1f),
                    colors = if (option == selected) {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF18D2E7),
                            contentColor = Color(0xFF001116)
                        )
                    } else {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF122433),
                            contentColor = Color(0xFFE8F3FF)
                        )
                    }
                ) {
                    Text(option)
                }
            }
            repeat(3 - rowItems.size) {
                Box(modifier = Modifier.weight(1f))
            }
        }
    }
}

@Composable
fun ChatScreen(
    tracks: List<CatalogTrack>,
    selectedTrack: CatalogTrack?,
    playerStatus: String,
    isPlaying: Boolean,
    playbackProgress: Float,
    visualMetrics: TrackAnalysisMetrics,
    onRefreshCatalog: () -> Unit,
    onSelectTrack: (CatalogTrack) -> Unit,
    onPlayTrack: () -> Unit,
    onPauseTrack: () -> Unit,
    onStopTrack: () -> Unit,
    messages: List<ChatMessage>,
    availableModels: List<String>,
    selectedModel: String,
    onModelSelect: (String) -> Unit,
    chatSettings: ChatSettings,
    chatWebsocketUrl: String,
    chatSettingsPersistence: String,
    adminSessionActive: Boolean,
    settingsExpanded: Boolean,
    onToggleSettings: () -> Unit,
    onSettingsChange: (ChatSettings) -> Unit,
    onSaveSettings: () -> Unit,
    chatInput: String,
    onInput: (String) -> Unit,
    onSend: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text(text = "Catalog Player + Analysis", color = Color(0xFFE8F3FF), fontWeight = FontWeight.Bold)
        Text(text = playerStatus, color = Color(0xFF8DB0C8))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onRefreshCatalog, modifier = Modifier.weight(1f)) { Text("Refresh Catalog") }
            Button(onClick = onPlayTrack, modifier = Modifier.weight(1f)) { Text("Play") }
            Button(onClick = onPauseTrack, modifier = Modifier.weight(1f)) { Text("Pause") }
            Button(onClick = onStopTrack, modifier = Modifier.weight(1f)) { Text("Stop") }
        }

        Text(
            text = selectedTrack?.let { "${it.title} • BPM ${it.bpm.ifBlank { "N/A" }}${if (it.durationLabel.isNotBlank()) " • ${it.durationLabel}" else ""}" }
                ?: "No track selected",
            color = Color(0xFFE8F3FF)
        )
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(10.dp)
                .background(Color(0xFF122433))
        ) {
            Box(
                modifier = Modifier
                    .fillMaxWidth(playbackProgress.coerceIn(0f, 1f))
                    .height(10.dp)
                    .background(if (isPlaying) Color(0xFF18D2E7) else Color(0xFF2E5C72))
            )
        }

        Text(text = visualMetrics.bpmLabel, color = Color(0xFF8DB0C8))
        metricBars(selectedTrack, visualMetrics).forEach { (label, value) ->
            MetricMeter(label = label, value = value)
        }
        Text(text = visualMetrics.summary, color = Color(0xFF8DB0C8))

        LazyColumn(
            modifier = Modifier
                .fillMaxWidth()
                .height(150.dp)
                .background(Color(0xFF071017))
                .padding(8.dp)
        ) {
            items(tracks) { track ->
                Button(
                    onClick = { onSelectTrack(track) },
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 3.dp),
                    colors = if (selectedTrack?.key == track.key) {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF18D2E7),
                            contentColor = Color(0xFF001116)
                        )
                    } else {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF122433),
                            contentColor = Color(0xFFE8F3FF)
                        )
                    }
                ) {
                    Text("${track.title} • BPM ${track.bpm.ifBlank { "N/A" }}")
                }
            }
        }

        LazyColumn(
            modifier = Modifier
                .fillMaxWidth()
                .height(240.dp)
                .background(Color(0xFF071017))
                .padding(8.dp)
        ) {
            items(messages) { msg ->
                Text(
                    text = "${msg.role}: ${msg.text}",
                    color = if (msg.role == "user") Color(0xFF9CD0EF) else Color(0xFFE8F3FF),
                    modifier = Modifier.padding(vertical = 4.dp)
                )
            }
        }

        Text(text = "Model", color = Color(0xFF9CD0EF))
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            availableModels.forEach { model ->
                Button(
                    onClick = { onModelSelect(model) },
                    modifier = Modifier.weight(1f),
                    colors = if (model == selectedModel) {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF18D2E7),
                            contentColor = Color(0xFF001116)
                        )
                    } else {
                        ButtonDefaults.buttonColors(
                            containerColor = Color(0xFF122433),
                            contentColor = Color(0xFFE8F3FF)
                        )
                    }
                ) {
                    Text(model)
                }
            }
        }

        OutlinedTextField(
            value = chatInput,
            onValueChange = onInput,
            label = { Text("Prompt") },
            modifier = Modifier.fillMaxWidth()
        )

        Button(onClick = onSend, modifier = Modifier.fillMaxWidth()) {
            Text("Send")
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(onClick = onToggleSettings, modifier = Modifier.weight(1f)) {
                Text(if (settingsExpanded) "Hide Chat Settings" else "Show Chat Settings")
            }
            Button(onClick = onSaveSettings, modifier = Modifier.weight(1f)) {
                Text("Save Settings")
            }
        }

        if (settingsExpanded) {
            Text(text = "Runtime Settings", color = Color(0xFFE8F3FF), fontWeight = FontWeight.Bold)
            Text(
                text = "Admin session: ${if (adminSessionActive) "active" else "required"}",
                color = if (adminSessionActive) Color(0xFF8FE0C9) else Color(0xFFEAA4A4)
            )
            Text(
                text = "App WebSocket: ${chatWebsocketUrl.ifBlank { "not provided" }}",
                color = Color(0xFF8DB0C8)
            )
            Text(
                text = "Backend persistence: ${chatSettingsPersistence.ifBlank { "server default" }}",
                color = Color(0xFF8DB0C8)
            )
            Text(
                text = "Webhook events: ${chatSettings.webhook.events.joinToString()}",
                color = Color(0xFF8DB0C8)
            )

            ChoiceButtonGroup(
                label = "Transport Mode",
                options = ChatTransportModes,
                selected = chatSettings.transportMode,
                onSelect = { onSettingsChange(chatSettings.copy(transportMode = it)) }
            )

            ChoiceButtonGroup(
                label = "Tone",
                options = ChatTonePresets,
                selected = chatSettings.prompt.tone,
                onSelect = { onSettingsChange(chatSettings.copy(prompt = chatSettings.prompt.copy(tone = it))) }
            )

            ChoiceButtonGroup(
                label = "Reasoning Effort",
                options = ChatReasoningEfforts,
                selected = chatSettings.reasoning.effort,
                onSelect = { onSettingsChange(chatSettings.copy(reasoning = chatSettings.reasoning.copy(effort = it))) }
            )

            ChoiceButtonGroup(
                label = "Verbosity",
                options = ChatVerbosityLevels,
                selected = chatSettings.response.verbosity,
                onSelect = { onSettingsChange(chatSettings.copy(response = chatSettings.response.copy(verbosity = it))) }
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(
                    onClick = {
                        onSettingsChange(
                            chatSettings.copy(
                                context = chatSettings.context.copy(
                                    usePreviousResponseId = !chatSettings.context.usePreviousResponseId
                                )
                            )
                        )
                    },
                    modifier = Modifier.weight(1f)
                ) {
                    Text(if (chatSettings.context.usePreviousResponseId) "Context: Stateful" else "Context: Manual")
                }
                Button(
                    onClick = {
                        onSettingsChange(
                            chatSettings.copy(reasoning = chatSettings.reasoning.copy(enabled = !chatSettings.reasoning.enabled))
                        )
                    },
                    modifier = Modifier.weight(1f)
                ) {
                    Text(if (chatSettings.reasoning.enabled) "Reasoning: On" else "Reasoning: Off")
                }
                Button(
                    onClick = {
                        onSettingsChange(
                            chatSettings.copy(webhook = chatSettings.webhook.copy(enabled = !chatSettings.webhook.enabled))
                        )
                    },
                    modifier = Modifier.weight(1f)
                ) {
                    Text(if (chatSettings.webhook.enabled) "Webhook: On" else "Webhook: Off")
                }
            }

            OutlinedTextField(
                value = chatSettings.prompt.personalityMode,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(prompt = chatSettings.prompt.copy(personalityMode = it)))
                },
                label = { Text("Personality Mode") },
                modifier = Modifier.fillMaxWidth()
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                OutlinedTextField(
                    value = chatSettings.context.memoryWindowItems,
                    onValueChange = {
                        onSettingsChange(chatSettings.copy(context = chatSettings.context.copy(memoryWindowItems = it)))
                    },
                    label = { Text("Memory Window") },
                    modifier = Modifier.weight(1f)
                )
                OutlinedTextField(
                    value = chatSettings.context.summaryItems,
                    onValueChange = {
                        onSettingsChange(chatSettings.copy(context = chatSettings.context.copy(summaryItems = it)))
                    },
                    label = { Text("Summary Items") },
                    modifier = Modifier.weight(1f)
                )
            }

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                OutlinedTextField(
                    value = chatSettings.context.maxPromptChars,
                    onValueChange = {
                        onSettingsChange(chatSettings.copy(context = chatSettings.context.copy(maxPromptChars = it)))
                    },
                    label = { Text("Max Prompt Chars") },
                    modifier = Modifier.weight(1f)
                )
                OutlinedTextField(
                    value = chatSettings.context.compactThreshold,
                    onValueChange = {
                        onSettingsChange(chatSettings.copy(context = chatSettings.context.copy(compactThreshold = it)))
                    },
                    label = { Text("Compact Threshold") },
                    modifier = Modifier.weight(1f)
                )
            }

            OutlinedTextField(
                value = chatSettings.response.maxOutputTokens,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(response = chatSettings.response.copy(maxOutputTokens = it)))
                },
                label = { Text("Max Output Tokens") },
                modifier = Modifier.fillMaxWidth()
            )

            OutlinedTextField(
                value = chatSettings.webhook.url,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(webhook = chatSettings.webhook.copy(url = it)))
                },
                label = { Text("Webhook URL") },
                modifier = Modifier.fillMaxWidth()
            )

            OutlinedTextField(
                value = chatSettings.webhook.secret,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(webhook = chatSettings.webhook.copy(secret = it)))
                },
                label = { Text("Webhook Secret") },
                visualTransformation = PasswordVisualTransformation(),
                modifier = Modifier.fillMaxWidth()
            )

            OutlinedTextField(
                value = chatSettings.prompt.systemPrefix,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(prompt = chatSettings.prompt.copy(systemPrefix = it)))
                },
                label = { Text("System Prefix") },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(120.dp)
            )

            OutlinedTextField(
                value = chatSettings.prompt.systemSuffix,
                onValueChange = {
                    onSettingsChange(chatSettings.copy(prompt = chatSettings.prompt.copy(systemSuffix = it)))
                },
                label = { Text("System Suffix") },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(120.dp)
            )
        }
    }
}

@Composable
fun MetricMeter(label: String, value: Float) {
    Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(label, color = Color(0xFFE8F3FF))
            Text("${(value * 100f).toInt()}%", color = Color(0xFF8DB0C8))
        }
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(8.dp)
                .background(Color(0xFF122433))
        ) {
            Box(
                modifier = Modifier
                    .fillMaxWidth(value.coerceIn(0f, 1f))
                    .height(8.dp)
                    .background(
                        when {
                            value >= 0.8f -> Color(0xFF39E18D)
                            value >= 0.55f -> Color(0xFF18D2E7)
                            value >= 0.35f -> Color(0xFFF6C46D)
                            else -> Color(0xFFE05E5E)
                        }
                    )
            )
        }
    }
}

@Composable
fun UploadScreen(
    selected: Uri?,
    adminUser: String,
    adminPassword: String,
    uploadMode: UploadMode,
    soundPackType: String,
    soundPackTitle: String,
    soundPackDescription: String,
    soundPackBpm: String,
    soundPackKey: String,
    soundPackTempo: String,
    soundPackPrice: String,
    referenceGenre: String,
    websiteAssetPath: String,
    onAdminUser: (String) -> Unit,
    onAdminPassword: (String) -> Unit,
    onUploadMode: (UploadMode) -> Unit,
    onSoundPackType: (String) -> Unit,
    onSoundPackTitle: (String) -> Unit,
    onSoundPackDescription: (String) -> Unit,
    onSoundPackBpm: (String) -> Unit,
    onSoundPackKey: (String) -> Unit,
    onSoundPackTempo: (String) -> Unit,
    onSoundPackPrice: (String) -> Unit,
    onReferenceGenre: (String) -> Unit,
    onWebsiteAssetPath: (String) -> Unit,
    onPick: () -> Unit,
    onSaveAdminCredentials: () -> Unit,
    onLogin: () -> Unit,
    onUpload: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        OutlinedTextField(
            value = adminUser,
            onValueChange = onAdminUser,
            label = { Text("Admin Username") },
            modifier = Modifier.fillMaxWidth()
        )

        OutlinedTextField(
            value = adminPassword,
            onValueChange = onAdminPassword,
            label = { Text("Admin Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onSaveAdminCredentials, modifier = Modifier.weight(1f)) {
                Text("Save Credentials")
            }
            Button(onClick = onLogin, modifier = Modifier.weight(1f)) {
                Text("Login")
            }
        }

        Text(
            text = "Credentials save locally on this device and can be changed here without an active admin session.",
            color = Color(0xFF8DB0C8)
        )

        Text(text = "Upload Target", color = Color(0xFF8DB0C8))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(
                onClick = { onUploadMode(UploadMode.CATALOG) },
                modifier = Modifier.weight(1f),
                colors = if (uploadMode == UploadMode.CATALOG) {
                    ButtonDefaults.buttonColors(containerColor = Color(0xFF18D2E7), contentColor = Color(0xFF001116))
                } else {
                    ButtonDefaults.buttonColors()
                }
            ) {
                Text("Catalog")
            }
            Button(
                onClick = { onUploadMode(UploadMode.REFERENCE) },
                modifier = Modifier.weight(1f),
                colors = if (uploadMode == UploadMode.REFERENCE) {
                    ButtonDefaults.buttonColors(containerColor = Color(0xFF18D2E7), contentColor = Color(0xFF001116))
                } else {
                    ButtonDefaults.buttonColors()
                }
            ) {
                Text("Reference")
            }
            Button(
                onClick = { onUploadMode(UploadMode.WEBSITE_ASSET) },
                modifier = Modifier.weight(1f),
                colors = if (uploadMode == UploadMode.WEBSITE_ASSET) {
                    ButtonDefaults.buttonColors(containerColor = Color(0xFF18D2E7), contentColor = Color(0xFF001116))
                } else {
                    ButtonDefaults.buttonColors()
                }
            ) {
                Text("Website")
            }
        }

        Text(text = selected?.toString() ?: "No file selected", color = Color(0xFF8DB0C8))
        Button(onClick = onPick, modifier = Modifier.fillMaxWidth()) {
            Text("Select ${uploadModeLabel(uploadMode)} File")
        }

        when (uploadMode) {
            UploadMode.CATALOG -> {
                OutlinedTextField(
                    value = soundPackType,
                    onValueChange = onSoundPackType,
                    label = { Text("Pack Type (soundpack/midipack/drumpack/samplepack/single)") },
                    modifier = Modifier.fillMaxWidth()
                )

                OutlinedTextField(
                    value = soundPackTitle,
                    onValueChange = onSoundPackTitle,
                    label = { Text("Track Title") },
                    modifier = Modifier.fillMaxWidth()
                )

                OutlinedTextField(
                    value = soundPackDescription,
                    onValueChange = onSoundPackDescription,
                    label = { Text("Description") },
                    modifier = Modifier.fillMaxWidth()
                )

                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedTextField(
                        value = soundPackBpm,
                        onValueChange = onSoundPackBpm,
                        label = { Text("BPM") },
                        modifier = Modifier.weight(1f)
                    )
                    OutlinedTextField(
                        value = soundPackKey,
                        onValueChange = onSoundPackKey,
                        label = { Text("Key") },
                        modifier = Modifier.weight(1f)
                    )
                    OutlinedTextField(
                        value = soundPackTempo,
                        onValueChange = onSoundPackTempo,
                        label = { Text("Tempo") },
                        modifier = Modifier.weight(1f)
                    )
                }

                OutlinedTextField(
                    value = soundPackPrice,
                    onValueChange = onSoundPackPrice,
                    label = { Text("Price") },
                    modifier = Modifier.fillMaxWidth()
                )
            }
            UploadMode.REFERENCE -> {
                OutlinedTextField(
                    value = soundPackTitle,
                    onValueChange = onSoundPackTitle,
                    label = { Text("Reference Track Title") },
                    modifier = Modifier.fillMaxWidth()
                )
                Text(text = "Reference Genre", color = Color(0xFF8DB0C8))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
                    ReferenceGenres.forEach { genre ->
                        Button(
                            onClick = { onReferenceGenre(genre) },
                            modifier = Modifier.weight(1f),
                            colors = if (referenceGenre == genre) {
                                ButtonDefaults.buttonColors(containerColor = Color(0xFF18D2E7), contentColor = Color(0xFF001116))
                            } else {
                                ButtonDefaults.buttonColors()
                            }
                        ) {
                            Text(genre)
                        }
                    }
                }
                OutlinedTextField(
                    value = referenceGenre,
                    onValueChange = onReferenceGenre,
                    label = { Text("Genre") },
                    modifier = Modifier.fillMaxWidth()
                )
                Text(
                    text = "Uploads go to the licensed reference intake path and can be rebuilt with the reference actions.",
                    color = Color(0xFF8DB0C8)
                )
            }
            UploadMode.WEBSITE_ASSET -> {
                Text(text = "Website Asset Target", color = Color(0xFF8DB0C8))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
                    WebsiteAssetPresets.forEach { preset ->
                        Button(
                            onClick = { onWebsiteAssetPath(preset.path) },
                            modifier = Modifier.weight(1f)
                        ) {
                            Text(preset.label)
                        }
                    }
                }
                OutlinedTextField(
                    value = websiteAssetPath,
                    onValueChange = onWebsiteAssetPath,
                    label = { Text("Repo-relative target path") },
                    modifier = Modifier.fillMaxWidth()
                )
                Text(
                    text = "Use this for album art, banners, hero images, and any website photo replacement.",
                    color = Color(0xFF8DB0C8)
                )
            }
        }

        Button(onClick = onUpload, modifier = Modifier.fillMaxWidth()) {
            Text("Upload ${uploadModeLabel(uploadMode)}")
        }
    }
}

@Composable
fun CommandScreen(
    commandInput: String,
    onInput: (String) -> Unit,
    output: String,
    registeredActions: List<RegisteredAction>,
    adminSessionToken: String,
    websiteFilePath: String,
    websiteFileContent: String,
    websiteOutput: String,
    siteDashboardSummary: String,
    saleItemName: String,
    saleAmount: String,
    saleCustomerEmail: String,
    onWebsiteFilePath: (String) -> Unit,
    onWebsiteFileContent: (String) -> Unit,
    onSaleItemName: (String) -> Unit,
    onSaleAmount: (String) -> Unit,
    onSaleCustomerEmail: (String) -> Unit,
    onRun: () -> Unit,
    onQuick: (String) -> Unit,
    onLoadFile: () -> Unit,
    onSaveFile: () -> Unit,
    onDeletePath: () -> Unit,
    onListDir: () -> Unit,
    onUsePreset: (String) -> Unit,
    onLoadCatalog: () -> Unit,
    onRemoveTrackByKey: () -> Unit,
    onLoadInquiries: () -> Unit,
    onLoadLogs: () -> Unit,
    onLoadSales: () -> Unit,
    onRecordSale: () -> Unit,
    onRefreshDashboard: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        OutlinedTextField(
            value = commandInput,
            onValueChange = onInput,
            label = { Text("Command") },
            modifier = Modifier.fillMaxWidth()
        )

        Button(onClick = onRun, modifier = Modifier.fillMaxWidth()) {
            Text("Run")
        }

        Text(
            text = output.ifEmpty { "Command output will appear here." },
            color = Color(0xFF9CD0EF),
            modifier = Modifier
                .fillMaxWidth()
                .height(220.dp)
                .background(Color(0xFF071017))
                .verticalScroll(rememberScrollState())
                .padding(8.dp)
        )

        registeredActions.forEach { action ->
            Button(onClick = { onQuick("action:${action.id}") }, modifier = Modifier.fillMaxWidth()) {
                Text("${action.id} — ${action.description}")
            }
        }

        Text(
            text = "Local shell works after offline admin login. Useful commands: pwd, ls, ls /sdcard, id, getprop ro.build.version.release, df -h, pm list packages, input keyevent 3.",
            color = Color(0xFF8DB0C8)
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = { onQuick("pwd && ls") }, modifier = Modifier.weight(1f)) { Text("Local Files") }
            Button(onClick = { onQuick("getprop ro.build.version.release && id") }, modifier = Modifier.weight(1f)) { Text("Device Info") }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Text(
                text = "Live Site Data",
                color = Color(0xFFE8F3FF),
                fontWeight = FontWeight.Bold,
                modifier = Modifier.weight(1f)
            )
            Button(onClick = onRefreshDashboard) {
                Text("Refresh")
            }
        }
        Text(
            text = siteDashboardSummary,
            color = Color(0xFF9CD0EF),
            modifier = Modifier
                .fillMaxWidth()
                .height(180.dp)
                .background(Color(0xFF071017))
                .verticalScroll(rememberScrollState())
                .padding(8.dp)
        )

        Text(
            text = if (adminSessionToken.isBlank()) "Admin login required for website control." else "Website admin session active.",
            color = if (adminSessionToken.isBlank()) Color(0xFFEAA4A4) else Color(0xFF8FE0C9)
        )

        Text(text = "Website File Control", color = Color(0xFF8DB0C8))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            WebsiteTextPresets.take(3).forEach { preset ->
                Button(onClick = { onUsePreset(preset.path) }, modifier = Modifier.weight(1f)) {
                    Text(preset.label)
                }
            }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            WebsiteTextPresets.drop(3).forEach { preset ->
                Button(onClick = { onUsePreset(preset.path) }, modifier = Modifier.weight(1f)) {
                    Text(preset.label)
                }
            }
        }

        OutlinedTextField(
            value = websiteFilePath,
            onValueChange = onWebsiteFilePath,
            label = { Text("File Path / Track Key") },
            modifier = Modifier.fillMaxWidth()
        )

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onLoadFile, modifier = Modifier.weight(1f)) { Text("Load File") }
            Button(onClick = onSaveFile, modifier = Modifier.weight(1f)) { Text("Save + Deploy") }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onListDir, modifier = Modifier.weight(1f)) { Text("List Dir") }
            Button(onClick = onDeletePath, modifier = Modifier.weight(1f)) { Text("Delete Path") }
        }
        Button(
            onClick = { onQuick("action:deploy:site") },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Deploy Site Now")
        }

        OutlinedTextField(
            value = websiteFileContent,
            onValueChange = onWebsiteFileContent,
            label = { Text("File Content") },
            modifier = Modifier
                .fillMaxWidth()
                .height(240.dp)
        )

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onLoadCatalog, modifier = Modifier.weight(1f)) { Text("List Tracks") }
            Button(onClick = onRemoveTrackByKey, modifier = Modifier.weight(1f)) { Text("Remove Track Key") }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            Button(onClick = onLoadInquiries, modifier = Modifier.weight(1f)) { Text("Inquiries") }
            Button(onClick = onLoadLogs, modifier = Modifier.weight(1f)) { Text("ADMINLOG") }
            Button(onClick = onLoadSales, modifier = Modifier.weight(1f)) { Text("Sales") }
        }

        OutlinedTextField(
            value = saleItemName,
            onValueChange = onSaleItemName,
            label = { Text("Sale Item") },
            modifier = Modifier.fillMaxWidth()
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            OutlinedTextField(
                value = saleAmount,
                onValueChange = onSaleAmount,
                label = { Text("Amount") },
                modifier = Modifier.weight(1f)
            )
            OutlinedTextField(
                value = saleCustomerEmail,
                onValueChange = onSaleCustomerEmail,
                label = { Text("Customer Email") },
                modifier = Modifier.weight(1f)
            )
        }
        Button(onClick = onRecordSale, modifier = Modifier.fillMaxWidth()) {
            Text("Record Sale + Receipt")
        }

        Text(
            text = websiteOutput.ifBlank { "Website admin results will appear here." },
            color = Color(0xFF9CD0EF),
            modifier = Modifier
                .fillMaxWidth()
                .height(260.dp)
                .background(Color(0xFF071017))
                .verticalScroll(rememberScrollState())
                .padding(8.dp)
        )
    }
}

class ApiClient(private val baseUrl: String, private val token: String) {
    private val client = OkHttpClient.Builder().build()
    private var ws: WebSocket? = null

    private fun endpoint(path: String): String {
        return "${baseUrl.trimEnd('/')}$path"
    }

    private fun isDirectOllama(): Boolean {
        val normalized = baseUrl.lowercase().trimEnd('/')
        return normalized.contains(":11434") || (normalized.contains("ollama") && normalized.endsWith("/api"))
    }

    private fun isDirectOpenAI(): Boolean {
        return baseUrl.lowercase().contains("api.openai.com")
    }

    fun connectChat(
        sessionId: String,
        onReady: (String) -> Unit,
        onToken: (String) -> Unit,
        onIssue: (String) -> Unit,
        onError: (String) -> Unit
    ) {
        if (isDirectOllama() || isDirectOpenAI()) {
            onReady(if (isDirectOllama()) "ollama direct" else "openai direct")
            onError("Direct provider mode uses HTTP chat from Send; WebSocket is only for the website backend.")
            return
        }

        val wsUrl = baseUrl
            .replace("https://", "wss://")
            .replace("http://", "ws://")
            .trimEnd('/') + "/ws/chat" + if (token.isNotBlank()) "?token=$token" else ""

        val request = Request.Builder().url(wsUrl).build()
        ws = client.newWebSocket(request, object : WebSocketListener() {
            override fun onMessage(webSocket: WebSocket, text: String) {
                val payload = JSONObject(text)
                when (payload.optString("type")) {
                    "chat.ready" -> onReady(payload.optString("provider", "openai"))
                    "chat.token" -> onToken(payload.optString("text"))
                    "issue.object" -> onIssue(payload.optJSONObject("issue")?.optString("summary") ?: "issue")
                    "chat.error" -> onError(payload.optString("message", "error"))
                }
            }

            override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: okhttp3.Response?) {
                onError("websocket unavailable: ${t.message ?: "unknown error"}")
            }
        })

        sendChat("Session start", sessionId, "")
    }

    fun clearChatMemory(sessionId: String) {
        runCatching {
            val body = JSONObject().put("session_id", sessionId).toString()
            val request = Request.Builder()
                .url(endpoint("/api/v1/memory/clear"))
                .addHeader("Content-Type", "application/json")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()
            client.newCall(request).execute().close()
        }
    }

    fun closeChat(sessionId: String) {
        runCatching {
            ws?.send(
                JSONObject()
                    .put("type", "chat.clear")
                    .put("session_id", sessionId)
                    .toString()
            )
        }
        runCatching {
            ws?.close(1000, "session end")
        }
        ws = null
        Thread {
            clearChatMemory(sessionId)
        }.start()
    }

    fun sendChat(prompt: String, sessionId: String, model: String = ""): Boolean {
        val payload = JSONObject()
            .put("type", "chat.send")
            .put("text", prompt)
            .put("session_id", sessionId)
            .put("model", model)
        return ws?.send(payload.toString()) == true
    }

    fun askChat(prompt: String, sessionId: String, model: String = ""): String {
        if (isDirectOllama()) {
            return askOllamaDirect(prompt, model)
        }
        if (isDirectOpenAI()) {
            return askOpenAIDirect(prompt, model)
        }

        return try {
            val body = JSONObject()
                .put("prompt", prompt)
                .put("session_id", sessionId)
                .put("model", model)
                .toString()

            val request = Request.Builder()
                .url(endpoint("/api/v1/chat/ask"))
                .addHeader("Content-Type", "application/json")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                if (response.isSuccessful && payload?.optBoolean("ok") == true) {
                    payload.optString("summary")
                        .ifBlank { payload.optString("answer") }
                        .ifBlank { raw.ifEmpty { "ok" } }
                } else {
                    payload?.optString("error", "chat request failed") ?: raw.ifEmpty { "chat request failed" }
                }
            }
        } catch (error: Exception) {
            "chat network error: ${error.message ?: "unknown error"}"
        }
    }

    private fun askOllamaDirect(prompt: String, model: String): String {
        return try {
            val body = JSONObject()
                .put("model", model.ifBlank { "llama3.1" })
                .put("stream", false)
                .put(
                    "messages",
                    JSONArray()
                        .put(JSONObject().put("role", "system").put("content", "You are AIFRED, the North3rnLight3r admin assistant. Be direct, technical, and useful."))
                        .put(JSONObject().put("role", "user").put("content", prompt))
                )
                .toString()
            val request = Request.Builder()
                .url("${baseUrl.trimEnd('/')}/api/chat")
                .addHeader("Content-Type", "application/json")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                if (response.isSuccessful && payload != null) {
                    payload.optJSONObject("message")?.optString("content")
                        ?.ifBlank { payload.optString("response") }
                        ?.ifBlank { raw }
                        ?: raw
                } else {
                    payload?.optString("error") ?: raw.ifBlank { "Ollama request failed" }
                }
            }
        } catch (error: Exception) {
            "Ollama direct error: ${error.message ?: "unknown error"}"
        }
    }

    private fun askOpenAIDirect(prompt: String, model: String): String {
        if (token.isBlank()) {
            return "OpenAI direct mode requires AIFRED_API_TOKEN to contain an OpenAI API key."
        }
        return try {
            val body = JSONObject()
                .put("model", model.ifBlank { "gpt-5.2" })
                .put(
                    "messages",
                    JSONArray()
                        .put(JSONObject().put("role", "system").put("content", "You are AIFRED, the North3rnLight3r admin assistant. Be direct, technical, and useful."))
                        .put(JSONObject().put("role", "user").put("content", prompt))
                )
                .toString()
            val request = Request.Builder()
                .url("${baseUrl.trimEnd('/')}/chat/completions")
                .addHeader("Content-Type", "application/json")
                .addHeader("Authorization", "Bearer $token")
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                if (response.isSuccessful && payload != null) {
                    payload.optJSONArray("choices")
                        ?.optJSONObject(0)
                        ?.optJSONObject("message")
                        ?.optString("content")
                        ?.ifBlank { raw }
                        ?: raw
                } else {
                    payload?.optJSONObject("error")?.optString("message") ?: raw.ifBlank { "OpenAI request failed" }
                }
            }
        } catch (error: Exception) {
            "OpenAI direct error: ${error.message ?: "unknown error"}"
        }
    }

    private fun parseChatSettingsResult(raw: String, responseOk: Boolean): ChatSettingsResult {
        val payload = runCatching { JSONObject(raw.ifEmpty { "{}" }) }.getOrElse { JSONObject() }
        val settings = parseChatSettings(payload.optJSONObject("settings"))
        return ChatSettingsResult(
            ok = responseOk && payload.optBoolean("ok", false),
            websocketUrl = payload.optString("websocket_url", ""),
            persistence = payload.optString("persistence", ""),
            settings = settings,
            message = payload.optString("error", payload.optString("message", ""))
        )
    }

    fun listModels(): ModelCatalog {
        if (isDirectOllama()) {
            return try {
                val request = Request.Builder().url("${baseUrl.trimEnd('/')}/api/tags").build()
                client.newCall(request).execute().use { response ->
                    val raw = response.body?.string().orEmpty()
                    val payload = runCatching { JSONObject(raw.ifEmpty { "{}" }) }.getOrNull()
                    val items = payload?.optJSONArray("models")
                    val models = buildList {
                        if (items != null) {
                            for (index in 0 until items.length()) {
                                val name = items.optJSONObject(index)?.optString("name").orEmpty()
                                if (name.isNotBlank()) add(name)
                            }
                        }
                    }.ifEmpty { listOf("llama3.1") }
                    ModelCatalog(models, models.first())
                }
            } catch (_error: Exception) {
                ModelCatalog(listOf("llama3.1"), "llama3.1")
            }
        }
        if (isDirectOpenAI()) {
            return try {
                val request = Request.Builder()
                    .url("${baseUrl.trimEnd('/')}/models")
                    .addHeader("Authorization", "Bearer $token")
                    .build()
                client.newCall(request).execute().use { response ->
                    val raw = response.body?.string().orEmpty()
                    val payload = runCatching { JSONObject(raw.ifEmpty { "{}" }) }.getOrNull()
                    val items = payload?.optJSONArray("data")
                    val models = buildList {
                        if (items != null) {
                            for (index in 0 until items.length()) {
                                val id = items.optJSONObject(index)?.optString("id").orEmpty()
                                if (id.isNotBlank() && id.startsWith("gpt")) add(id)
                            }
                        }
                    }.ifEmpty { listOf("gpt-5.2") }
                    ModelCatalog(models, models.first())
                }
            } catch (_error: Exception) {
                ModelCatalog(listOf("gpt-5.2"), "gpt-5.2")
            }
        }

        return try {
            val request = Request.Builder()
                .url(endpoint("/api/v1/models/list"))
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw.ifEmpty { "{}" }) }.getOrNull()
                val models = jsonStringList(payload?.optJSONArray("models"))
                if (response.isSuccessful && payload?.optBoolean("ok") == true && models.isNotEmpty()) {
                    ModelCatalog(
                        models = models,
                        activeModel = payload.optString("active_model", models.first())
                    )
                } else {
                    ModelCatalog(listOf("gpt-5.2"), "gpt-5.2")
                }
            }
        } catch (_error: Exception) {
            ModelCatalog(listOf("gpt-5.2"), "gpt-5.2")
        }
    }

    fun getChatSettings(): ChatSettingsResult {
        return try {
            val request = Request.Builder()
                .url(endpoint("/api/v1/chat/settings"))
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .build()
            client.newCall(request).execute().use { response ->
                parseChatSettingsResult(response.body?.string().orEmpty(), response.isSuccessful)
            }
        } catch (error: Exception) {
            ChatSettingsResult(
                ok = false,
                message = "chat settings network error: ${error.message ?: "unknown error"}"
            )
        }
    }

    fun saveChatSettings(adminSessionToken: String, settings: ChatSettings): ChatSettingsResult {
        return try {
            val body = JSONObject()
                .put("transport_mode", settings.transportMode)
                .put(
                    "webhook",
                    JSONObject()
                        .put("enabled", settings.webhook.enabled)
                        .put("url", settings.webhook.url)
                        .put("secret", settings.webhook.secret)
                        .put("events", JSONArray(settings.webhook.events))
                )
                .put(
                    "context",
                    JSONObject()
                        .put("use_previous_response_id", settings.context.usePreviousResponseId)
                        .put("memory_window_items", settings.context.memoryWindowItems)
                        .put("summary_items", settings.context.summaryItems)
                        .put("max_prompt_chars", settings.context.maxPromptChars)
                        .put("compact_threshold", settings.context.compactThreshold)
                )
                .put(
                    "prompt",
                    JSONObject()
                        .put("tone", settings.prompt.tone)
                        .put("personality_mode", settings.prompt.personalityMode)
                        .put("system_prefix", settings.prompt.systemPrefix)
                        .put("system_suffix", settings.prompt.systemSuffix)
                )
                .put(
                    "reasoning",
                    JSONObject()
                        .put("enabled", settings.reasoning.enabled)
                        .put("effort", settings.reasoning.effort)
                )
                .put(
                    "response",
                    JSONObject()
                        .put("verbosity", settings.response.verbosity)
                        .put("max_output_tokens", settings.response.maxOutputTokens)
                )

            val request = Request.Builder()
                .url(endpoint("/api/v1/admin/chat/settings/save"))
                .addHeader("Content-Type", "application/json")
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .post(body.toString().toRequestBody("application/json".toMediaType()))
                .build()

            client.newCall(request).execute().use { response ->
                parseChatSettingsResult(response.body?.string().orEmpty(), response.isSuccessful)
            }
        } catch (error: Exception) {
            ChatSettingsResult(
                ok = false,
                settings = settings,
                message = "chat settings save error: ${error.message ?: "unknown error"}"
            )
        }
    }

    fun listCatalogTracks(): List<CatalogTrack> {
        return try {
            val request = Request.Builder()
                .url(endpoint("/api/v1/catalog/list"))
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                val items = payload?.optJSONArray("tracks")
                if (!response.isSuccessful || items == null) {
                    emptyList()
                } else {
                    buildList {
                        for (index in 0 until items.length()) {
                            val item = items.optJSONObject(index) ?: continue
                            val key = item.optString("key").trim()
                            val title = item.optString("title").trim()
                            val streamUrl = item.optString("full_song_url").trim()
                                .ifBlank { item.optString("stream_url").trim() }
                                .ifBlank { item.optString("public_url").trim() }
                            if (key.isBlank() || title.isBlank() || streamUrl.isBlank()) {
                                continue
                            }
                            val bpm = item.optString("bpm").trim()
                            add(
                                CatalogTrack(
                                    key = key,
                                    title = title,
                                    bpm = bpm,
                                    genre = item.optString("genre").trim(),
                                    durationLabel = item.optString("duration_label").trim(),
                                    streamUrl = streamUrl,
                                    artworkUrl = item.optString("artwork_url").trim(),
                                    analysisMetrics = parseTrackAnalysisMetrics(item, bpm)
                                )
                            )
                        }
                    }
                }
            }
        } catch (_error: Exception) {
            emptyList()
        }
    }

    fun listActions(): List<RegisteredAction> {
        return try {
            val request = Request.Builder()
                .url(endpoint("/api/v1/registry/actions"))
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = JSONObject(raw.ifEmpty { "{}" })
                val items = payload.optJSONArray("actions")
                if (!response.isSuccessful || items == null) {
                    emptyList()
                } else {
                    buildList {
                        for (index in 0 until items.length()) {
                            val item = items.optJSONObject(index) ?: continue
                            val id = item.optString("id").trim()
                            val description = item.optString("description").trim()
                            if (id.isNotEmpty()) {
                                add(RegisteredAction(id, description))
                            }
                        }
                    }
                }
            }
        } catch (_error: Exception) {
            emptyList()
        }
    }

    fun runCommand(adminSessionToken: String, command: String): String {
        if (adminSessionToken.startsWith("local-admin-")) {
            return runLocalShellCommand(command)
        }

        return try {
            val body = JSONObject().put("command_line", command).toString()
            val request = Request.Builder()
                .url(endpoint("/api/v1/command/run"))
                .addHeader("Content-Type", "application/json")
                .apply {
                    if (adminSessionToken.isNotBlank()) {
                        addHeader("Authorization", "Bearer $adminSessionToken")
                    }
                }
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                if (payload != null) {
                    val ok = payload.optBoolean("ok", false)
                    val exitCode = payload.optInt("exit_code", -1)
                    val stdout = payload.optString("stdout", "")
                    val stderr = payload.optString("stderr", "")
                    val error = payload.optString("error", "")
                    val text = buildString {
                        if (ok) {
                            append("exit_code=").append(exitCode)
                        } else {
                            append("command failed")
                        }
                        if (stdout.isNotBlank()) {
                            append("\n\nstdout:\n").append(stdout.trimEnd())
                        }
                        if (stderr.isNotBlank()) {
                            append("\n\nstderr:\n").append(stderr.trimEnd())
                        }
                        if (error.isNotBlank()) {
                            append("\n\nerror:\n").append(error.trimEnd())
                        }
                    }.trim()
                    if (text.isNotBlank()) {
                        return@use text
                    }
                }
                raw.ifEmpty { "No output" }
            }
        } catch (error: Exception) {
            "command network error: ${error.message ?: "unknown error"}"
        }
    }

    fun adminLogin(username: String, password: String): AdminLoginResult {
        val local = localAdminLogin(username, password)
        return try {
            val body = JSONObject()
                .put("username", username)
                .put("password", password)
                .toString()

            val request = Request.Builder()
                .url(endpoint("/api/v1/admin/login"))
                .addHeader("Content-Type", "application/json")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("Authorization", "Bearer $token")
                    }
                }
                .post(body.toRequestBody("application/json".toMediaType()))
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                val payload = runCatching { JSONObject(raw) }.getOrNull()
                if (response.isSuccessful && payload?.optBoolean("ok") == true) {
                    return@use AdminLoginResult(
                        ok = true,
                        username = payload.optString("username", username),
                        sessionToken = payload.optString("session_token", ""),
                        message = "admin online"
                    )
                }

                AdminLoginResult(
                    ok = local.ok,
                    username = local.username,
                    sessionToken = local.sessionToken,
                    message = if (local.ok) {
                        "admin offline; website control requires online login"
                    } else {
                        payload?.optString("error", "login failed") ?: "login failed"
                    }
                )
            }
        } catch (error: Exception) {
            if (local.ok) {
                local.copy(message = "admin offline; backend unreachable: ${error.message ?: "unknown error"}")
            } else {
                AdminLoginResult(
                    ok = false,
                    username = "",
                    sessionToken = "",
                    message = "login network error: ${error.message ?: "unknown error"}"
                )
            }
        }
    }

    private fun runLocalShellCommand(command: String): String {
        return try {
            val process = ProcessBuilder("sh", "-c", command)
                .redirectErrorStream(false)
                .start()
            val finished = process.waitFor(12, TimeUnit.SECONDS)
            if (!finished) {
                process.destroyForcibly()
                return "local shell timeout after 12 seconds"
            }
            val stdout = process.inputStream.bufferedReader().readText().trimEnd()
            val stderr = process.errorStream.bufferedReader().readText().trimEnd()
            buildString {
                append("local exit_code=").append(process.exitValue())
                if (stdout.isNotBlank()) append("\n\nstdout:\n").append(stdout)
                if (stderr.isNotBlank()) append("\n\nstderr:\n").append(stderr)
                if (stdout.isBlank() && stderr.isBlank()) append("\n\nno output")
            }
        } catch (error: Exception) {
            "local shell error: ${error.message ?: "unknown error"}"
        }
    }

    private fun renderApiResult(response: okhttp3.Response, raw: String): String {
        val payload = runCatching { JSONObject(raw) }.getOrNull()
        return if (payload != null) {
            payload.toString(2)
        } else {
            if (raw.isNotBlank()) raw else "request complete (${response.code})"
        }
    }

    private fun adminJsonPost(adminSessionToken: String, route: String, body: JSONObject): Pair<Boolean, String> {
        return try {
            val request = Request.Builder()
                .url(endpoint(route))
                .addHeader("Content-Type", "application/json")
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .post(body.toString().toRequestBody("application/json".toMediaType()))
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                Pair(response.isSuccessful, renderApiResult(response, raw))
            }
        } catch (error: Exception) {
            Pair(false, "admin request error: ${error.message ?: "unknown error"}")
        }
    }

    private fun adminGet(adminSessionToken: String, route: String): Pair<Boolean, String> {
        return try {
            val request = Request.Builder()
                .url(endpoint(route))
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .get()
                .build()
            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                Pair(response.isSuccessful, renderApiResult(response, raw))
            }
        } catch (error: Exception) {
            Pair(false, "admin request error: ${error.message ?: "unknown error"}")
        }
    }

    fun adminReadFile(adminSessionToken: String, relPath: String): AdminFileReadResult {
        val (ok, rendered) = adminJsonPost(
            adminSessionToken,
            "/api/v1/admin/files/read",
            JSONObject().put("path", relPath)
        )
        return if (!ok) {
            AdminFileReadResult(ok = false, content = "", message = rendered)
        } else {
            val payload = runCatching { JSONObject(rendered) }.getOrNull()
            if (payload != null && payload.optBoolean("ok")) {
                AdminFileReadResult(ok = true, content = payload.optString("content", ""), message = "file loaded")
            } else {
                AdminFileReadResult(ok = false, content = "", message = rendered)
            }
        }
    }

    fun adminWriteFile(adminSessionToken: String, relPath: String, content: String): String {
        val (_ok, rendered) = adminJsonPost(
            adminSessionToken,
            "/api/v1/admin/files/write",
            JSONObject().put("path", relPath).put("content", content).put("deploy", true)
        )
        return rendered
    }

    fun adminDeletePath(adminSessionToken: String, relPath: String): String {
        val (_ok, rendered) = adminJsonPost(
            adminSessionToken,
            "/api/v1/admin/files/delete",
            JSONObject().put("path", relPath)
        )
        return rendered
    }

    fun adminListFiles(adminSessionToken: String, relPath: String): String {
        val safePath = android.net.Uri.encode(relPath)
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/files/list?path=$safePath")
        return rendered
    }

    fun adminCatalogList(adminSessionToken: String): String {
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/catalog/list")
        return rendered
    }

    fun adminCatalogRemove(adminSessionToken: String, key: String): String {
        val (_ok, rendered) = adminJsonPost(
            adminSessionToken,
            "/api/v1/admin/catalog/remove",
            JSONObject().put("key", key)
        )
        return rendered
    }

    fun adminInquiriesList(adminSessionToken: String): String {
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/inquiries/list")
        return rendered
    }

    fun adminLogsList(adminSessionToken: String): String {
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/logs/list?limit=300")
        return rendered
    }

    fun adminSalesList(adminSessionToken: String): String {
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/sales/list")
        return rendered
    }

    fun adminDashboardState(adminSessionToken: String): String {
        val (_ok, rendered) = adminGet(adminSessionToken, "/api/v1/admin/dashboard/state")
        return rendered
    }

    fun adminRecordSale(
        adminSessionToken: String,
        itemName: String,
        amount: String,
        customerEmail: String
    ): String {
        val body = JSONObject()
            .put("item_name", itemName)
            .put("amount", amount)
            .put("currency", "USD")
            .put("payment_provider", "paypal")
            .put("customer_email", customerEmail)
        val (_ok, rendered) = adminJsonPost(adminSessionToken, "/api/v1/admin/sales/record", body)
        return rendered
    }

    private fun copyUriToTempFile(
        contentResolver: android.content.ContentResolver,
        uri: Uri,
        fallbackStem: String
    ): Pair<File, String>? {
        val uploadName = buildUploadFileName(contentResolver, uri, fallbackStem)
        val suffix = uploadName.substringAfterLast('.', "").trim()
        val tempFile = File.createTempFile(
            fallbackStem,
            if (suffix.isNotBlank()) ".${suffix.lowercase()}" else ".bin"
        )
        val stream = contentResolver.openInputStream(uri) ?: return null
        stream.use { input ->
            FileOutputStream(tempFile).use { output ->
                input.copyTo(output)
            }
        }
        return Pair(tempFile, uploadName)
    }

    fun adminUploadFile(
        contentResolver: android.content.ContentResolver,
        uri: Uri,
        adminSessionToken: String,
        targetPath: String
    ): String {
        return try {
            val copied = copyUriToTempFile(contentResolver, uri, "AIFRED_asset_upload")
                ?: return "cannot open file"
            val (tempFile, uploadName) = copied
            val mime = contentResolver.getType(uri).orEmpty().ifBlank { "application/octet-stream" }
            val fileBody = tempFile.asRequestBody(mime.toMediaType())
            val multipart = MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("path", targetPath)
                .addFormDataPart("file", uploadName, fileBody)
                .build()

            val request = Request.Builder()
                .url(endpoint("/api/v1/admin/files/upload"))
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .post(multipart)
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                tempFile.delete()
                if (response.isSuccessful) {
                    "Uploaded ${targetPath.trim()} from $uploadName"
                } else {
                    "upload failed: ${raw.ifEmpty { "unknown error" }}"
                }
            }
        } catch (error: Exception) {
            "upload network error: ${error.message ?: "unknown error"}"
        }
    }

    fun uploadReferenceTrack(
        contentResolver: android.content.ContentResolver,
        uri: Uri,
        adminSessionToken: String,
        genre: String,
        title: String
    ): String {
        return try {
            val copied = copyUriToTempFile(contentResolver, uri, "AIFRED_reference_upload")
                ?: return "cannot open file"
            val (tempFile, uploadName) = copied
            val mime = contentResolver.getType(uri).orEmpty().ifBlank { "application/octet-stream" }
            val fileBody = tempFile.asRequestBody(mime.toMediaType())
            val multipart = MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("file", uploadName, fileBody)
                .addFormDataPart("genre", genre)
                .addFormDataPart("title", title)
                .build()

            val request = Request.Builder()
                .url(endpoint("/api/v1/admin/reference/upload"))
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .post(multipart)
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                tempFile.delete()
                if (response.isSuccessful) {
                    val payload = runCatching { JSONObject(raw) }.getOrNull()
                    val storedPath = payload?.optString("stored_path").orEmpty()
                    "Uploaded licensed reference to ${storedPath.ifBlank { genre }}"
                } else {
                    "upload failed: ${raw.ifEmpty { "unknown error" }}"
                }
            }
        } catch (error: Exception) {
            "upload network error: ${error.message ?: "unknown error"}"
        }
    }

    fun uploadSoundPack(
        contentResolver: android.content.ContentResolver,
        uri: Uri,
        adminSessionToken: String,
        meta: SoundPackMeta
    ): String {
        return try {
            val copied = copyUriToTempFile(contentResolver, uri, "AIFRED_catalog_upload")
                ?: return "cannot open file"
            val (tempFile, uploadName) = copied
            val mime = contentResolver.getType(uri).orEmpty().ifBlank { "application/octet-stream" }
            val fileBody = tempFile.asRequestBody(mime.toMediaType())
            val multipart = MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("file", uploadName, fileBody)
                .addFormDataPart("pack_type", meta.packType)
                .addFormDataPart("title", meta.title)
                .addFormDataPart("description", meta.description)
                .addFormDataPart("bpm", meta.bpm)
                .addFormDataPart("key", meta.key)
                .addFormDataPart("tempo", meta.tempo)
                .addFormDataPart("price", meta.price)
                .build()

            val request = Request.Builder()
                .url(endpoint("/api/v1/admin/catalog/upload"))
                .addHeader("Authorization", "Bearer $adminSessionToken")
                .apply {
                    if (token.isNotBlank()) {
                        addHeader("X-Api-Token", token)
                    }
                }
                .post(multipart)
                .build()

            client.newCall(request).execute().use { response ->
                val raw = response.body?.string().orEmpty()
                tempFile.delete()
                if (response.isSuccessful) {
                    val payload = runCatching { JSONObject(raw) }.getOrNull()
                    val trackTitle = payload?.optJSONObject("track")?.optString("title")
                        ?.takeIf { it.isNotBlank() }
                        ?: meta.title.ifBlank { "catalog audio" }
                    "Uploaded ${trackTitle} to the configured website catalog route"
                } else {
                    "upload failed: ${raw.ifEmpty { "unknown error" }}"
                }
            }
        } catch (error: Exception) {
            "upload network error: ${error.message ?: "unknown error"}"
        }
    }
}
