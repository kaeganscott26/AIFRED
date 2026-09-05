using System.Diagnostics;
using System.Security.Cryptography;
using Microsoft.VisualBasic.FileIO;
using Microsoft.Win32;

internal static class InstallOwnership
{
    internal static readonly string PluginParent = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3", "AIFRED Beta");
    internal static readonly string HostParent = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Aifred", "beta");
    internal static string Host => Path.Combine(HostParent, "IntelligenceHost", "AifredIntelligenceHost.exe");
    internal const string StartupName = "AIFRED Beta Intelligence Host";

    internal static void CheckTree(string path, string parent)
    {
        path = Path.GetFullPath(path); parent = Path.GetFullPath(parent);
        if (!path.StartsWith(parent.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new IOException("Install path escaped its owner.");
        for (var ancestor = new DirectoryInfo(path); ancestor != null; ancestor = ancestor.Parent)
            if (ancestor.Exists && ancestor.Attributes.HasFlag(FileAttributes.ReparsePoint)) throw new IOException("Reparse point in install path.");
        if (Directory.Exists(path))
        {
            var pending = new Stack<string>(); pending.Push(path);
            while (pending.TryPop(out var directory))
                foreach (var entry in Directory.EnumerateFileSystemEntries(directory))
                {
                    var attributes = File.GetAttributes(entry);
                    if (attributes.HasFlag(FileAttributes.ReparsePoint)) throw new IOException("Reparse point in install tree.");
                    if (attributes.HasFlag(FileAttributes.Directory)) pending.Push(entry);
                }
        }
    }
    internal static void Recycle(string path, string parent)
    {
        CheckTree(path, parent);
        if (Directory.Exists(path)) FileSystem.DeleteDirectory(path, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin, UICancelOption.ThrowException);
    }
    internal static void StopHost()
    {
        foreach (var process in Process.GetProcessesByName("AifredIntelligenceHost"))
            using (process)
            {
                try
                {
                    if (string.Equals(process.MainModule?.FileName, Host, StringComparison.OrdinalIgnoreCase))
                    { process.Kill(); if (!process.WaitForExit(10000)) throw new IOException("Beta host did not stop."); }
                }
                catch (System.ComponentModel.Win32Exception error) { throw new IOException("Cannot establish host process ownership.", error); }
            }
    }
    internal static void CopyVerified(string source, string target)
    {
        Directory.CreateDirectory(target);
        foreach (var directory in Directory.EnumerateDirectories(source, "*", System.IO.SearchOption.AllDirectories)) Directory.CreateDirectory(Path.Combine(target, Path.GetRelativePath(source, directory)));
        foreach (var file in Directory.EnumerateFiles(source, "*", System.IO.SearchOption.AllDirectories))
        {
            var destination = Path.Combine(target, Path.GetRelativePath(source, file)); File.Copy(file, destination);
            using var input = File.OpenRead(file); using var output = File.OpenRead(destination);
            if (!SHA256.HashData(input).SequenceEqual(SHA256.HashData(output))) throw new IOException("Installed hash mismatch.");
        }
    }
    internal static void Install(string source, string parent, string name)
    {
        var target = Path.Combine(parent, name); var candidate = target + ".candidate"; var previous = target + ".previous";
        CheckTree(target, parent); CheckTree(candidate, parent); CheckTree(previous, parent);
        if (Directory.Exists(candidate) || Directory.Exists(previous)) throw new IOException("Retained installation recovery requires inspection.");
        CopyVerified(source, candidate);
        if (Directory.Exists(target)) Directory.Move(target, previous);
        try { Directory.Move(candidate, target); }
        catch { if (Directory.Exists(previous)) Directory.Move(previous, target); throw; }
        Recycle(previous, parent);
    }
    internal static void Startup(bool enabled)
    {
        using var key = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run");
        if (enabled) key.SetValue(StartupName, $"\"{Host}\" --channel beta");
        else key.DeleteValue(StartupName, false);
    }
}
