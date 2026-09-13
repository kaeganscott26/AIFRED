using System.Diagnostics;

namespace Aifred.WindowsLifecycle;

internal static class Program
{
    [STAThread]
    static void Main(string[] args)
    {
#if AIFRED_SETUP
        const LifecycleMode mode = LifecycleMode.Setup;
#elif AIFRED_UPDATE
        const LifecycleMode mode = LifecycleMode.Update;
#elif AIFRED_UNINSTALL
        const LifecycleMode mode = LifecycleMode.Uninstall;
#else
#error Build must define one AIFRED lifecycle mode.
#endif
        if ((mode == LifecycleMode.Update || mode == LifecycleMode.Uninstall)
            && !args.Contains("--from-temp", StringComparer.OrdinalIgnoreCase)
            && LifecycleCore.IsRunningFromInstallRoot())
        {
            var temporary = Path.Combine(Path.GetTempPath(), $"AIFRED-Beta-Uninstall-{Guid.NewGuid():N}.exe");
            File.Copy(Environment.ProcessPath!, temporary, true);
            Process.Start(new ProcessStartInfo(temporary, "--from-temp") { UseShellExecute = false });
            return;
        }

        ApplicationConfiguration.Initialize();
        Application.Run(new LifecycleForm(mode));
        if (args.Contains("--from-temp", StringComparer.OrdinalIgnoreCase)) LifecycleCore.ScheduleTemporarySelfDelete();
    }
}
