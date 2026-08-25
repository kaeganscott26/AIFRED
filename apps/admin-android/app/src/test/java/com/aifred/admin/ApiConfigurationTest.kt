package com.aifred.admin

import org.junit.Assert.assertEquals
import org.junit.Test

class ApiConfigurationTest {
    @Test
    fun providerProfilesSelectExpectedEndpointsAndModels() {
        val website = apiProviderDefaults("website", "https://www.north3rnlight3r.com/")
        val ollama = apiProviderDefaults("ollama", website.baseUrl)
        val openai = apiProviderDefaults("openai", website.baseUrl, "private-key")

        assertEquals("https://www.north3rnlight3r.com", website.baseUrl)
        assertEquals("http://127.0.0.1:11434", ollama.baseUrl)
        assertEquals("aifred:latest", ollama.model)
        assertEquals("https://api.openai.com/v1", openai.baseUrl)
        assertEquals("gpt-5.6-luna", openai.model)
        assertEquals("private-key", openai.apiKey)
    }
}
