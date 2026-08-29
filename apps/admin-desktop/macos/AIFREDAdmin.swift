import Cocoa
import WebKit

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private let web = WKWebView(frame: .zero)
    private let output = NSTextView(frame: .zero)
    private let status = NSTextField(labelWithString: "Local archive ready")
    private let search = NSSearchField(frame: .zero)
    private let repoRoot: URL

    override init() {
        if let configured = ProcessInfo.processInfo.environment["AIFRED_REPO_ROOT"] {
            repoRoot = URL(fileURLWithPath: configured)
        } else if let bundled = Bundle.main.url(forResource: "repo-root", withExtension: "txt"),
                  let configured = try? String(contentsOf: bundled, encoding: .utf8).trimmingCharacters(in: .whitespacesAndNewlines) {
            repoRoot = URL(fileURLWithPath: configured)
        } else {
            repoRoot = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
        }
        super.init()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 1220, height: 790), styleMask: [.titled, .closable, .miniaturizable, .resizable], backing: .buffered, defer: false)
        window.title = "AIFRED Admin Desktop"
        window.center()
        let split = NSSplitView(frame: window.contentView!.bounds)
        split.autoresizingMask = [.width, .height]
        split.isVertical = true
        let sidebar = NSView(frame: NSRect(x: 0, y: 0, width: 290, height: 790))
        let stack = NSStackView(frame: NSRect(x: 16, y: 16, width: 258, height: 758))
        stack.orientation = .vertical; stack.alignment = .leading; stack.spacing = 10
        let title = NSTextField(labelWithString: "AIFRED Archive")
        title.font = .boldSystemFont(ofSize: 22); stack.addArrangedSubview(title)
        let detail = NSTextField(wrappingLabelWithString: "Desktop cold storage. Live administration is rendered by the authenticated production /ops console.")
        detail.textColor = .secondaryLabelColor; stack.addArrangedSubview(detail)
        search.placeholderString = "Bounded search term"; search.widthAnchor.constraint(equalToConstant: 250).isActive = true; stack.addArrangedSubview(search)
        [
            ("Archive Status", ["status"]), ("Archive Eligible FORGE Data", ["rotate"]),
            ("Archive Current Logs", ["archive", "--force"]), ("View Archives", ["list"]),
            ("Verify Archives", ["verify"]), ("Rebuild Archive Index", ["rebuild-index"]),
            ("Search Archives", ["search"]), ("Restore to FORGE Workspace", ["restore"]),
            ("Prune Archive ID", ["prune"])
        ].forEach { label, arguments in
            let button = NSButton(title: label, target: self, action: #selector(runArchive(_:)))
            button.identifier = NSUserInterfaceItemIdentifier(arguments.joined(separator: "\u{1f}"))
            button.bezelStyle = .rounded; button.widthAnchor.constraint(equalToConstant: 250).isActive = true
            stack.addArrangedSubview(button)
        }
        status.textColor = .secondaryLabelColor; stack.addArrangedSubview(status)
        let scroll = NSScrollView(); scroll.hasVerticalScroller = true; scroll.documentView = output; scroll.heightAnchor.constraint(greaterThanOrEqualToConstant: 220).isActive = true; scroll.widthAnchor.constraint(equalToConstant: 250).isActive = true; stack.addArrangedSubview(scroll)
        sidebar.addSubview(stack); split.addArrangedSubview(sidebar); split.addArrangedSubview(web); split.setPosition(290, ofDividerAt: 0)
        window.contentView?.addSubview(split); window.makeKeyAndOrderFront(nil)
        let base = ProcessInfo.processInfo.environment["AIFRED_API_BASE_URL"] ?? "https://www.north3rnlight3r.com"
        web.load(URLRequest(url: URL(string: "\(base)/ops")!, cachePolicy: .reloadIgnoringLocalCacheData))
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc private func runArchive(_ sender: NSButton) {
        var arguments = sender.identifier!.rawValue.components(separatedBy: "\u{1f}")
        if ["search", "restore"].contains(arguments.first) { arguments += ["--query", search.stringValue, "--limit", "100", "--byte-limit", "1048576"] }
        if arguments.first == "prune" {
            let alert = NSAlert(); alert.messageText = "Permanently delete this local archive?"; alert.informativeText = search.stringValue; alert.addButton(withTitle: "Delete"); alert.addButton(withTitle: "Cancel")
            guard !search.stringValue.isEmpty, alert.runModal() == .alertFirstButtonReturn else { return }
            arguments += ["--id", search.stringValue, "--confirm"]
        }
        sender.isEnabled = false; status.stringValue = "Running \(arguments.first ?? "operation")…"
        DispatchQueue.global(qos: .userInitiated).async {
            let process = Process(); process.executableURL = URL(fileURLWithPath: "/usr/bin/env")
            process.arguments = ["node", self.repoRoot.appendingPathComponent("tools/aifred-archive.mjs").path] + arguments
            process.currentDirectoryURL = self.repoRoot
            let pipe = Pipe(); process.standardOutput = pipe; process.standardError = pipe
            do { try process.run(); process.waitUntilExit(); let text = String(data: pipe.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""; DispatchQueue.main.async { self.output.string = text; self.status.stringValue = process.terminationStatus == 0 ? "Completed" : "Failed"; sender.isEnabled = true } }
            catch { DispatchQueue.main.async { self.output.string = "ERROR: \(error.localizedDescription)"; self.status.stringValue = "Failed"; sender.isEnabled = true } }
        }
    }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
