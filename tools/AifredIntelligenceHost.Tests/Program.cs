using System.Net;
using System.Text;
using System.Text.Json.Nodes;
using Aifred.Intelligence;

var tests = new IntelligenceHostContractTests();
await tests.RunAsync();

sealed class IntelligenceHostContractTests
{
    static JsonObject Envelope(JsonObject context, string channel = "official")
    {
        context["schema"]="aifred.filtered-mix.v1";context["product_channel"]=channel;context["product_version"]=channel=="beta"?"0.3.6-beta-stable":"4.0.0-alpha.2";context["plugin_instance_id"]="instance-1";context["session_id"]="session-1";
        context["profile_id"]="MIX_BALANCED";context["profile_version"]=1;context["observation_id"]="1";
        var metrics=new JsonArray();foreach(var metric in ContextContract.Metrics)metrics.Add(new JsonObject{["metric"]=metric.Name,["unit"]=metric.Unit,["available"]=false});
        var bands=new JsonArray();foreach(var hz in ContextContract.Centres)bands.Add(new JsonObject{["metric"]="band_energy",["unit"]="dBFS",["available"]=false,["centre_hz"]=hz});
        context["metrics"]=metrics;context["bands"]=bands;context["session_context"]=new JsonArray();return context;
    }
    int failures;

    public async Task RunAsync()
    {
        await OllamaRouteBuildsMixOrientedPrompt();
        await OpenAiCompatibleRouteIsSelectable();
        await CanonicalApiRoutesHaveExpectedPaths();
        await MissingProviderIsCleanlyUnavailable();
        PublicSettingsHideSecrets();
        LegacyWebsiteBasesNormalizeToCanonicalApiV1();
        var wrongUnit=Envelope(new JsonObject());wrongUnit["metrics"]![3]!["unit"]="dBFS";
        Expect(ContextContract.Validate(wrongUnit)!=null,"LUFS cannot be relabelled dBFS");
        var wrongBand=Envelope(new JsonObject());wrongBand["bands"]![16]!["centre_hz"]=300.0;
        Expect(ContextContract.Validate(wrongBand)!=null,"850 Hz cannot be replaced with 300 Hz");
        var stereo=Envelope(new JsonObject());stereo["profile_id"]="STEREO_PHASE_DIAGNOSTIC";stereo["profile_version"]=2;
        Expect(ContextContract.Validate(stereo)==null,"diagnostic profile v2 accepted");
        Expect(ContextContract.Validate(new JsonObject())!=null,"raw snapshot rejected");
        var current=Envelope(new JsonObject());
        Expect(ContextContract.Validate(current)==null,"filtered contract accepted");
        Expect(ContextContract.Validate(current,"beta")!=null,"cross-channel request rejected");
        var beta=Envelope(new JsonObject(),"beta");
        Expect(ContextContract.Validate(beta,"beta")==null,"Beta filtered contract accepted");
        Expect(ContextContract.Validate(beta,"official")!=null,"Beta cross-channel request rejected");
        current["profile_id"]="TRACKING_FAST";
        Expect(ContextContract.Validate(current)!=null,"unimplemented profile rejected");
        if (failures != 0)
        {
            Console.Error.WriteLine($"AifredIntelligenceHost contract tests: {failures} failure(s)");
            Environment.ExitCode = 1;
            return;
        }
        Console.WriteLine("AifredIntelligenceHost contract tests: PASS");
    }

