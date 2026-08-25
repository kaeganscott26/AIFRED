package com.aifred.admin

import org.junit.Assert.assertEquals
import org.junit.Test

class CatalogUrlsTest {
    private val production = "https://www.north3rnlight3r.com"

    @Test
    fun resolvesWebsiteApiPathAgainstProductionBase() {
        assertEquals(
            "$production/api/v1/assets/audio/catalog/Test%20Beat.mp3",
            resolveCatalogAssetUrl(production, "/api/v1/assets/audio/catalog/Test%20Beat.mp3")
        )
    }

    @Test
    fun preservesAbsoluteCatalogUrl() {
        val absolute = "https://cdn.example.test/beats/Test.mp3"
        assertEquals(absolute, resolveCatalogAssetUrl(production, absolute))
    }

    @Test
    fun handlesRelativeAndProtocolRelativePaths() {
        assertEquals("$production/api/v1/catalog.mp3", resolveCatalogAssetUrl("$production/", "api/v1/catalog.mp3"))
        assertEquals("https://cdn.example.test/catalog.mp3", resolveCatalogAssetUrl(production, "//cdn.example.test/catalog.mp3"))
    }
}
