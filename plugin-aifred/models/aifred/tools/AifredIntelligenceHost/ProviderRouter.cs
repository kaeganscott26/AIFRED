using System.Net.Http.Headers;
using System.Text;
using System.Text.Json.Nodes;

namespace Aifred.Intelligence;

public sealed record ProviderStatus(bool Available, string Provider, string Model, string? Error);
public sealed record ProviderReply(bool Success, string Response, string? Error);

public sealed class ProviderRouter
{
    readonly Func<HttpClient> clientFactory;

    public ProviderRouter(Func<HttpClient>? clientFactory = null)
    {
        this.clientFactory = clientFactory ?? (() => new HttpClient());
    }

    public async Task<ProviderStatus> CheckAsync(HostSettings settings, CancellationToken token = default)
    {
        try
        {
            using var client = CreateClient(settings);
            if (IsOllama(settings.Provider))
            {
                using var response = await client.GetAsync(settings.Endpoint + "/api/tags", token);
                if (!response.IsSuccessStatusCode)
                    return new(false, settings.Provider, settings.Model, "Ollama is unavailable.");
                var root = JsonNode.Parse(await response.Content.ReadAsStringAsync(token))?.AsObject();
                var models = root?["models"]?.AsArray();
                var found = models?.Any(item =>
                {
                    var name = item?["name"]?.GetValue<string>() ?? "";
                    return name.Equals(settings.Model, StringComparison.OrdinalIgnoreCase)
                        || name.StartsWith(settings.Model + ":", StringComparison.OrdinalIgnoreCase);
                }) == true;
                return found
                    ? new(true, settings.Provider, settings.Model, null)
                    : new(false, settings.Provider, settings.Model, "Selected Ollama model is not installed.");
            }

            if (string.IsNullOrWhiteSpace(settings.ApiKey))
                return new(false, settings.Provider, settings.Model,
                    "OpenAI-compatible provider has no configured API key.");
            using var modelsResponse = await client.GetAsync(settings.Endpoint + "/models", token);
            return modelsResponse.IsSuccessStatusCode
                ? new(true, settings.Provider, settings.Model, null)
                : new(false, settings.Provider, settings.Model, "OpenAI-compatible provider is unavailable.");
        }
        catch (Exception exception) when (exception is HttpRequestException
                                          or TaskCanceledException
                                          or OperationCanceledException)
        {
            return new(false, settings.Provider, settings.Model, "Provider is unavailable or timed out.");
        }
    }

    public async Task<ProviderReply> ChatAsync(HostSettings settings,
                                                string message,
                                                JsonObject context,
                                                CancellationToken token = default)
    {
        var contractError=ContextContract.Validate(context);
        if(contractError!=null) return new(false,"",contractError);
        if (string.IsNullOrWhiteSpace(message))
            return new(false, "", "A question is required.");

        try
        {
            using var client = CreateClient(settings);
            var userPrompt = IntelligencePrompt.Build(message, context);

            if (IsOllama(settings.Provider))
            {
                var body = new JsonObject
                {
                    ["model"] = settings.Model,
                    ["stream"] = false,
                    ["messages"] = new JsonArray
                    {
                        new JsonObject { ["role"] = "system", ["content"] = IntelligencePrompt.System },
                        new JsonObject { ["role"] = "user", ["content"] = userPrompt }
                    }
                };
                using var response = await client.PostAsync(settings.Endpoint + "/api/chat",
                    JsonContent(body), token);
                if (!response.IsSuccessStatusCode)
                    return new(false, "", "Ollama request failed.");
                var root = JsonNode.Parse(await response.Content.ReadAsStringAsync(token))?.AsObject();
                var text = root?["message"]?["content"]?.GetValue<string>()?.Trim() ?? "";
                return string.IsNullOrWhiteSpace(text)
                    ? new(false, "", "Ollama returned an empty response.")
                    : new(true, text, null);
            }

            if (string.IsNullOrWhiteSpace(settings.ApiKey))
                return new(false, "", "OpenAI-compatible provider has no configured API key.");
            var compatibleBody = new JsonObject
            {
                ["model"] = settings.Model,
                ["messages"] = new JsonArray
                {
                    new JsonObject { ["role"] = "system", ["content"] = IntelligencePrompt.System },
                    new JsonObject { ["role"] = "user", ["content"] = userPrompt }
                }
            };
            using var compatibleRequest = new HttpRequestMessage(
                HttpMethod.Post, settings.Endpoint + "/chat/completions")
            {
                Content = JsonContent(compatibleBody)
            };
            compatibleRequest.Headers.Add("Idempotency-Key", Guid.NewGuid().ToString("N"));
            compatibleRequest.Headers.Add("X-AIFRED-Client",
                ContextContract.Text(context, "plugin_instance_id"));
            compatibleRequest.Headers.Add("X-AIFRED-Product", "aifred");
            compatibleRequest.Headers.Add("X-AIFRED-Channel",
                ContextContract.Text(context, "product_channel"));
            compatibleRequest.Headers.Add("X-AIFRED-Version",
                ContextContract.Text(context, "product_version"));
            compatibleRequest.Headers.Add("X-AIFRED-Purpose", "user-chat");
            using var compatibleResponse = await client.SendAsync(
                compatibleRequest, HttpCompletionOption.ResponseHeadersRead, token);
            if (!compatibleResponse.IsSuccessStatusCode)
                return new(false, "", "OpenAI-compatible request failed.");
            var compatibleRoot = JsonNode.Parse(
                await compatibleResponse.Content.ReadAsStringAsync(token))?.AsObject();
            var compatibleText = compatibleRoot?["choices"]?[0]?["message"]?["content"]
                ?.GetValue<string>()?.Trim() ?? "";
            return string.IsNullOrWhiteSpace(compatibleText)
                ? new(false, "", "OpenAI-compatible provider returned an empty response.")
                : new(true, compatibleText, null);
        }
        catch (Exception exception) when (exception is HttpRequestException
                                          or TaskCanceledException
                                          or OperationCanceledException)
        {
            return new(false, "", "Provider is unavailable or timed out.");
        }
    }

    HttpClient CreateClient(HostSettings settings)
    {
        var client = clientFactory();
        client.Timeout = TimeSpan.FromMilliseconds(settings.TimeoutMs);
        if (!IsOllama(settings.Provider) && !string.IsNullOrWhiteSpace(settings.ApiKey))
            client.DefaultRequestHeaders.Authorization =
                new AuthenticationHeaderValue("Bearer", settings.ApiKey);
        return client;
    }

    static StringContent JsonContent(JsonObject body) =>
        new(body.ToJsonString(), Encoding.UTF8, "application/json");

    static bool IsOllama(string provider) =>
        provider.Equals("ollama", StringComparison.OrdinalIgnoreCase);
}
