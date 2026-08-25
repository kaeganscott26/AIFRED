package com.aifred.admin

internal fun resolveCatalogAssetUrl(baseUrl: String, candidate: String): String {
    val value = candidate.trim()
    if (value.isBlank()) {
        return ""
    }
    if (value.startsWith("https://", ignoreCase = true) || value.startsWith("http://", ignoreCase = true)) {
        return value
    }

    val base = baseUrl.trim().trimEnd('/')
    if (base.isBlank()) {
        return value
    }
    if (value.startsWith("//")) {
        val scheme = if (base.startsWith("http://", ignoreCase = true)) "http:" else "https:"
        return "$scheme$value"
    }
    return "$base/${value.trimStart('/')}"
}
