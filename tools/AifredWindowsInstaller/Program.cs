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
