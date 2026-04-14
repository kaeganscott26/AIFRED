using System.IO.Compression;
using System.Reflection;

const string ZipResourceName = "AIFRED-VST3-windows.zip";
const string PluginFolderName = "Aifred.vst3";

var useSystemInstall = args.Any(arg => string.Equals(arg, "--system", StringComparison.OrdinalIgnoreCase));
var targetRoot = useSystemInstall
    ? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3")
    : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "Common", "VST3");
var targetPlugin = Path.Combine(targetRoot, PluginFolderName);

Console.WriteLine("AIFRED VST3 Installer");
Console.WriteLine(useSystemInstall ? "Install mode: system-wide" : "Install mode: current user");
Console.WriteLine($"Target: {targetPlugin}");

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
        if (!Directory.Exists(sourcePlugin))
        {
            throw new InvalidOperationException("Installer payload did not contain the VST3 bundle.");
        }

        Directory.CreateDirectory(targetRoot);
        if (Directory.Exists(targetPlugin))
        {
            Directory.Delete(targetPlugin, recursive: true);
        }

        CopyDirectory(sourcePlugin, targetPlugin);

        Console.WriteLine();
        Console.WriteLine("AIFRED installed.");
        Console.WriteLine("Open FL Studio, go to Plugin Manager, add or rescan this folder:");
        Console.WriteLine(targetRoot);
        Console.WriteLine();
        Console.WriteLine("For system-wide install, rerun this installer from an Administrator terminal with: --system");
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
    Console.Error.WriteLine();
    Console.Error.WriteLine("Windows blocked access to the target folder.");
    Console.Error.WriteLine("Run the installer normally for current-user install, or run as Administrator with --system for system-wide install.");
    return 1;
}
catch (Exception ex)
{
    Console.Error.WriteLine();
    Console.Error.WriteLine("Install failed:");
    Console.Error.WriteLine(ex.Message);
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
    return normalized.StartsWith(PluginFolderName + "/", StringComparison.OrdinalIgnoreCase);
}
