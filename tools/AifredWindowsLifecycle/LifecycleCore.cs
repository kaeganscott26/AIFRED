using System.Diagnostics;
using System.IO.Compression;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;
using Microsoft.Win32;

namespace Aifred.WindowsLifecycle;

internal static class LifecycleCore
{
    const string Product = "AIFRED Beta";
    const string Channel = "beta";
    const string StartupName = "AIFRED Beta Intelligence Host";
    const string ReleaseApi = "https://north3rnlight3r.com/api/v1/releases/current?channel=beta";
    const string PayloadResource = "AIFRED.Beta.Payload.zip";
    const string UninstallKeyName = "AIFRED Beta";
    static string? TestRoot => Environment.GetEnvironmentVariable("AIFRED_LIFECYCLE_TEST_ROOT")?.Trim();
    static string CommonFiles => TestRoot is { Length: > 0 } root ? Path.Combine(root, "CommonProgramFiles") : Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles);
    static string LocalData => TestRoot is { Length: > 0 } root ? Path.Combine(root, "LocalAppData") : Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
    static string PluginParent => Path.Combine(CommonFiles, "VST3", Product);
    static string PluginTarget => Path.Combine(PluginParent, "Aifred.vst3");
    internal static string RuntimeRoot => Path.Combine(LocalData, "Programs", Product);
    static string HostTarget => Path.Combine(RuntimeRoot, "IntelligenceHost");
    static string HostExe => Path.Combine(HostTarget, "AifredIntelligenceHost.exe");
    static string LegacyHostTarget => Path.Combine(LocalData, "Aifred", Channel, "IntelligenceHost");
    static string UpdaterExe => Path.Combine(RuntimeRoot, "AIFRED-Beta-Updater.exe");
    static string UninstallerExe => Path.Combine(RuntimeRoot, "AIFRED-Beta-Uninstall.exe");
    static string InstalledManifest => Path.Combine(RuntimeRoot, "install.json");

    internal static bool IsRunningFromInstallRoot()
    {
        var process = Environment.ProcessPath;
        return process != null && IsChild(process, RuntimeRoot);
    }

    internal static string InstallEmbedded(IProgress<string>? progress = null)
    {
        using var input = Assembly.GetExecutingAssembly().GetManifestResourceStream(PayloadResource)
            ?? throw new IOException("The signed installation payload is missing.");
        var scratch = OwnedScratch("install");
        try
        {
            var archive = Path.Combine(scratch, "payload.zip");
            using (var output = File.Create(archive)) input.CopyTo(output);
            return InstallArchive(archive, progress);
        }
        finally { DeleteScratch(scratch); }
    }

    internal static string InstallArchive(string archive, IProgress<string>? progress = null)
    {
        progress?.Report("Verifying the embedded AIFRED payload…");
        var scratch = OwnedScratch("payload");
        try
        {
            ZipFile.ExtractToDirectory(archive, scratch);
            var manifest = ValidatePayload(scratch);
            StopOwnedHosts();
            progress?.Report("Installing the AIFRED Beta VST3…");
            InstallTree(Path.Combine(scratch, "Aifred.vst3"), PluginTarget, PluginParent, "plugin");
            progress?.Report("Installing the self-contained Intelligence Host…");
            InstallTree(Path.Combine(scratch, "IntelligenceHost"), HostTarget, RuntimeRoot, "host");
            Directory.CreateDirectory(RuntimeRoot);
            InstallFile(Path.Combine(scratch, "Lifecycle", "AIFRED-Beta-Updater.exe"), UpdaterExe);
            InstallFile(Path.Combine(scratch, "Lifecycle", "AIFRED-Beta-Uninstall.exe"), UninstallerExe);
            var receipt = new JsonObject {
                ["schema"] = "aifred.windows-install.v1", ["product"] = Product, ["channel"] = Channel,
                ["version"] = manifest["version"]?.GetValue<string>() ?? "unknown",
                ["installed_at"] = DateTimeOffset.UtcNow.ToString("O"), ["source_sha256"] = Sha256(archive)
            };
            File.WriteAllText(InstalledManifest, receipt.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
            RemoveTree(LegacyHostTarget, Path.Combine(LocalData, "Aifred", Channel), allowUnmarked: true);
            RegisterLifecycle(receipt["version"]?.GetValue<string>() ?? "unknown");
            StartHost();
            progress?.Report("AIFRED Beta installation verified.");
            return "AIFRED Beta is installed. Rescan VST3 plugins in your DAW.";
        }
        finally { DeleteScratch(scratch); }
    }

    internal static async Task<string> UpdateFromOfficialApi(IProgress<string>? progress = null)
    {
        progress?.Report("Reading the Official AIFRED Beta release manifest…");
        using var client = new HttpClient { Timeout = TimeSpan.FromMinutes(10) };
        client.DefaultRequestHeaders.UserAgent.ParseAdd("AIFRED-Beta-Updater/1.0");
        var root = JsonNode.Parse(await client.GetStringAsync(ReleaseApi))?.AsObject()
            ?? throw new IOException("The release API returned an invalid response.");
        var release = root["release"]?.AsObject() ?? throw new IOException("No published Beta release is available.");
        var version = release["version"]?.GetValue<string>() ?? throw new IOException("The release version is missing.");
        var setup = release["artifacts"]?["setup"]?.AsObject() ?? throw new IOException("The Setup artifact is missing.");
        var expectedHash = setup["sha256"]?.GetValue<string>()?.ToLowerInvariant() ?? throw new IOException("The Setup hash is missing.");
        var expectedSize = setup["size_bytes"]?.GetValue<long>() ?? throw new IOException("The Setup size is missing.");
        var relative = setup["download_url"]?.GetValue<string>() ?? throw new IOException("The Setup URL is missing.");
        var current = InstalledVersion();
        if (CompareVersions(current, version) >= 0) return $"AIFRED Beta {current} is already current.";
        var download = new Uri(new Uri("https://north3rnlight3r.com"), relative);
        if (!download.Scheme.Equals("https", StringComparison.OrdinalIgnoreCase) || download.Host != "north3rnlight3r.com")
            throw new IOException("The release API returned an untrusted download origin.");
        var scratch = OwnedScratch("update");
        var setupPath = Path.Combine(scratch, "AIFRED-Beta-Setup.exe");
        try
        {
            progress?.Report($"Downloading AIFRED Beta {version}…");
            await using (var input = await client.GetStreamAsync(download))
            await using (var output = File.Create(setupPath)) await input.CopyToAsync(output);
            if (new FileInfo(setupPath).Length != expectedSize || !Sha256(setupPath).Equals(expectedHash, StringComparison.OrdinalIgnoreCase))
                throw new IOException("The downloaded Setup file failed size or SHA-256 verification.");
            progress?.Report("Launching the verified AIFRED Beta Setup…");
            var process = Process.Start(new ProcessStartInfo(setupPath) { UseShellExecute = true })
                ?? throw new IOException("Windows could not launch Setup.");
            process.WaitForExit();
            if (process.ExitCode != 0) throw new IOException($"Setup exited with code {process.ExitCode}.");
            return $"AIFRED Beta {version} is installed and verified.";
        }
        finally { DeleteScratch(scratch); }
    }

    internal static string Uninstall(IProgress<string>? progress = null)
    {
        progress?.Report("Stopping the AIFRED Beta Intelligence Host…");
        StopOwnedHosts();
        UnregisterLifecycle();
        progress?.Report("Removing Beta-owned binaries…");
        RemoveTree(PluginTarget, PluginParent, allowUnmarked: true);
        RemoveTree(LegacyHostTarget, Path.Combine(LocalData, "Aifred", Channel), allowUnmarked: true);
        RemoveTree(RuntimeRoot, Path.Combine(LocalData, "Programs"), allowUnmarked: true);
        return "AIFRED Beta binaries were removed. User settings and AIFRED Official were preserved.";
    }

    internal static void ScheduleTemporarySelfDelete()
    {
        if (Environment.ProcessPath is { } processPath && IsChild(processPath, Path.GetTempPath())) MoveFileEx(processPath, null, 0x4);
    }

    static JsonObject ValidatePayload(string root)
    {
        var path = Path.Combine(root, "payload-manifest.json");
        var manifest = JsonNode.Parse(File.ReadAllText(path))?.AsObject() ?? throw new IOException("Invalid payload manifest.");
        if (manifest["schema"]?.GetValue<string>() != "aifred.windows-payload.v1"
            || manifest["product"]?.GetValue<string>() != Product || manifest["channel"]?.GetValue<string>() != Channel)
            throw new IOException("Payload identity mismatch.");
        var files = manifest["files"]?.AsObject() ?? throw new IOException("Payload file inventory is missing.");
        foreach (var item in files)
        {
            var file = SafeChild(root, item.Key);
            var expected = item.Value?.GetValue<string>() ?? "";
            if (!File.Exists(file) || !Sha256(file).Equals(expected, StringComparison.OrdinalIgnoreCase))
                throw new IOException("Payload verification failed for " + item.Key);
        }
        foreach (var required in new[] {
            "Aifred.vst3/Contents/x86_64-win/Aifred.vst3", "IntelligenceHost/AifredIntelligenceHost.exe",
            "Lifecycle/AIFRED-Beta-Updater.exe", "Lifecycle/AIFRED-Beta-Uninstall.exe"
        }) if (!files.ContainsKey(required)) throw new IOException("Payload component is missing: " + required);
        return manifest;
    }

    static void InstallTree(string source, string target, string parent, string component)
    {
        if (!Directory.Exists(source)) throw new IOException("Missing payload component: " + component);
        CheckTree(target, parent);
        var candidate = target + ".candidate";
        var previous = target + ".previous";
        RemoveTree(candidate, parent, allowUnmarked: true);
        RemoveTree(previous, parent, allowUnmarked: true);
        CopyVerified(source, candidate);
        File.WriteAllText(Path.Combine(candidate, ".aifred-owned.json"), JsonSerializer.Serialize(new { product = Product, channel = Channel, component }));
        if (Directory.Exists(target)) Directory.Move(target, previous);
        try { Directory.Move(candidate, target); }
        catch { if (Directory.Exists(previous)) Directory.Move(previous, target); throw; }
        RemoveTree(previous, parent, allowUnmarked: true);
    }

    static void InstallFile(string source, string target)
    {
        if (!File.Exists(source)) throw new IOException("Missing lifecycle executable: " + Path.GetFileName(source));
        Directory.CreateDirectory(Path.GetDirectoryName(target)!);
        var candidate = target + ".candidate";
        File.Copy(source, candidate, true);
        if (Sha256(source) != Sha256(candidate)) throw new IOException("Lifecycle executable verification failed.");
        File.Move(candidate, target, true);
    }

    static void CopyVerified(string source, string target)
    {
        Directory.CreateDirectory(target);
        foreach (var directory in Directory.EnumerateDirectories(source, "*", SearchOption.AllDirectories))
            Directory.CreateDirectory(Path.Combine(target, Path.GetRelativePath(source, directory)));
        foreach (var file in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
        {
            var destination = Path.Combine(target, Path.GetRelativePath(source, file));
            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            File.Copy(file, destination, true);
            if (Sha256(file) != Sha256(destination)) throw new IOException("Installed file verification failed.");
        }
    }

    static void StopOwnedHosts()
    {
        var owned = new[] { HostExe, Path.Combine(LegacyHostTarget, "AifredIntelligenceHost.exe") };
        foreach (var process in Process.GetProcessesByName("AifredIntelligenceHost"))
        using (process)
        {
            string? path;
            try { path = process.MainModule?.FileName; }
            catch { continue; }
            if (path != null && owned.Any(item => path.Equals(item, StringComparison.OrdinalIgnoreCase)))
            {
                process.Kill();
                if (!process.WaitForExit(10000)) throw new IOException("The owned Intelligence Host did not stop.");
            }
        }
    }

    static void StartHost()
    {
        if (TestRoot is { Length: > 0 } || !File.Exists(HostExe)) return;
        Process.Start(new ProcessStartInfo(HostExe, "--channel beta") { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden });
    }

    static void RegisterLifecycle(string version)
    {
        if (TestRoot is { Length: > 0 }) return;
        using (var run = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run"))
            run.SetValue(StartupName, $"\"{HostExe}\" --channel beta");
        using var key = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\" + UninstallKeyName);
        key.SetValue("DisplayName", Product);
        key.SetValue("DisplayVersion", version);
        key.SetValue("Publisher", "North3rnLight3r");
        key.SetValue("InstallLocation", RuntimeRoot);
        key.SetValue("DisplayIcon", UninstallerExe);
        key.SetValue("UninstallString", $"\"{UninstallerExe}\"");
        key.SetValue("ModifyPath", $"\"{UpdaterExe}\"");
        key.SetValue("URLInfoAbout", "https://north3rnlight3r.com");
        key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
    }

    static void UnregisterLifecycle()
    {
        if (TestRoot is { Length: > 0 }) return;
        using (var run = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run")) run.DeleteValue(StartupName, false);
        Registry.CurrentUser.DeleteSubKeyTree(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\" + UninstallKeyName, false);
    }

    static string InstalledVersion()
    {
        if (File.Exists(InstalledManifest))
            return JsonNode.Parse(File.ReadAllText(InstalledManifest))?["version"]?.GetValue<string>() ?? "0.0.0";
        if (TestRoot is not { Length: > 0 })
            using (var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\" + UninstallKeyName))
                if (key?.GetValue("DisplayVersion") is string value) return value;
        return "0.0.0";
    }

    internal static int CompareVersions(string left, string right)
    {
        static Version Parse(string value)
        {
            var numeric = value.Split('-', '+')[0];
            return Version.TryParse(numeric, out var parsed) ? parsed : new Version(0, 0, 0);
        }
        return Parse(left).CompareTo(Parse(right));
    }

    static void RemoveTree(string path, string parent, bool allowUnmarked)
    {
        CheckTree(path, parent);
        if (!Directory.Exists(path)) return;
        if (!allowUnmarked && !File.Exists(Path.Combine(path, ".aifred-owned.json"))) throw new IOException("Cannot prove directory ownership: " + path);
        if (TestRoot is { Length: > 0 }) Directory.Delete(path, true);
        else Microsoft.VisualBasic.FileIO.FileSystem.DeleteDirectory(path, Microsoft.VisualBasic.FileIO.UIOption.OnlyErrorDialogs,
            Microsoft.VisualBasic.FileIO.RecycleOption.SendToRecycleBin, Microsoft.VisualBasic.FileIO.UICancelOption.ThrowException);
    }

    static void CheckTree(string path, string parent)
    {
        var full = Path.GetFullPath(path); var owner = Path.GetFullPath(parent);
        if (!IsChild(full, owner)) throw new IOException("Lifecycle path escaped its owner.");
        for (var current = new DirectoryInfo(full); current != null; current = current.Parent)
            if (current.Exists && current.Attributes.HasFlag(FileAttributes.ReparsePoint)) throw new IOException("Reparse point in lifecycle path.");
        if (!Directory.Exists(full)) return;
        foreach (var entry in Directory.EnumerateFileSystemEntries(full, "*", SearchOption.AllDirectories))
            if (File.GetAttributes(entry).HasFlag(FileAttributes.ReparsePoint)) throw new IOException("Reparse point in lifecycle tree.");
    }

    static bool IsChild(string path, string parent)
    {
        var full = Path.GetFullPath(path); var owner = Path.GetFullPath(parent).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        return full.StartsWith(owner, StringComparison.OrdinalIgnoreCase);
    }

    static string SafeChild(string root, string relative)
    {
        var path = Path.GetFullPath(Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar)));
        if (!IsChild(path, root)) throw new IOException("Payload path escaped its archive.");
        return path;
    }

    static string OwnedScratch(string purpose)
    {
        var root = Path.Combine(Path.GetTempPath(), "AIFRED-Beta-Lifecycle");
        Directory.CreateDirectory(root);
        var path = Path.Combine(root, purpose + "-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(path);
        File.WriteAllText(Path.Combine(path, ".aifred-owned.json"), "{");
        return path;
    }

    static void DeleteScratch(string path)
    {
        var owner = Path.Combine(Path.GetTempPath(), "AIFRED-Beta-Lifecycle");
        if (Directory.Exists(path) && File.Exists(Path.Combine(path, ".aifred-owned.json")) && IsChild(path, owner)) Directory.Delete(path, true);
    }

    internal static string Sha256(string path)
    {
        using var stream = File.OpenRead(path);
        return Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
    }

    internal static void WriteFailureLog(Exception error)
    {
        try
        {
            var root = Path.Combine(LocalData, "Aifred", Channel, "logs");
            Directory.CreateDirectory(root);
            File.AppendAllText(Path.Combine(root, "lifecycle.log"), $"{DateTimeOffset.UtcNow:O} {error}\n");
        }
        catch { }
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool MoveFileEx(string existing, string? replacement, int flags);
}
