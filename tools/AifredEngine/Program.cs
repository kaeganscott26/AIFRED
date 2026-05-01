using System.Net;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

var runtime = AifredRuntime.Create();
await runtime.RunAsync();

sealed class AifredRuntime
{
    const string EngineVersion = "1.0.0";
    const int DefaultPort = 8787;

    readonly string installRoot;
    readonly string appDataRoot;
    readonly string configPath;
    readonly string userSettingsPath;
    readonly string logPath;
    readonly HttpListener listener = new();
    JsonObject config;
    JsonObject userSettings;

    AifredRuntime(string installRoot, string appDataRoot)
    {
        this.installRoot = installRoot;
        this.appDataRoot = appDataRoot;
        configPath = Path.Combine(installRoot, "config", "config.json");
        userSettingsPath = Path.Combine(appDataRoot, "user_settings.json");
        logPath = Path.Combine(installRoot, "logs", "engine.log");
        Directory.CreateDirectory(Path.GetDirectoryName(configPath)!);
        Directory.CreateDirectory(Path.GetDirectoryName(userSettingsPath)!);
        Directory.CreateDirectory(Path.GetDirectoryName(logPath)!);
        config = LoadJson(configPath, DefaultConfig(installRoot));
        userSettings = LoadJson(userSettingsPath, DefaultUserSettings());
    }

