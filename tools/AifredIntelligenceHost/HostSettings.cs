using System.Text.Json;
using System.Text.Json.Nodes;

namespace Aifred.Intelligence;

public sealed record HostSettings(
    string Provider,
    string Endpoint,
    string Model,
    string ApiKey,
    int TimeoutMs,
    int Port)
{
    public static HostSettings Defaults => new(
        "ollama", "http://127.0.0.1:11434", "aifred:latest", "", 180_000, 8787);

    public static HostSettings FromJson(JsonObject json, HostSettings? fallback = null)
    {
        var current = fallback ?? Defaults;
        var provider = Text(json, "provider", Text(json, "provider_mode", current.Provider))
            .Trim().ToLowerInvariant();
        var endpoint = Text(json, "endpoint", Text(json, "custom_endpoint",
            Text(json, "ollama_url", current.Endpoint))).TrimEnd('/');
        var model = Text(json, "model", Text(json, "model_name", current.Model)).Trim();
        var apiKey = Text(json, "api_key", current.ApiKey).Trim();
        var timeout = Integer(json, "timeout_ms", current.TimeoutMs);
        var port = Integer(json, "port", current.Port);

        if (provider.Equals("ollama", StringComparison.OrdinalIgnoreCase) && string.IsNullOrWhiteSpace(endpoint))
            endpoint = Defaults.Endpoint;
        if (string.IsNullOrWhiteSpace(model))
            model = provider.Equals("ollama", StringComparison.OrdinalIgnoreCase)
                ? Defaults.Model : current.Model;
        if (provider is not ("ollama" or "openai" or "openai-compatible"))
            throw new InvalidOperationException("provider must be ollama, openai, or openai-compatible");
        if (!Uri.TryCreate(endpoint, UriKind.Absolute, out var uri)
            || (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
            throw new InvalidOperationException("provider endpoint must be an absolute HTTP or HTTPS URL");

        return new HostSettings(provider, endpoint, model, apiKey,
            Math.Clamp(timeout, 1_000, 420_000), Math.Clamp(port, 1024, 65_535));
    }

    public HostSettings WithEnvironmentOverrides()
    {
        var provider = Environment.GetEnvironmentVariable("AIFRED_PROVIDER")?.Trim();
        var endpoint = Environment.GetEnvironmentVariable("AIFRED_PROVIDER_ENDPOINT")?.Trim();
        var model = Environment.GetEnvironmentVariable("AIFRED_PROVIDER_MODEL")?.Trim();
        var apiKey = Environment.GetEnvironmentVariable("AIFRED_PROVIDER_API_KEY")?.Trim();
        return this with
        {
            Provider = string.IsNullOrWhiteSpace(provider) ? Provider : provider,
            Endpoint = string.IsNullOrWhiteSpace(endpoint) ? Endpoint : endpoint.TrimEnd('/'),
            Model = string.IsNullOrWhiteSpace(model) ? Model : model,
            ApiKey = string.IsNullOrWhiteSpace(apiKey) ? ApiKey : apiKey
        };
    }

    public JsonObject ToPersistedJson() => new()
    {
        ["provider"] = Provider,
        ["endpoint"] = Endpoint,
        ["model"] = Model,
        ["api_key"] = ApiKey,
        ["timeout_ms"] = TimeoutMs,
        ["port"] = Port
    };

    public JsonObject ToPublicJson() => new()
    {
        ["provider"] = Provider,
        ["endpoint"] = Endpoint,
        ["model"] = Model,
        ["api_key_configured"] = !string.IsNullOrWhiteSpace(ApiKey),
        ["timeout_ms"] = TimeoutMs,
        ["port"] = Port
    };

    static string Text(JsonObject json,string key,string fallback) => json[key] is JsonValue value && value.TryGetValue<string>(out var text)?text:fallback;
    static int Integer(JsonObject json,string key,int fallback) => json[key] is JsonValue value && value.TryGetValue<int>(out var number)?number:fallback;
}

public sealed class SettingsStore
{
    readonly string path;
    readonly string prior;
    readonly string channel;
    readonly object gate=new();
    public SettingsStore(string channel,string? root=null)
    {
        this.channel=channel;
        var appData=root??Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        path=Path.Combine(appData,"Aifred",channel,"IntelligenceHost","settings.json");
        prior=channel=="beta"?Path.Combine(appData,"Aifred","user_settings.json"):Path.Combine(appData,"Aifred","Engine","settings.json");
    }
    public HostSettings Load()
    {
        var defaults=HostSettings.Defaults with {Port=channel=="beta"?8787:8788};
        var source=File.Exists(path)?path:prior;
        if(!File.Exists(source))return defaults;
        var json=JsonNode.Parse(File.ReadAllText(source)) as JsonObject??throw new InvalidOperationException("Invalid settings object.");
        // Read old provider settings once; no old DSP/context implementation is loaded.
        if(!File.Exists(path))
        {
            if(json["openai_api_key"] is JsonValue key && json["api_key"]==null)json["api_key"]=key.DeepClone();
            if(json["provider_mode"] is JsonValue provider && provider.ToString()=="compatible")json["provider_mode"]="openai-compatible";
        }
        var settings=HostSettings.FromJson(json,defaults) with {Port=defaults.Port};
        if(!File.Exists(path))Save(settings);
        return settings;
    }
    public void Save(HostSettings settings)
    {
        lock(gate)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var temporary=path+".pending";
            File.WriteAllText(temporary,settings.ToPersistedJson().ToJsonString(new JsonSerializerOptions {WriteIndented=true}));
            File.Move(temporary,path,true);
        }
    }
}
