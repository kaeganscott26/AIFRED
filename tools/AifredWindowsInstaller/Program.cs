using System.Diagnostics;
using System.Net.Http.Json;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Forms;

ApplicationConfiguration.Initialize();
using var form = new InstallerForm();
Application.Run(form);

sealed class InstallerForm : Form
{
    const string Model = "aifred:latest";
    const string OllamaEndpoint = "http://127.0.0.1:11434";
    readonly Label status = new() { AutoSize = true, Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
    readonly ProgressBar progress = new() { Dock = DockStyle.Bottom, Style = ProgressBarStyle.Marquee, Height = 14 };

    public InstallerForm()
    {
        Text = "AIFRED Beta Setup";
        ClientSize = new Size(520, 150);
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        var title = new Label { Text = "AIFRED Beta", Font = new Font("Segoe UI", 16, FontStyle.Bold), Dock = DockStyle.Top, Height = 42 };
        var panel = new Panel { Dock = DockStyle.Fill, Padding = new Padding(24, 12, 24, 0) };
        panel.Controls.Add(status);
        Controls.Add(panel);
        Controls.Add(progress);
        Controls.Add(title);
        Shown += async (_, _) => await InstallAsync();
    }

    async Task InstallAsync()
    {
        var scratch = Path.Combine(Path.GetTempPath(), "aifred-beta-install-" + Guid.NewGuid().ToString("N"));
        try
        {
            Directory.CreateDirectory(scratch);
            SetStatus("Preparing the AIFRED plugin and Intelligence Host...");
            var payload = Path.Combine(scratch, "payload");
            using (var input = Assembly.GetExecutingAssembly().GetManifestResourceStream("AIFRED-VST3-windows.zip") ?? throw new IOException("Missing installation payload."))
            using (var archive = new MemoryStream())
            {
                await input.CopyToAsync(archive);
                archive.Position = 0;
                System.IO.Compression.ZipFile.ExtractToDirectory(archive, payload);
            }

            var plugin = Path.Combine(payload, "Aifred.vst3");
            var sharedDsp = Path.Combine(payload, "shared-dsp");
            var host = Path.Combine(payload, "AifredIntelligenceHost");
            if (!File.Exists(Path.Combine(plugin, "Contents", "x86_64-win", "Aifred.vst3")) ||
                !File.Exists(Path.Combine(sharedDsp, "README.md")) ||
                !File.Exists(Path.Combine(host, "AifredIntelligenceHost.exe")))
                throw new IOException("The installer payload is incomplete.");

            InstallOwnership.StopHost();
            InstallOwnership.Install(plugin, InstallOwnership.PluginParent, "Aifred.vst3");
            InstallOwnership.Install(sharedDsp, InstallOwnership.PluginParent, "shared-dsp");
            InstallOwnership.Install(host, InstallOwnership.HostParent, "IntelligenceHost");

            SetStatus("Setting up Ollama and the AIFRED model...");
            var ollama = await EnsureOllamaAsync(scratch);
            await EnsureModelAsync(ollama);
            InstallOwnership.Startup(true);

            SetStatus("Starting the Intelligence Host on port 8787...");
            InstallOwnership.StartHost();
            await WaitForHostAsync();
            progress.Style = ProgressBarStyle.Blocks;
            progress.Value = 100;
            SetStatus("Setup complete. Rescan VST3 plugins in your DAW to load AIFRED.");
            MessageBox.Show(this, "AIFRED Beta is ready. Ollama, aifred:latest, and the Intelligence Host are configured for chat on port 8787. Existing settings were retained.", "AIFRED Beta", MessageBoxButtons.OK, MessageBoxIcon.Information);
            Close();
        }
        catch (Exception error)
        {
            progress.Style = ProgressBarStyle.Blocks;
            SetStatus("Setup could not be completed.");
            MessageBox.Show(this, error.Message, "AIFRED Beta setup failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Environment.ExitCode = 1;
        }
        finally
        {
            InstallOwnership.Recycle(scratch, Path.GetTempPath());
        }
    }

    async Task<string> EnsureOllamaAsync(string scratch)
    {
        var ollama = FindOllama();
        if (ollama is null)
        {
            var installer = Path.Combine(scratch, "OllamaSetup.exe");
            using var client = new HttpClient { Timeout = TimeSpan.FromMinutes(10) };
            var bytes = await client.GetByteArrayAsync("https://ollama.com/download/OllamaSetup.exe");
            await File.WriteAllBytesAsync(installer, bytes);
            using var setup = Process.Start(new ProcessStartInfo(installer) { UseShellExecute = true }) ?? throw new IOException("Could not start the Ollama installer.");
            await setup.WaitForExitAsync();
            if (setup.ExitCode != 0) throw new IOException($"Ollama setup exited with code {setup.ExitCode}.");
            for (var attempt = 0; attempt < 30 && ollama is null; attempt++)
            {
                await Task.Delay(1000);
                ollama = FindOllama();
            }
        }
        if (ollama is null) throw new IOException("Ollama was not found after setup.");
        if (!await IsOllamaReadyAsync())
        {
            var logs = Path.Combine(InstallOwnership.HostParent, "logs");
            Directory.CreateDirectory(logs);
            var process = Process.Start(new ProcessStartInfo(ollama, "serve")
            {
                UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden,
                RedirectStandardOutput = true, RedirectStandardError = true,
                WorkingDirectory = Path.GetDirectoryName(ollama)!
            });
            if (process is null) throw new IOException("Could not start Ollama.");
            _ = process.StandardOutput.ReadToEndAsync().ContinueWith(task => File.WriteAllText(Path.Combine(logs, "ollama.log"), task.Result));
            _ = process.StandardError.ReadToEndAsync().ContinueWith(task => File.WriteAllText(Path.Combine(logs, "ollama-error.log"), task.Result));
        }
        for (var attempt = 0; attempt < 60; attempt++)
        {
            if (await IsOllamaReadyAsync()) return ollama;
            await Task.Delay(1000);
        }
        throw new IOException("Ollama did not become ready on port 11434.");
    }

    async Task EnsureModelAsync(string ollama)
    {
        using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(10) };
        var tags = await client.GetFromJsonAsync<JsonObject>(OllamaEndpoint + "/api/tags") ?? throw new IOException("Ollama returned no model list.");
        var installed = tags["models"]?.AsArray().Any(item => string.Equals(item?["name"]?.GetValue<string>(), Model, StringComparison.OrdinalIgnoreCase)) == true;
        if (installed) return;
        using var pull = Process.Start(new ProcessStartInfo(ollama, $"pull {Model}") { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden }) ?? throw new IOException("Could not start the Ollama model download.");
        await pull.WaitForExitAsync();
        if (pull.ExitCode != 0) throw new IOException("Ollama could not download aifred:latest. Check the network connection and run setup again.");
    }

    async Task WaitForHostAsync()
    {
        using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(2) };
        for (var attempt = 0; attempt < 30; attempt++)
        {
            try
            {
                var health = await client.GetFromJsonAsync<JsonObject>("http://127.0.0.1:8787/health");
                if (health?["host_identity"]?.GetValue<string>() == "AifredIntelligenceHost" && health["product_channel"]?.GetValue<string>() == "beta") return;
            }
            catch (HttpRequestException) { }
            await Task.Delay(500);
        }
        throw new IOException("The AIFRED Intelligence Host did not become ready on port 8787.");
    }

    static string? FindOllama()
    {
        var candidates = new[]
        {
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "Ollama", "ollama.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Ollama", "ollama.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Ollama", "ollama.exe")
        };
        return candidates.FirstOrDefault(File.Exists);
    }

    static async Task<bool> IsOllamaReadyAsync()
    {
        try
        {
            using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(2) };
            using var response = await client.GetAsync(OllamaEndpoint + "/api/tags");
            return response.IsSuccessStatusCode;
        }
        catch (HttpRequestException) { return false; }
        catch (TaskCanceledException) { return false; }
    }

    void SetStatus(string message) => status.Text = message;
}