    async Task OllamaRouteBuildsMixOrientedPrompt()
    {
        var capture = new RequestCapture();
        var router = new ProviderRouter(() => new HttpClient(new MockHandler(capture, request =>
        {
            if (request.RequestUri?.AbsolutePath == "/api/tags")
                return Json(HttpStatusCode.OK, """{"models":[{"name":"aifred:latest"}]}""");
            return Json(HttpStatusCode.OK,
                """{"message":{"role":"assistant","content":"A natural mocked reply."}}""");
        })));
        var settings = HostSettings.Defaults with { TimeoutMs = 5000 };
        var health = await router.CheckAsync(settings);
        Expect(health.Available, "mocked Ollama health must be available");

        const string question = "Why does my chorus feel wider but weaker?";
        var context = new JsonObject { ["mode"] = "Analyze" };
        Envelope(context);
        context["metrics"]![1] = new JsonObject
        {
            ["metric"] = "rms", ["display_name"] = "RMS", ["unit"] = "dBFS", ["available"] = true,
            ["typical"] = -12.5, ["minimum"] = -14.0, ["maximum"] = -10.0,
            ["latest"] = -11.0, ["trend"] = "stable"
        };
        var reply = await router.ChatAsync(settings, question, context);
        Expect(reply.Success && reply.Response == "A natural mocked reply.",
            "mocked Ollama response must pass through");
        var outbound = JsonNode.Parse(capture.LastBody)!.AsObject();
        var userContent = outbound["messages"]?[1]?["content"]?.GetValue<string>() ?? "";
        Expect(userContent.Contains("PRODUCER QUESTION", StringComparison.Ordinal)
               && userContent.Contains(question, StringComparison.Ordinal),
            "producer question must remain the actual user prompt");
        Expect(userContent.Contains("AUTHORITATIVE MEASURED CONTEXT", StringComparison.Ordinal)
               && userContent.Contains("RMS [dBFS]: available; typical=-12.5", StringComparison.Ordinal),
            "validated measurements must be presented as mix evidence");
        Expect(userContent.Contains("REFERENCE STATE", StringComparison.Ordinal)
               && userContent.Contains("PROVENANCE, EVIDENCE, CONFIDENCE, AND AVAILABILITY", StringComparison.Ordinal),
            "reference and evidence sections must be explicit");
        Expect(!userContent.TrimStart().StartsWith("{", StringComparison.Ordinal)
               && !userContent.Contains("\"message\"", StringComparison.Ordinal)
               && !userContent.Contains("\"context\"", StringComparison.Ordinal),
            "provider user content must not be raw transport JSON");
    }

    async Task OpenAiCompatibleRouteIsSelectable()
    {
        var capture = new RequestCapture();
        var router = new ProviderRouter(() => new HttpClient(new MockHandler(capture, request =>
        {
            if (request.RequestUri?.AbsolutePath == "/v1/models")
                return Json(HttpStatusCode.OK, """{"data":[{"id":"mix-model"}]}""");
            return Json(HttpStatusCode.OK,
                """{"choices":[{"message":{"content":"Compatible mocked reply."}}]}""");
        })));
        var settings = new HostSettings("openai-compatible", "https://provider.invalid/v1",
            "mix-model", "test-only-key", 5000, 8787);
        var health = await router.CheckAsync(settings);
        Expect(health.Available, "mocked OpenAI-compatible health must be available");
        var reply = await router.ChatAsync(settings, "What changed?",
            Envelope(new JsonObject { ["mode"] = "Compare", ["compare_b"] = Envelope(new JsonObject()) }));
        Expect(reply.Success && reply.Response == "Compatible mocked reply.",
            "OpenAI-compatible route must return provider text");
        Expect(capture.SawBearerToken, "OpenAI-compatible request must use configured bearer token");
    }

    async Task MissingProviderIsCleanlyUnavailable()
    {
        var router = new ProviderRouter(() => new HttpClient(new MockHandler(new(), _ =>
            throw new HttpRequestException("offline"))));
        var settings = new HostSettings("openai-compatible", "https://provider.invalid/v1",
            "mix-model", "", 1000, 8787);
        var health = await router.CheckAsync(settings);
        Expect(!health.Available && health.Error?.Contains("no configured API key") == true,
            "missing provider credentials must produce a clean unavailable state");
        var reply = await router.ChatAsync(settings, "Still meter?", Envelope(new JsonObject()));
        Expect(!reply.Success && reply.Error?.Contains("no configured API key") == true,
            "chat must remain unavailable without pretending the engine or DSP failed");
    }

    void PublicSettingsHideSecrets()
    {
        var settings = HostSettings.Defaults with { ApiKey = "do-not-return" };
        var json = settings.ToPublicJson().ToJsonString();
        Expect(!json.Contains("do-not-return") && !json.Contains("\"api_key\""),
            "public settings payload must not expose provider secrets");
    }

