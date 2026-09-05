using System.Windows.Forms;
ApplicationConfiguration.Initialize();
try
{
    InstallOwnership.StopHost(); InstallOwnership.Startup(false);
    InstallOwnership.Recycle(Path.Combine(InstallOwnership.PluginParent, "Aifred.vst3"), InstallOwnership.PluginParent);
    InstallOwnership.Recycle(Path.Combine(InstallOwnership.HostParent, "IntelligenceHost"), InstallOwnership.HostParent);
    MessageBox.Show("AIFRED Beta binaries removed. User settings and Official were retained.", "AIFRED Beta");
}
catch (Exception error)
{
    MessageBox.Show(error.Message, "AIFRED Beta uninstall failed", MessageBoxButtons.OK, MessageBoxIcon.Error); Environment.ExitCode = 1;
}
