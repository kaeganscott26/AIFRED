package com.aifred.admin

import androidx.test.ext.junit.runners.AndroidJUnit4
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class LocalOllamaNetworkTest {
    @Test
    fun loopbackCleartextSupportsOllamaDiscoveryAndChat() {
        MockWebServer().use { server ->
            server.enqueue(
                MockResponse()
                    .setHeader("Content-Type", "application/json")
                    .setBody("""{"models":[{"name":"aifred:latest"}]}""")
            )
            server.enqueue(
                MockResponse()
                    .setHeader("Content-Type", "application/json")
                    .setBody("""{"message":{"role":"assistant","content":"local ollama ready"}}""")
            )
            server.start()

            val client = ApiClient("http://127.0.0.1:${server.port}", "", "ollama")
            val discovery = client.testApiConnection()
            assertTrue(discovery.message, discovery.ok)
            assertEquals(listOf("aifred:latest"), discovery.models)
            assertEquals("local ollama ready", client.askChat("status", "test", "aifred:latest"))

            assertEquals("/api/tags", server.takeRequest().path)
            assertEquals("/api/chat", server.takeRequest().path)
        }
    }
}
