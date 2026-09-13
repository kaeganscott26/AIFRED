using System.IO.Compression;
using System.Security.Cryptography;
using System.Text.Json;
using Aifred.WindowsLifecycle;

var root = Path.Combine(Path.GetTempPath(), "aifred-lifecycle-tests-" + Guid.NewGuid().ToString("N"));
var payload = Path.Combine(root, "payload");
var archive = Path.Combine(root, "payload.zip");
Directory.CreateDirectory(payload);
Environment.SetEnvironmentVariable("AIFRED_LIFECYCLE_TEST_ROOT", Path.Combine(root, "machine"));
try
{
    Write("Aifred.vst3/Contents/x86_64-win/Aifred.vst3", "plugin-v1");
    Write("IntelligenceHost/AifredIntelligenceHost.exe", "host-v1");
    Write("Lifecycle/AIFRED-Beta-Updater.exe", "updater-v1");
    Write("Lifecycle/AIFRED-Beta-Uninstall.exe", "uninstaller-v1");
    var files = Directory.EnumerateFiles(payload, "*", SearchOption.AllDirectories)
        .ToDictionary(path => Path.GetRelativePath(payload, path).Replace('\\', '/'), Hash);
    File.WriteAllText(Path.Combine(payload, "payload-manifest.json"), JsonSerializer.Serialize(new {
        schema = "aifred.windows-payload.v1", product = "AIFRED Beta", channel = "beta", version = "0.4.0", files
    }));
    ZipFile.CreateFromDirectory(payload, archive);
    var installed = LifecycleCore.InstallArchive(archive);
    Check(installed.Contains("installed", StringComparison.OrdinalIgnoreCase), "clean install");
    var machine = Environment.GetEnvironmentVariable("AIFRED_LIFECYCLE_TEST_ROOT")!;
    var plugin = Path.Combine(machine, "CommonProgramFiles", "VST3", "AIFRED Beta", "Aifred.vst3", "Contents", "x86_64-win", "Aifred.vst3");
    Check(File.ReadAllText(plugin) == "plugin-v1", "installed plugin provenance");
    File.WriteAllText(Path.Combine(payload, "Aifred.vst3", "Contents", "x86_64-win", "Aifred.vst3"), "plugin-v2");
    files = Directory.EnumerateFiles(payload, "*", SearchOption.AllDirectories).Where(path => !path.EndsWith("payload-manifest.json"))
        .ToDictionary(path => Path.GetRelativePath(payload, path).Replace('\\', '/'), Hash);
    File.WriteAllText(Path.Combine(payload, "payload-manifest.json"), JsonSerializer.Serialize(new {
        schema = "aifred.windows-payload.v1", product = "AIFRED Beta", channel = "beta", version = "0.4.1", files
    }));
    File.Delete(archive); ZipFile.CreateFromDirectory(payload, archive);
    LifecycleCore.InstallArchive(archive);
    Check(File.ReadAllText(plugin) == "plugin-v2", "clean update replacement");
    Check(LifecycleCore.CompareVersions("0.4.0-beta", "0.4.1") < 0, "semantic update ordering");
    var settings = Path.Combine(machine, "Roaming", "Aifred", "beta", "settings.json");
    Directory.CreateDirectory(Path.GetDirectoryName(settings)!); File.WriteAllText(settings, "preserve");
    LifecycleCore.Uninstall();
    Check(!File.Exists(plugin), "clean uninstall");
    Check(File.Exists(settings), "user settings preserved");
    Console.WriteLine("AIFRED Windows lifecycle tests: PASS");
}
finally
{
    Environment.SetEnvironmentVariable("AIFRED_LIFECYCLE_TEST_ROOT", null);
    if (Directory.Exists(root)) Directory.Delete(root, true);
}

void Write(string relative, string content)
{
    var path = Path.Combine(payload, relative.Replace('/', Path.DirectorySeparatorChar));
    Directory.CreateDirectory(Path.GetDirectoryName(path)!); File.WriteAllText(path, content);
}
string Hash(string path)
{
    using var stream = File.OpenRead(path); return Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
}
void Check(bool ok, string name) { if (!ok) throw new Exception("Lifecycle test failed: " + name); }
