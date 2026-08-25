package com.aifred.admin

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
