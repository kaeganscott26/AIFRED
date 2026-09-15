using System.Text.Json.Nodes;

namespace Aifred.Intelligence;

public static class ContextContract
{
    public static readonly (string Name, string Unit)[] Metrics = [
        ("sample_peak","dBFS"),("rms","dBFS"),("true_peak","dBTP"),
        ("momentary_loudness","LUFS"),("short_term_loudness","LUFS"),("integrated_loudness","LUFS"),
        ("loudness_range","LU"),("broadband_crest","dB"),("correlation","ratio"),
        ("left_energy","dBFS"),("right_energy","dBFS"),("mid_energy","dBFS"),("side_energy","dBFS"),
        ("left_right_balance","dB"),("side_to_mid","dB"),("width","percent")];
    public static readonly double[] Centres = [20,30,40,50,60,70,80,90,100,150,200,250,350,450,600,750,850,1000,1500,2000,3000,4000,6000,8000,10000,12000,14000,16000,18000,20000];
    public static string? Validate(JsonObject context, string? channel = null)
    {
        if (Text(context, "schema") != "aifred.filtered-mix.v1") return "FilteredMixContext v1 is required.";
        var product = Text(context, "product_channel");
        if (product is not ("beta" or "official") || (channel != null && product != channel)) return "Product channel mismatch.";
        foreach (var name in new[] { "product_version", "plugin_instance_id", "session_id", "profile_id", "observation_id" })
            if (Text(context, name).Length is 0 or > 128) return "Missing or invalid identity: " + name;
        if (Text(context,"profile_id") is not ("MIX_BALANCED" or "SPECTRUM_SURGICAL" or "MASTERING_PRECISION" or "STEREO_PHASE_DIAGNOSTIC")) return "Unknown DSP profile.";
        if (context["profile_version"] is not JsonValue version || !version.TryGetValue<int>(out var revision) || revision != (Text(context,"profile_id") == "STEREO_PHASE_DIAGNOSTIC" ? 2 : 1)) return "Unsupported profile version.";
        if (context["metrics"] is not JsonArray metrics || metrics.Count != 16 || context["bands"] is not JsonArray bands || bands.Count != 30) return "Measurement contract is incomplete.";
        if (context["session_context"] is JsonArray history && history.Count > 4) return "Session history exceeds its bound.";
        for (var i=0;i<metrics.Count;i++)
            if (metrics[i] is not JsonObject metric || Text(metric,"metric")!=Metrics[i].Name || Text(metric,"unit")!=Metrics[i].Unit || !Boolean(metric,"available") || metric.ContainsKey("centre_hz")) return "Metric identity, unit or availability mismatch.";
        for (var i=0;i<bands.Count;i++)
            if (bands[i] is not JsonObject band || Text(band,"metric")!="band_energy" || Text(band,"unit")!="dBFS" || !Boolean(band,"available") || band["centre_hz"] is not JsonValue centre || !centre.TryGetValue<double>(out var hz) || hz!=Centres[i]) return "Band frequency contract mismatch.";
        return null;
    }
    static bool Boolean(JsonObject json,string name) => json[name] is JsonValue value && value.TryGetValue<bool>(out _);
    public static string Text(JsonObject json, string name) => json[name] is JsonValue value && value.TryGetValue<string>(out var text) ? text : "";
}