    async Task CanonicalApiRoutesHaveExpectedPaths()
    {
        var capture = new RequestCapture();
        var router = new ProviderRouter(() => new HttpClient(new MockHandler(capture, request =>
            request.RequestUri?.AbsolutePath.EndsWith("/models", StringComparison.Ordinal) == true
                ? Json(HttpStatusCode.OK, """{"data":[{"id":"aifred:latest"}]}""")
                : Json(HttpStatusCode.OK, """{"choices":[{"message":{"content":"Canonical reply."}}]}"""))));
        var settings = HostSettings.FromJson(new JsonObject
        {
            ["provider"] = "openai-compatible",
            ["endpoint"] = "https://north3rnlight3r.com/v1",
            ["model"] = "aifred:latest",
            ["api_key"] = "test-only-key"
        });
        Expect((await router.CheckAsync(settings)).Available, "canonical API model discovery must be available");
        Expect((await router.ChatAsync(settings, "Check the mix.", Envelope(new JsonObject()))).Success,
            "canonical API chat must be available");
        Expect((await router.ChatAsync(settings, "Check the Beta mix.", Envelope(new JsonObject(), "beta"))).Success,
            "canonical API chat must accept the Beta contract");
        Expect(capture.Paths.Contains("/api/v1/models"), "model discovery must use /api/v1/models");
        Expect(capture.Paths.Contains("/api/v1/chat/completions"), "chat must use /api/v1/chat/completions");
        Expect(capture.Channels.Contains("official"), "Flagship requests must identify the official channel");
        Expect(capture.Channels.Contains("beta"), "Beta requests must identify the beta channel");
    }

    void LegacyWebsiteBasesNormalizeToCanonicalApiV1()
    {
        foreach (var endpoint in new[]
                 {
                     "https://north3rnlight3r.com",
                     "https://north3rnlight3r.com/api",
                     "https://north3rnlight3r.com/api/v1",
                     "https://north3rnlight3r.com/v1",
                     "https://aifred-site.pages.dev/v1",
                     "https://dead-preview.aifred-site.pages.dev/api/v1"
                 })
        {
            var settings = HostSettings.FromJson(new JsonObject
            {
                ["provider"] = "openai-compatible",
                ["endpoint"] = endpoint,
                ["model"] = "aifred:latest"
            });
            Expect(settings.Endpoint == "https://north3rnlight3r.com/api/v1",
                $"legacy website base {endpoint} must normalize to the canonical API v1 route");
        }
        var external = HostSettings.FromJson(new JsonObject
        {
            ["provider"] = "openai-compatible",
            ["endpoint"] = "https://provider.invalid/custom/v1",
            ["model"] = "mix-model"
        });
        Expect(external.Endpoint == "https://provider.invalid/custom/v1",
            "non-AIFRED compatible providers must keep their configured endpoint");
    }

    void Expect(bool condition, string message)
    {
        if (condition) return;
        failures++;
        Console.Error.WriteLine("FAIL: " + message);
    }

    static HttpResponseMessage Json(HttpStatusCode status, string json) => new(status)
    {
        Content = new StringContent(json, Encoding.UTF8, "application/json")
    };
}

sealed class RequestCapture
{
    public string LastBody { get; set; } = "";
    public bool SawBearerToken { get; set; }
    public List<string> Paths { get; } = [];
    public List<string> Channels { get; } = [];
}

sealed class MockHandler(RequestCapture capture,
                         Func<HttpRequestMessage, HttpResponseMessage> responseFactory)
    : HttpMessageHandler
{
    protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,
                                                                  CancellationToken cancellationToken)
    {
        capture.SawBearerToken |= request.Headers.Authorization?.Scheme == "Bearer";
        capture.Paths.Add(request.RequestUri?.AbsolutePath ?? "");
        if (request.Headers.TryGetValues("X-AIFRED-Channel", out var channels))
            capture.Channels.AddRange(channels);
        if (request.Content != null)
            capture.LastBody = await request.Content.ReadAsStringAsync(cancellationToken);
        return responseFactory(request);
    }
}
