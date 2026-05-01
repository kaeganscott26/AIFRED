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
        if (Directory.Exists(targetPlugin))
        {
            Directory.Delete(targetPlugin, recursive: true);
        }

        CopyDirectory(sourcePlugin, targetPlugin);
        CopyDirectory(Path.Combine(extractedRoot, "Aifred"), productRoot);
        EnsureOllamaReady();
        ConfigureAiProvider(productRoot);
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

static void RegisterStartup(string enginePath)
{
    using var key = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run");
    key?.SetValue("AIFRED Engine", $"\"{enginePath}\"");
}

static void ConfigureAiProvider(string productRoot)
{
    var result = MessageBox.Show(
        "AIFRED uses local Ollama by default.\n\nChoose Yes to enter an online/OpenAI-compatible endpoint and API key instead. Choose No to keep the local setup.",
        "AIFRED AI Setup",
        MessageBoxButtons.YesNo,
        MessageBoxIcon.Question);

    if (result != DialogResult.Yes)
    {
        SaveAiSettings("ollama", "http://127.0.0.1:11434", "", "aifred:latest");
        return;
    }

    using var form = new Form
    {
        Text = "AIFRED AI Setup",
        Width = 560,
        Height = 260,
        FormBorderStyle = FormBorderStyle.FixedDialog,
        StartPosition = FormStartPosition.CenterScreen,
        MaximizeBox = false,
        MinimizeBox = false
    };

    var endpointLabel = new Label { Left = 20, Top = 22, Width = 500, Text = "Endpoint (OpenAI or compatible):" };
    var endpoint = new TextBox { Left = 20, Top = 46, Width = 500, Text = "https://api.openai.com/v1" };
    var keyLabel = new Label { Left = 20, Top = 82, Width = 500, Text = "API key:" };
    var apiKey = new TextBox { Left = 20, Top = 106, Width = 500, PasswordChar = '*' };
    var modelLabel = new Label { Left = 20, Top = 142, Width = 500, Text = "Model:" };
    var model = new TextBox { Left = 20, Top = 166, Width = 500, Text = "gpt-5.4-mini" };
    var save = new Button { Text = "Save", Left = 340, Top = 198, Width = 84, DialogResult = DialogResult.OK };
    var cancel = new Button { Text = "Cancel", Left = 436, Top = 198, Width = 84, DialogResult = DialogResult.Cancel };
    form.Controls.AddRange(new Control[] { endpointLabel, endpoint, keyLabel, apiKey, modelLabel, model, save, cancel });
    form.AcceptButton = save;
    form.CancelButton = cancel;

    if (form.ShowDialog() != DialogResult.OK)
    {
        return;
    }

    SaveAiSettings("openai-compatible", endpoint.Text.Trim(), apiKey.Text.Trim(), model.Text.Trim());
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
        if (!OllamaModelExists("aifred:latest"))
        {
            TryRun("ollama", "pull llama3.2:3b", waitMs: 600000);
        }
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
