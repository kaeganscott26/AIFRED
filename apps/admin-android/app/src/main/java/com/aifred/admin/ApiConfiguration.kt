package com.aifred.admin

import java.net.URI

data class ApiConfiguration(
    val provider: String = "website",
    val baseUrl: String = "",
    val apiKey: String = "",
    val model: String = "aifred:latest"
)

data class ApiConnectionResult(
    val ok: Boolean,
    val message: String,
    val models: List<String> = emptyList()
)

internal val ApiProviders = listOf("website", "ollama", "openai")

internal fun validateApiEndpoint(value: String): String? {
    val endpoint = value.trim()
    val uri = runCatching { URI(endpoint) }.getOrNull() ?: return "Endpoint must be a valid URL"
    val scheme = uri.scheme?.lowercase() ?: return "Endpoint must use http:// or https://"
    val host = uri.host?.lowercase() ?: return "Endpoint must include a host"
    if (scheme == "https") return null
    if (scheme != "http") return "Endpoint must use http:// or https://"

    val privateHttp = host == "localhost" || host == "127.0.0.1" || host == "::1" ||
        host.startsWith("127.") || host.startsWith("10.") || host.startsWith("192.168.") ||
        host.split('.').takeIf { it.size == 4 }?.getOrNull(0) == "172" &&
        (host.split('.').getOrNull(1)?.toIntOrNull() in 16..31)
    return if (privateHttp) null else "Cleartext HTTP is limited to loopback/private-network API endpoints"
}

internal fun apiProviderDefaults(
    provider: String,
    websiteBaseUrl: String,
    existingApiKey: String = ""
): ApiConfiguration {
    return when (provider.trim().lowercase()) {
        "ollama" -> ApiConfiguration(
            provider = "ollama",
            baseUrl = "http://127.0.0.1:11434",
            model = "aifred:latest"
        )
        "openai" -> ApiConfiguration(
            provider = "openai",
            baseUrl = "https://api.openai.com/v1",
            apiKey = existingApiKey,
            model = "gpt-5.6-luna"
        )
        else -> ApiConfiguration(
            provider = "website",
            baseUrl = websiteBaseUrl.trimEnd('/'),
            model = "aifred:latest"
        )
    }
}