    public static AifredRuntime Create()
    {
        var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        var appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        var executableRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, ".."));
        var installRoot = File.Exists(Path.Combine(executableRoot, "config", "config.json"))
            ? executableRoot
            : Path.Combine(programFiles, "Aifred");
        return new AifredRuntime(
            installRoot,
            Path.Combine(appData, "Aifred"));
    }

    public async Task RunAsync()
    {
        var port = GetInt(config, "port", DefaultPort);
        listener.Prefixes.Add($"http://127.0.0.1:{port}/");
        listener.Start();
        Log($"AIFRED engine started on 127.0.0.1:{port}");

        while (listener.IsListening)
        {
            try
            {
                var context = await listener.GetContextAsync();
                _ = Task.Run(() => HandleAsync(context));
            }
            catch (ObjectDisposedException)
            {
                break;
            }
            catch (HttpListenerException)
            {
                break;
            }
            catch (Exception ex)
            {
                Log("listener error: " + ex.Message);
            }
        }
    }

    async Task HandleAsync(HttpListenerContext context)
    {
        try
        {
            var path = context.Request.Url?.AbsolutePath.TrimEnd('/').ToLowerInvariant() ?? "";
            if (context.Request.HttpMethod == "GET" && path == "/health")
            {
                await JsonAsync(context, Health());
            }
            else if (context.Request.HttpMethod == "POST" && path == "/analyze")
            {
                var body = await ReadJsonAsync(context.Request);
                await JsonAsync(context, Analyze(body));
            }
            else if (context.Request.HttpMethod == "POST" && path == "/chat")
            {
                var body = await ReadJsonAsync(context.Request);
                await JsonAsync(context, await ChatAsync(body));
            }
            else if (context.Request.HttpMethod == "GET" && path == "/v1/settings")
            {
                await JsonAsync(context, new JsonObject { ["ok"] = true, ["config"] = Clone(config), ["user_settings"] = Clone(userSettings) });
            }
            else if (context.Request.HttpMethod == "POST" && path == "/v1/settings")
            {
                userSettings = await ReadJsonAsync(context.Request);
                SaveJson(userSettingsPath, userSettings);
                await JsonAsync(context, new JsonObject { ["ok"] = true, ["user_settings"] = Clone(userSettings) });
            }
            else if (context.Request.HttpMethod == "POST" && path == "/v1/restart")
            {
                await JsonAsync(context, new JsonObject { ["ok"] = true, ["message"] = "restart accepted" });
                Log("restart requested");
                Environment.Exit(0);
            }
            else
            {
                await JsonAsync(context, new JsonObject { ["ok"] = false, ["error"] = "unknown route" }, 404);
            }
        }
        catch (Exception ex)
        {
            Log("request error: " + ex);
            await JsonAsync(context, new JsonObject { ["ok"] = false, ["error"] = "engine request failed" }, 500);
        }
    }

    JsonObject Health()
    {
        var modelPath = GetString(config, "model_path", Path.Combine(installRoot, "models", "aifred-assistant-q4.gguf"));
        if (!Path.IsPathRooted(modelPath)) modelPath = Path.Combine(installRoot, modelPath);
        var provider = EffectiveProvider();
        var modelName = EffectiveModelName();
        var ollamaModelAvailable = provider.Contains("ollama", StringComparison.OrdinalIgnoreCase) && OllamaModelExists(modelName);
        var fileModelAvailable = File.Exists(modelPath);
        return new JsonObject
        {
            ["ok"] = true,
            ["engine_version"] = EngineVersion,
            ["model_loaded"] = fileModelAvailable || ollamaModelAvailable,
            ["ollama_model_available"] = ollamaModelAvailable,
            ["provider_mode"] = provider,
            ["chat_model"] = modelName,
            ["chat_endpoint"] = EffectiveEndpoint(),
            ["model_path"] = modelPath
        };
    }

    JsonObject Analyze(JsonObject body)
    {
        var metrics = body["metrics"]?.AsObject() ?? new JsonObject();
        var diagnosis = body["diagnosis"]?.AsObject() ?? new JsonObject();
        var width = GetDouble(metrics, "stereo_width", 0.5);
        var corr = GetDouble(metrics, "correlation", 1.0);
        var lufs = GetDouble(metrics, "lufs_s", GetDouble(metrics, "lufs_i", -90.0));
        var truePeak = Math.Max(GetDouble(metrics, "true_peak_l", -90.0), GetDouble(metrics, "true_peak_r", -90.0));
        var crest = GetDouble(metrics, "crest_factor", 0.0);

        var issues = new JsonArray();
        var actions = new JsonArray();
        var summary = "DSP metrics received.";
        var confidence = 0.72;

        if (width < 0.25 && corr > 0.75)
        {
            issues.Add("Stereo image is too centered");
            confidence = 0.84;
        }
        if (truePeak > 0.0)
        {
            issues.Add("True peak is over 0 dBTP");
            confidence = Math.Max(confidence, 0.9);
        }
        if (crest < 8.0 && lufs > -14.0)
        {
            issues.Add("Crest factor is low for the current loudness");
            confidence = Math.Max(confidence, 0.82);
        }
        if (issues.Count == 0)
        {
            issues.Add("No threshold flag");
        }

        return new JsonObject
        {
            ["summary"] = summary,
            ["issues"] = issues,
            ["actions"] = actions,
            ["confidence"] = confidence,
            ["dominant"] = GetString(diagnosis, "dominant", "Core Analysis")
        };
    }

    async Task<JsonObject> ChatAsync(JsonObject body)
    {
        var message = GetString(body, "message", "").Trim();
        var context = body["context"]?.AsObject() ?? new JsonObject();
        if (message.Length == 0)
        {
            return new JsonObject { ["response"] = "" };
        }

        var provider = EffectiveProvider();
        var model = EffectiveModelName();
        var endpoint = EffectiveEndpoint();
        try
        {
            if (provider.Contains("ollama", StringComparison.OrdinalIgnoreCase) || endpoint.Contains("11434", StringComparison.OrdinalIgnoreCase))
            {
                return new JsonObject { ["response"] = await AskOllamaAsync(endpoint, model, message, context) };
            }
            if (provider.Contains("openai", StringComparison.OrdinalIgnoreCase) || provider.Contains("compatible", StringComparison.OrdinalIgnoreCase))
            {
                return new JsonObject { ["response"] = await AskOpenAiCompatibleAsync(endpoint, model, message, context) };
            }
        }
        catch (Exception ex)
        {
            Log("chat provider error: " + ex.Message);
        }

        return new JsonObject
        {
            ["response"] = "Local model response unavailable."
        };
    }

    async Task<string> AskOllamaAsync(string endpoint, string model, string message, JsonObject context)
    {
        endpoint = endpoint.TrimEnd('/');
        using var client = new HttpClient { Timeout = TimeSpan.FromMilliseconds(GetInt(config, "timeout_ms", 30000)) };
        var body = new JsonObject
        {
            ["model"] = model,
            ["stream"] = false,
            ["prompt"] = BuildMixPrompt(message, context),
            ["options"] = new JsonObject
            {
                ["num_predict"] = 140,
                ["temperature"] = 0.35
            }
        };
        var response = await client.PostAsync($"{endpoint}/api/generate", new StringContent(body.ToJsonString(), Encoding.UTF8, "application/json"));
        response.EnsureSuccessStatusCode();
        var json = JsonNode.Parse(await response.Content.ReadAsStringAsync())?.AsObject() ?? new JsonObject();
        return CleanAiText(GetString(json, "response", ""));
    }

    async Task<string> AskOpenAiCompatibleAsync(string endpoint, string model, string message, JsonObject context)
    {
        endpoint = endpoint.TrimEnd('/');
        using var client = new HttpClient { Timeout = TimeSpan.FromMilliseconds(GetInt(config, "timeout_ms", 30000)) };
        var apiKey = EffectiveApiKey();
        if (!string.IsNullOrWhiteSpace(apiKey)) client.DefaultRequestHeaders.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", apiKey);
        var body = new JsonObject
        {
            ["model"] = model,
            ["messages"] = new JsonArray
            {
                new JsonObject { ["role"] = "system", ["content"] = SystemPrompt() },
                new JsonObject { ["role"] = "user", ["content"] = BuildMixPrompt(message, context) }
            },
            ["temperature"] = 0.35
        };
        var response = await client.PostAsync($"{endpoint}/chat/completions", new StringContent(body.ToJsonString(), Encoding.UTF8, "application/json"));
        response.EnsureSuccessStatusCode();
        var json = JsonNode.Parse(await response.Content.ReadAsStringAsync())?.AsObject() ?? new JsonObject();
        var content = json["choices"]?[0]?["message"]?["content"]?.GetValue<string>() ?? "";
        return CleanAiText(content);
    }

    string BuildMixPrompt(string message, JsonObject context)
    {
        return $"""
        {SystemPrompt()}
        Snapshot: {context.ToJsonString(new JsonSerializerOptions { WriteIndented = false })}
        Interpretation notes: {BuildInterpretationNotes(context)}
        Question: {message}
        """;
    }

    static string BuildInterpretationNotes(JsonObject context)
    {
        var notes = new List<string>();
        var metrics = context["metrics"]?.AsObject() ?? context["current"]?.AsObject();
        var reference = context["reference_pool"]?.AsObject();
        var snapshot = context["analysis_snapshot"]?.AsObject();
        if (snapshot != null)
        {
            var hasSignal = GetBool(snapshot, "has_signal", false);
            var hasReference = GetBool(snapshot, "has_reference", false);
            var valid = GetBool(snapshot, "values_valid", false);
            if (!hasSignal) notes.Add("AIFRED currently marks the signal as waiting/no usable input, so do not present scores as perfect.");
            if (!hasReference) notes.Add("No reference target is active, so reference-match scores should be treated as unavailable instead of perfect.");
            if (!valid) notes.Add("The current analysis snapshot is invalid or stale; be explicit about uncertainty.");
            if (TryGetDouble(snapshot, "width_score01", out var widthScore) && TryGetDouble(snapshot, "displayed_width_percent", out var widthShown))
                notes.Add($"Canonical width score is {widthScore * 100.0:0}% and the UI displays {widthShown:0}%.");
            if (TryGetDouble(snapshot, "punch_score01", out var punchScore) && TryGetDouble(snapshot, "displayed_punch_percent", out var punchShown))
                notes.Add($"Canonical punch score is {punchScore * 100.0:0}% and the UI displays {punchShown:0}%.");
            if (TryGetDouble(snapshot, "loudness_score01", out var loudScore) && TryGetDouble(snapshot, "displayed_loudness_percent", out var loudShown))
                notes.Add($"Canonical loudness score is {loudScore * 100.0:0}% and the UI displays {loudShown:0}%.");
        }
        if (TryGetDouble(metrics, "integrated_lufs", out var lufs) && TryGetDouble(reference, "target_loudness_lufs", out var targetLufs))
        {
            notes.Add($"{lufs:0.0} LUFS is {(lufs > targetLufs ? "louder/hotter" : "quieter/softer")} than the {targetLufs:0.0} LUFS reference target.");
        }
        if (TryGetDouble(metrics, "true_peak_dbtp", out var truePeak))
        {
            notes.Add(truePeak > 0.0 ? $"True peak is {truePeak:0.0} dBTP, so there is clipping or insufficient ceiling." : $"True peak is {truePeak:0.0} dBTP.");
        }
        if (TryGetDouble(metrics, "crest_db", out var crest) && TryGetDouble(reference, "target_crest_db", out var targetCrest))
        {
            notes.Add($"{crest:0.0} dB crest is {(crest < targetCrest ? "less dynamic/more compressed" : "more dynamic/more open")} than the {targetCrest:0.0} dB reference crest.");
        }
        return notes.Count == 0 ? "Use the supplied snapshot when it is relevant." : string.Join(" ", notes);
    }

    static bool TryGetDouble(JsonObject? obj, string key, out double value)
    {
        value = 0;
        if (obj == null || obj[key] == null) return false;
        try
        {
            value = obj[key]!.GetValue<double>();
            return true;
        }
        catch
        {
            return double.TryParse(obj[key]!.ToString(), out value);
        }
    }

    static bool OllamaModelExists(string modelName)
    {
        try
        {
            using var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
            {
                FileName = "ollama",
                Arguments = "list",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                CreateNoWindow = true
            });
            if (process == null) return false;
            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit(5000);
            return output.Contains(modelName, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    static string SystemPrompt() =>
        "You are AIFRED, a natural mix-aware assistant inside an audio plugin. Reply conversationally to any user message. When the DAW audio snapshot or reference-pool metadata is relevant, use it to give practical mix advice with clear units such as dB, LUFS, dBTP, frequency range, crest, and correlation. Treat the interpretation notes as measured facts and do not contradict them. Remember that -9.2 LUFS is louder/hotter than -11.5 LUFS, and a 6.1 dB crest is less dynamic than a 10.4 dB crest by 4.3 dB. Keep the answer human, direct, and complete in 2 to 4 concise sentences. Do not print JSON, code fences, raw objects, tables, schema text, or implementation details.";

    static string CleanAiText(string value)
    {
        value = value.Trim();
        if (value.StartsWith("{") || value.StartsWith("["))
        {
            try
            {
                var parsed = JsonNode.Parse(value);
                var candidate = parsed?["response"]?.GetValue<string>()
                    ?? parsed?["message"]?.GetValue<string>()
                    ?? parsed?["answer"]?.GetValue<string>()
                    ?? parsed?["content"]?.GetValue<string>()
                    ?? "";
                if (!string.IsNullOrWhiteSpace(candidate)) return CleanAiText(candidate);
            }
            catch {}
            return "I received structured model data instead of a normal spoken answer. Ask again and I will answer naturally from the current mix context.";
        }
        value = value.Replace("```json", "", StringComparison.OrdinalIgnoreCase)
                     .Replace("```", "")
                     .Replace("\\n", "\n")
                     .Replace("\\\"", "\"")
                     .Replace("\\u2019", "'").Replace("u2019", "'")
                     .Replace("\\u2018", "'").Replace("u2018", "'")
                     .Replace("\\u201c", "\"").Replace("u201c", "\"")
                     .Replace("\\u201d", "\"").Replace("u201d", "\"")
                     .Replace("\\u2013", "-").Replace("u2013", "-")
                     .Replace("\\u2014", "-").Replace("u2014", "-")
                     .Replace("NaN", "unavailable", StringComparison.OrdinalIgnoreCase)
                     .Replace("Infinity", "unavailable", StringComparison.OrdinalIgnoreCase)
                     .Replace("undefined", "unavailable", StringComparison.OrdinalIgnoreCase)
                     .Trim();
        return value.Length == 0 ? "AIFRED did not receive a usable model response." : value;
    }

    string EffectiveProvider()
    {
        var overrideEnabled = GetBool(userSettings, "provider_override_enabled", false);
        return overrideEnabled ? GetString(userSettings, "provider_mode", "ollama") : GetString(config, "provider", "ollama");
    }

    string EffectiveEndpoint()
    {
        var overrideEnabled = GetBool(userSettings, "provider_override_enabled", false);
        var endpoint = overrideEnabled ? GetString(userSettings, "custom_endpoint", "") : GetString(config, "custom_endpoint", "");
        return string.IsNullOrWhiteSpace(endpoint) ? "http://127.0.0.1:11434" : endpoint;
    }

    string EffectiveModelName()
    {
        var overrideEnabled = GetBool(userSettings, "provider_override_enabled", false);
        var model = overrideEnabled ? GetString(userSettings, "model_name", "") : GetString(config, "model_name", "");
        return string.IsNullOrWhiteSpace(model) ? "aifred:latest" : model;
    }

    string EffectiveApiKey()
    {
        var overrideEnabled = GetBool(userSettings, "provider_override_enabled", false);
        return overrideEnabled ? GetString(userSettings, "api_key", "") : GetString(config, "openai_api_key", "");
    }

    void Log(string line)
    {
        File.AppendAllText(logPath, $"[{DateTimeOffset.Now:O}] {line}{Environment.NewLine}");
    }

    static JsonObject DefaultConfig(string root) => new()
    {
        ["mode"] = "local",
        ["port"] = DefaultPort,
        ["provider"] = "ollama",
        ["model_path"] = "models/aifred-assistant-q4.gguf",
        ["model_name"] = "aifred:latest",
        ["openai_api_key"] = "",
        ["custom_endpoint"] = "http://127.0.0.1:11434",
        ["timeout_ms"] = 180000
    };

    static JsonObject DefaultUserSettings() => new()
    {
        ["provider_override_enabled"] = false,
        ["provider_mode"] = "ollama",
        ["api_key"] = "",
        ["custom_endpoint"] = "http://127.0.0.1:11434",
        ["model_name"] = "aifred:latest"
    };

    static JsonObject LoadJson(string path, JsonObject fallback)
    {
        if (!File.Exists(path))
        {
            SaveJson(path, fallback);
            return fallback;
        }
        try
        {
            return JsonNode.Parse(File.ReadAllText(path))?.AsObject() ?? fallback;
        }
        catch (JsonException)
        {
            var backupPath = $"{path}.invalid-{DateTimeOffset.UtcNow:yyyyMMddHHmmss}";
            File.Move(path, backupPath, true);
            SaveJson(path, fallback);
            return fallback;
        }
    }

    static void SaveJson(string path, JsonObject value)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllText(path, value.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
    }

    static JsonObject Clone(JsonObject source) => JsonNode.Parse(source.ToJsonString())!.AsObject();
    static async Task<JsonObject> ReadJsonAsync(HttpListenerRequest request)
    {
        using var reader = new StreamReader(request.InputStream, request.ContentEncoding);
        var text = await reader.ReadToEndAsync();
        return string.IsNullOrWhiteSpace(text) ? new JsonObject() : JsonNode.Parse(text)?.AsObject() ?? new JsonObject();
    }

    static async Task JsonAsync(HttpListenerContext context, JsonObject body, int status = 200)
    {
        var bytes = Encoding.UTF8.GetBytes(body.ToJsonString());
        context.Response.StatusCode = status;
        context.Response.ContentType = "application/json; charset=utf-8";
        context.Response.ContentLength64 = bytes.Length;
        context.Response.Headers["Cache-Control"] = "no-store";
        await context.Response.OutputStream.WriteAsync(bytes);
        context.Response.Close();
    }

    static string GetString(JsonObject obj, string key, string fallback) => obj[key]?.GetValue<string>() ?? fallback;
    static bool GetBool(JsonObject obj, string key, bool fallback) => obj[key]?.GetValue<bool>() ?? fallback;
    static int GetInt(JsonObject obj, string key, int fallback) => obj[key]?.GetValue<int>() ?? fallback;
    static double GetDouble(JsonObject obj, string key, double fallback) => obj[key]?.GetValue<double>() ?? fallback;
}
