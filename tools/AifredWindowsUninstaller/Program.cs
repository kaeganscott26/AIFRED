using System.Windows.Forms;

const string PluginFolderName = "Aifred.vst3";

var targetPlugin = Path.Combine(
    Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles),
    "VST3",
    PluginFolderName);
var productRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Aifred");
var settingsRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Aifred");

try
{
    var confirm = MessageBox.Show(
        "Remove AIFRED VST3, the local AIFRED backend, startup entry, and saved backend settings from this computer?",
        "AIFRED Uninstall",
        MessageBoxButtons.YesNo,
        MessageBoxIcon.Question);
    if (confirm != DialogResult.Yes) return 0;

    StopExistingEngine();
    RemoveStartupEntry();
    DeleteDirectory(targetPlugin);
    DeleteDirectory(productRoot);
    DeleteDirectory(settingsRoot);

    MessageBox.Show(
        "AIFRED was removed from this computer.",
        "AIFRED Uninstall",
        MessageBoxButtons.OK,
        MessageBoxIcon.Information);
    return 0;
}
catch (UnauthorizedAccessException)
{
    MessageBox.Show(
        "Windows blocked removal from a protected folder. Run the uninstaller as Administrator.",
        "AIFRED Uninstall",
        MessageBoxButtons.OK,
        MessageBoxIcon.Error);
    return 1;
}
catch (Exception ex)
{
    MessageBox.Show($"Uninstall failed:\n\n{ex.Message}", "AIFRED Uninstall", MessageBoxButtons.OK, MessageBoxIcon.Error);
    return 1;
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

static void RemoveStartupEntry()
{
    try
    {
        using var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", writable: true);
        key?.DeleteValue("AIFRED Engine", throwOnMissingValue: false);
    }
    catch {}
}

static void DeleteDirectory(string path)
{
    if (Directory.Exists(path))
    {
        Directory.Delete(path, recursive: true);
    }
}
