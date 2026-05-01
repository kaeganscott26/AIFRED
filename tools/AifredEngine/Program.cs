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
                await JsonAsync(context, Chat(body));
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
        return new JsonObject
        {
            ["ok"] = true,
            ["engine_version"] = EngineVersion,
            ["model_loaded"] = File.Exists(modelPath),
            ["provider_mode"] = provider,
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
        var summary = "Core analysis is active. No critical coaching move is needed yet.";
        var confidence = 0.72;

        if (width < 0.25 && corr > 0.75)
        {
            summary = "Your mix is controlled and mono-safe, but the stereo image is tighter than the target.";
            issues.Add("Stereo image is too centered");
            actions.Add("Widen non-bass elements first");
            actions.Add("Keep kick and sub centered");
            confidence = 0.84;
        }
        if (truePeak > 0.0)
        {
            issues.Add("True peak is over 0 dBTP");
            actions.Add("Lower limiter output or reduce clip gain before export");
            confidence = Math.Max(confidence, 0.9);
        }
        if (crest < 8.0 && lufs > -14.0)
        {
            issues.Add("Crest factor suggests over-compression");
            actions.Add("Back off limiting or restore transient contrast");
            confidence = Math.Max(confidence, 0.82);
        }
        if (issues.Count == 0)
        {
            issues.Add("No high-priority issue detected");
            actions.Add("Check the loudest section before final export");
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

    JsonObject Chat(JsonObject body)
    {
        var message = GetString(body, "message", "").Trim();
        var context = body["context"]?.AsObject() ?? new JsonObject();
        var dominant = GetString(context, "dominant_diagnosis", "current diagnosis");
        var response = message.Length == 0
            ? "Ask a mix question after audio has played through AIFRED."
            : $"Based on {dominant}, AIFRED will only explain measured behavior. Core DSP stays authoritative; the assistant is here to translate the diagnosis into next moves.";
        return new JsonObject { ["response"] = response };
    }

    string EffectiveProvider()
    {
        var overrideEnabled = GetBool(userSettings, "provider_override_enabled", false);
        return overrideEnabled ? GetString(userSettings, "provider_mode", "bundled-local") : GetString(config, "provider", "bundled-local");
    }

    void Log(string line)
    {
        File.AppendAllText(logPath, $"[{DateTimeOffset.Now:O}] {line}{Environment.NewLine}");
    }

    static JsonObject DefaultConfig(string root) => new()
    {
        ["mode"] = "local",
        ["port"] = DefaultPort,
        ["provider"] = "bundled-local",
        ["model_path"] = "models/aifred-assistant-q4.gguf",
        ["openai_api_key"] = "",
        ["custom_endpoint"] = "",
        ["timeout_ms"] = 8000
    };

    static JsonObject DefaultUserSettings() => new()
    {
        ["provider_override_enabled"] = false,
        ["provider_mode"] = "bundled-local",
        ["api_key"] = "",
        ["custom_endpoint"] = "",
        ["model_name"] = ""
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
