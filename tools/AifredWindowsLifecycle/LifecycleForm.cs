using System.Drawing.Drawing2D;
using System.Reflection;

namespace Aifred.WindowsLifecycle;

internal enum LifecycleMode { Setup, Update, Uninstall }

internal sealed class BrandPanel : Panel
{
    protected override void OnPaintBackground(PaintEventArgs e)
    {
        using var brush = new LinearGradientBrush(ClientRectangle, Color.FromArgb(5, 15, 25), Color.FromArgb(1, 5, 9), 25f);
        e.Graphics.FillRectangle(brush, ClientRectangle);
        using var grid = new Pen(Color.FromArgb(24, 35, 220, 230));
        for (var x = 0; x < Width; x += 40) e.Graphics.DrawLine(grid, x, 0, x, Height);
        for (var y = 0; y < Height; y += 40) e.Graphics.DrawLine(grid, 0, y, Width, y);
    }
}

internal sealed class LifecycleForm : Form
{
    readonly LifecycleMode mode;
    readonly Label status;
    readonly ProgressBar progress;
    readonly Button action;
    bool finished;

    internal LifecycleForm(LifecycleMode mode)
    {
        this.mode = mode;
        Text = mode switch { LifecycleMode.Setup => "AIFRED Beta Setup", LifecycleMode.Update => "AIFRED Beta Update", _ => "AIFRED Beta Uninstall" };
        ClientSize = new Size(720, 470);
        MinimumSize = MaximumSize = SizeFromClientSize(ClientSize);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(2, 8, 14);
        ForeColor = Color.FromArgb(226, 243, 248);
        Font = new Font("Segoe UI", 10f);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;

        var root = new BrandPanel { Dock = DockStyle.Fill, Padding = new Padding(32) };
        Controls.Add(root);

        var mascot = new PictureBox { Location = new Point(34, 30), Size = new Size(72, 72), SizeMode = PictureBoxSizeMode.Zoom, BackColor = Color.Transparent };
        using (var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("AIFRED.Brand.Mascot.jpg"))
            if (stream != null) mascot.Image = Image.FromStream(stream);
        root.Controls.Add(mascot);

        root.Controls.Add(new Label {
            Text = "AIFRED  BETA", AutoSize = true, Location = new Point(126, 30),
            Font = new Font("Segoe UI Semibold", 25f, FontStyle.Bold), ForeColor = Color.White, BackColor = Color.Transparent
        });
        root.Controls.Add(new Label {
            Text = mode switch { LifecycleMode.Setup => "PROFESSIONAL WINDOWS INSTALLATION", LifecycleMode.Update => "SECURE UPDATE", _ => "CLEAN REMOVAL" },
            AutoSize = true, Location = new Point(130, 72), Font = new Font("Consolas", 10f, FontStyle.Bold),
            ForeColor = Color.FromArgb(105, 255, 105), BackColor = Color.Transparent
        });

        var title = new Label {
            Text = mode switch {
                LifecycleMode.Setup => "Installing the complete AIFRED Beta lifecycle",
                LifecycleMode.Update => "Checking the Official AIFRED release API",
                _ => "Remove AIFRED Beta from this computer"
            },
            AutoSize = false, Location = new Point(34, 132), Size = new Size(650, 38),
            Font = new Font("Segoe UI Semibold", 16f), ForeColor = Color.FromArgb(35, 227, 255), BackColor = Color.Transparent
        };
        root.Controls.Add(title);

        root.Controls.Add(new Label {
            Text = mode == LifecycleMode.Uninstall
                ? "The Beta VST3, Intelligence Host, updater, and lifecycle registration will be removed. Your provider settings and AIFRED Official remain untouched."
                : "Includes the VST3, the shared DSP build, the self-contained Intelligence Host, update support, and a matching uninstaller. No setup scripts or external runtimes are required.",
            AutoSize = false, Location = new Point(36, 180), Size = new Size(640, 62),
            ForeColor = Color.FromArgb(175, 202, 211), BackColor = Color.Transparent
        });

        status = new Label { Text = "Preparing…", AutoSize = false, Location = new Point(36, 267), Size = new Size(640, 48), ForeColor = Color.White, BackColor = Color.Transparent };
        root.Controls.Add(status);
        progress = new ProgressBar { Location = new Point(36, 319), Size = new Size(640, 12), Style = ProgressBarStyle.Marquee, MarqueeAnimationSpeed = 24 };
        root.Controls.Add(progress);
        action = new Button {
            Text = mode == LifecycleMode.Uninstall ? "UNINSTALL AIFRED BETA" : "WORKING…",
            Location = new Point(430, 372), Size = new Size(246, 46), FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(35, 227, 255), ForeColor = Color.FromArgb(1, 10, 15),
            Font = new Font("Segoe UI Semibold", 10f, FontStyle.Bold), Enabled = mode == LifecycleMode.Uninstall
        };
        action.FlatAppearance.BorderColor = Color.FromArgb(140, 255, 69);
        action.FlatAppearance.BorderSize = 2;
        action.Click += async (_, _) => { if (finished) Close(); else await ExecuteAsync(); };
        root.Controls.Add(action);

        Shown += async (_, _) => { if (mode != LifecycleMode.Uninstall) await ExecuteAsync(); };
    }

    async Task ExecuteAsync()
    {
        if (mode == LifecycleMode.Uninstall && MessageBox.Show(this,
                "Remove the AIFRED Beta binaries? User settings and AIFRED Official will be preserved.",
                "AIFRED Beta", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
        action.Enabled = false;
        progress.Style = ProgressBarStyle.Marquee;
        var reporter = new Progress<string>(message => status.Text = message);
        try
        {
            var result = await Task.Run(async () => mode switch {
                LifecycleMode.Setup => LifecycleCore.InstallEmbedded(reporter),
                LifecycleMode.Update => await LifecycleCore.UpdateFromOfficialApi(reporter),
                _ => LifecycleCore.Uninstall(reporter)
            });
            status.Text = result;
            progress.Style = ProgressBarStyle.Continuous;
            progress.Value = 100;
            action.Text = "CLOSE";
            action.Enabled = true;
            finished = true;
        }
        catch (Exception error)
        {
            LifecycleCore.WriteFailureLog(error);
            status.Text = "The operation did not complete. " + error.Message;
            progress.Style = ProgressBarStyle.Continuous;
            progress.Value = 0;
            action.Text = "RETRY";
            action.Enabled = true;
        }
    }
}
