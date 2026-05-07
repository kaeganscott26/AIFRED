using System.IO.Compression;
using System.Reflection;
using System.Net.Http;
using System.Windows.Forms;

const string ZipResourceName = "AIFRED-VST3-windows.zip";
const string PluginFolderName = "Aifred.vst3";
const string EngineExeName = "AifredEngine.exe";

var targetRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3");
var targetPlugin = Path.Combine(targetRoot, PluginFolderName);
var productRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Aifred");
var engineTarget = Path.Combine(productRoot, "bin", EngineExeName);
var aiConfig = ResolveAiConfig(args);

try
{
    using var resource = Assembly.GetExecutingAssembly().GetManifestResourceStream(ZipResourceName)
        ?? throw new InvalidOperationException($"Installer payload is missing: {ZipResourceName}");

    var tempRoot = Path.Combine(Path.GetTempPath(), "AIFRED-VST3-Setup-" + Guid.NewGuid().ToString("N"));
    var extractedRoot = Path.Combine(tempRoot, "payload");
    Directory.CreateDirectory(extractedRoot);

    try
    {
        using var archive = new ZipArchive(resource, ZipArchiveMode.Read);
        foreach (var entry in archive.Entries.Where(IsPluginEntry))
        {
            var destination = Path.GetFullPath(Path.Combine(extractedRoot, entry.FullName.Replace('/', Path.DirectorySeparatorChar)));
            if (!destination.StartsWith(Path.GetFullPath(extractedRoot), StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException("Installer payload contains an unsafe path.");
            }

            if (string.IsNullOrEmpty(entry.Name))
            {
                Directory.CreateDirectory(destination);
                continue;
            }

            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            entry.ExtractToFile(destination, overwrite: true);
        }

        var sourcePlugin = Path.Combine(extractedRoot, PluginFolderName);
        var sourceEngine = Path.Combine(extractedRoot, "Aifred", "bin", EngineExeName);
        if (!Directory.Exists(sourcePlugin))
        {
            throw new InvalidOperationException("Installer payload did not contain the VST3 bundle.");
        }
        if (!File.Exists(sourceEngine))
        {
            throw new InvalidOperationException("Installer payload did not contain AifredEngine.exe.");
        }

        Directory.CreateDirectory(targetRoot);
        Directory.CreateDirectory(productRoot);
        StopExistingEngine();
        if (Directory.Exists(targetPlugin))
        {
            Directory.Delete(targetPlugin, recursive: true);
        }
        if (Directory.Exists(productRoot))
        {
            Directory.Delete(productRoot, recursive: true);
        }

        CopyDirectory(sourcePlugin, targetPlugin);
        CopyDirectory(Path.Combine(extractedRoot, "Aifred"), productRoot);
        ClearPreviousBackendSettings();
        EnsureOllamaReady();
        ConfigureAiProvider(aiConfig);
        RegisterStartup(engineTarget);
        StartEngine(engineTarget);
        var health = await ValidateEngineHealth();

        MessageBox.Show(
            $"AIFRED installed.\n\nVST3:\n{targetPlugin}\n\nEngine:\n{engineTarget}\n\nEngine health: {(health ? "ready" : "starting or unavailable; core plugin analysis still works")}\n\nOpen FL Studio Plugin Manager and rescan VST3 plugins.",
            "AIFRED Setup",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
        return 0;
    }
    finally
    {
        if (Directory.Exists(tempRoot))
        {
            Directory.Delete(tempRoot, recursive: true);
        }
    }
}
catch (UnauthorizedAccessException)
{
    MessageBox.Show(
        "Windows blocked access to the install folder. Run the installer as Administrator for the standard system-wide VST3 install.",
        "AIFRED Setup",
        MessageBoxButtons.OK,
        MessageBoxIcon.Error);
    return 1;
}
catch (Exception ex)
{
    MessageBox.Show($"Install failed:\n\n{ex.Message}", "AIFRED Setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
    return 1;
}

static void CopyDirectory(string source, string destination)
{
    Directory.CreateDirectory(destination);

    foreach (var directory in Directory.EnumerateDirectories(source, "*", SearchOption.AllDirectories))
    {
        Directory.CreateDirectory(directory.Replace(source, destination, StringComparison.OrdinalIgnoreCase));
    }

    foreach (var file in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
    {
        var target = file.Replace(source, destination, StringComparison.OrdinalIgnoreCase);
        Directory.CreateDirectory(Path.GetDirectoryName(target)!);
        File.Copy(file, target, overwrite: true);
    }
}

static bool IsPluginEntry(ZipArchiveEntry entry)
{
    var normalized = entry.FullName.Replace('\\', '/');
    return normalized.StartsWith(PluginFolderName + "/", StringComparison.OrdinalIgnoreCase)
        || normalized.StartsWith("Aifred/", StringComparison.OrdinalIgnoreCase);
}

static void StopExistingEngine()
{
    foreach (var process in System.Diagnostics.Process.GetProcessesByName("AifredEngine"))
    {
        try
        {
            process.Kill(entireProcessTree: true);
            process.WaitForExit(3000);
        }
        catch {}
    }
}

static void ClearPreviousBackendSettings()
{
    var settingsPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Aifred", "user_settings.json");
    try
    {
        if (File.Exists(settingsPath)) File.Delete(settingsPath);
    }
    catch {}
}

static void RegisterStartup(string enginePath)
{
    using var key = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run");
    key?.SetValue("AIFRED Engine", $"\"{enginePath}\"");
}

static InstallerAiConfig ResolveAiConfig(string[] args)
{
    var values = ParseArgs(args);
    string Pick(string key, params string[] envNames)
    {
        if (values.TryGetValue(key, out var argValue) && !string.IsNullOrWhiteSpace(argValue)) return argValue.Trim();
        foreach (var envName in envNames)
        {
            var envValue = Environment.GetEnvironmentVariable(envName);
            if (!string.IsNullOrWhiteSpace(envValue)) return envValue.Trim();
        }
        return "";
    }

    var providerRaw = Pick("ai-provider", "AIFRED_AI_PROVIDER", "AIFRED_CHAT_PROVIDER");
    var apiKey = Pick("ai-key", "AIFRED_OPENAI_API_KEY", "OPENAI_API_KEY", "AIFRED_AI_API_KEY");
    var endpoint = Pick("ai-endpoint", "AIFRED_OPENAI_ENDPOINT", "OPENAI_BASE_URL", "AIFRED_AI_ENDPOINT");
    var model = Pick("ai-model", "AIFRED_OPENAI_MODEL", "OPENAI_MODEL", "AIFRED_OLLAMA_MODEL", "OLLAMA_MODEL");

    var provider = NormalizeProvider(providerRaw, !string.IsNullOrWhiteSpace(apiKey) || !string.IsNullOrWhiteSpace(endpoint));
    if (provider == "ollama")
    {
        endpoint = string.IsNullOrWhiteSpace(endpoint) ? "http://127.0.0.1:11434" : endpoint;
        model = string.IsNullOrWhiteSpace(model) ? "aifred:latest" : model;
        apiKey = "";
    }
    else
    {
        endpoint = string.IsNullOrWhiteSpace(endpoint) ? "https://api.openai.com/v1" : endpoint;
        model = string.IsNullOrWhiteSpace(model) ? "gpt-5.4-mini" : model;
    }

    return new InstallerAiConfig(provider, endpoint, apiKey, model);
}

static string NormalizeProvider(string providerRaw, bool hasOpenAiSignals)
{
    var normalized = providerRaw.Trim().ToLowerInvariant();
    if (normalized is "openai" or "openai-compatible" or "openai_compatible" or "compatible") return "openai-compatible";
    if (normalized is "ollama" or "local") return "ollama";
    return hasOpenAiSignals ? "openai-compatible" : "ollama";
}

static void ConfigureAiProvider(InstallerAiConfig config)
{
    SaveAiSettings(config.Provider, config.Endpoint, config.ApiKey, config.Model);
}

static void SaveAiSettings(string provider, string endpoint, string apiKey, string model)
{
    var settingsPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Aifred", "user_settings.json");
    Directory.CreateDirectory(Path.GetDirectoryName(settingsPath)!);
    File.WriteAllText(settingsPath, $$"""
    {
      "provider_override_enabled": true,
      "provider_mode": "{{JsonEscape(provider)}}",
      "api_key": "{{JsonEscape(apiKey)}}",
      "custom_endpoint": "{{JsonEscape(endpoint)}}",
      "model_name": "{{JsonEscape(model)}}"
    }
    """);
}

static void EnsureOllamaReady()
{
    if (!CommandWorks("ollama", "--version"))
    {
        TryRun("winget", "install --id Ollama.Ollama -e --accept-package-agreements --accept-source-agreements --silent", waitMs: 180000);
    }
    TryRun("ollama", "serve", waitMs: 1500, allowBackground: true);
    if (CommandWorks("ollama", "--version"))
    {
        EnsureAifredOllamaModel();
    }
}

static void EnsureAifredOllamaModel()
{
    if (OllamaModelExists("aifred:latest")) return;

    if (!OllamaModelExists("llama3.2:3b"))
    {
        TryRun("ollama", "pull llama3.2:3b", waitMs: 600000);
    }

    if (!OllamaModelExists("llama3.2:3b")) return;

    var modelFile = Path.Combine(Path.GetTempPath(), "AIFRED-Ollama-Modelfile-" + Guid.NewGuid().ToString("N"));
    try
    {
        File.WriteAllText(modelFile, """
        FROM llama3.2:3b
        SYSTEM You are AIFRED, a natural mix-aware assistant inside an audio plugin. Answer conversationally from the supplied audio analysis snapshot when it is relevant. Do not output JSON, code fences, raw structs, or implementation details.
        PARAMETER temperature 0.35
        """);
        TryRun("ollama", $"create aifred -f \"{modelFile}\"", waitMs: 600000);
    }
    finally
    {
        try
        {
            if (File.Exists(modelFile)) File.Delete(modelFile);
        } catch {}
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

static bool CommandWorks(string fileName, string arguments)
{
    try
    {
        using var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            UseShellExecute = false,
            CreateNoWindow = true
        });
        if (process == null) return false;
        process.WaitForExit(5000);
        return process.ExitCode == 0;
    }
    catch
    {
        return false;
    }
}

static void TryRun(string fileName, string arguments, int waitMs, bool allowBackground = false)
{
    try
    {
        var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            UseShellExecute = false,
            CreateNoWindow = true,
            WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden
        });
        if (process != null && !allowBackground) process.WaitForExit(waitMs);
    }
    catch
    {
        // Setup continues and reports engine health after install.
    }
}

static string JsonEscape(string value)
{
    return value
        .Replace("\\", "\\\\")
        .Replace("\"", "\\\"")
        .Replace("\r", "")
        .Replace("\n", "\\n");
}

static void StartEngine(string enginePath)
{
    try
    {
        System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
        {
            FileName = enginePath,
            UseShellExecute = false,
            CreateNoWindow = true,
            WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden
        });
    }
    catch
    {
        // Installer reports health below. The plugin still works without the engine.
    }
}

static async Task<bool> ValidateEngineHealth()
{
    using var client = new HttpClient { Timeout = TimeSpan.FromMilliseconds(1200) };
    for (var attempt = 0; attempt < 8; attempt++)
    {
        try
        {
            var response = await client.GetAsync("http://127.0.0.1:8787/health");
            if (response.IsSuccessStatusCode) return true;
        }
        catch {}
        await Task.Delay(350);
    }
    return false;
}

static Dictionary<string, string> ParseArgs(string[] args)
{
    var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    for (var i = 1; i < args.Length; i++)
    {
        var current = args[i];
        if (!current.StartsWith("--", StringComparison.Ordinal)) continue;
        var withoutPrefix = current[2..];
        var separator = withoutPrefix.IndexOf('=');
        if (separator >= 0)
        {
            result[withoutPrefix[..separator]] = withoutPrefix[(separator + 1)..];
            continue;
        }
        var next = i + 1 < args.Length ? args[i + 1] : "";
        if (!next.StartsWith("--", StringComparison.Ordinal))
        {
            result[withoutPrefix] = next;
            i += 1;
        }
        else
        {
            result[withoutPrefix] = "true";
        }
    }
    return result;
}

sealed record InstallerAiConfig(string Provider, string Endpoint, string ApiKey, string Model);
