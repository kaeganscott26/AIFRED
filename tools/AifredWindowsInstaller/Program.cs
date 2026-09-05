using System.Diagnostics;
using System.IO.Compression;
using System.Reflection;
using System.Windows.Forms;

ApplicationConfiguration.Initialize();
var scratch = Path.Combine(Path.GetTempPath(), "aifred-beta-install-" + Guid.NewGuid().ToString("N"));
try
{
    Directory.CreateDirectory(scratch);
    var archivePath = Path.Combine(scratch, "payload.zip");
    using (var input = Assembly.GetExecutingAssembly().GetManifestResourceStream("AIFRED-VST3-windows.zip") ?? throw new IOException("Missing installation payload."))
    using (var output = File.Create(archivePath)) input.CopyTo(output);
    var payload = Path.Combine(scratch, "payload");
    ZipFile.ExtractToDirectory(archivePath, payload);
    var plugin = Path.Combine(payload, "Aifred.vst3");
    var host = Path.Combine(payload, "AifredIntelligenceHost");
    if (!File.Exists(Path.Combine(plugin, "Contents", "x86_64-win", "Aifred.vst3")) || !File.Exists(Path.Combine(host, "AifredIntelligenceHost.exe"))) throw new IOException("Incomplete payload.");
    InstallOwnership.StopHost();
    InstallOwnership.Install(plugin, InstallOwnership.PluginParent, "Aifred.vst3");
    InstallOwnership.Install(host, InstallOwnership.HostParent, "IntelligenceHost");
    InstallOwnership.Startup(true);
    Process.Start(new ProcessStartInfo(InstallOwnership.Host, "--channel beta") { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden });
    MessageBox.Show("AIFRED Beta installed. Rescan VST3 plugins in your DAW. Intelligence Host uses port 8787 and requires .NET 10. Configure an available model provider separately. Existing settings were retained.", "AIFRED Beta");
}
catch (Exception error)
{
    MessageBox.Show(error.Message, "AIFRED Beta installation failed", MessageBoxButtons.OK, MessageBoxIcon.Error); Environment.ExitCode = 1;
}
finally { InstallOwnership.Recycle(scratch, Path.GetTempPath()); }
