using System.Text.Json.Nodes;

namespace Aifred.Intelligence;

public static class ContextContract
{
    public static string? Validate(JsonObject context, string? channel = null)
    {
        if (Text(context, "schema") != "aifred.filtered-mix.v1") return "FilteredMixContext v1 is required.";
        var product = Text(context, "product_channel");
        if (product is not ("beta" or "official") || (channel != null && product != channel)) return "Product channel mismatch.";
        foreach (var name in new[] { "product_version", "plugin_instance_id", "session_id", "profile_id", "observation_id" })
            if (Text(context, name).Length is 0 or > 128) return "Missing or invalid identity: " + name;
        if (Text(context,"profile_id") is not ("MIX_BALANCED" or "SPECTRUM_SURGICAL" or "MASTERING_PRECISION" or "STEREO_PHASE_DIAGNOSTIC")) return "Unknown DSP profile.";
        if (context["profile_version"] is not JsonValue version || !version.TryGetValue<int>(out var revision) || revision != 1) return "Unsupported profile version.";
        if (context["metrics"] is not JsonArray metrics || metrics.Count != 16 || context["bands"] is not JsonArray bands || bands.Count != 30) return "Measurement contract is incomplete.";
        if (context["session_context"] is JsonArray history && history.Count > 4) return "Session history exceeds its bound.";
        foreach (var node in metrics.Concat(bands))
            if (node is not JsonObject metric || Text(metric,"metric").Length==0 || Text(metric,"unit").Length==0 || metric["available"] is not JsonValue) return "Metric identity, unit and availability are required.";
        return null;
    }
    public static string Text(JsonObject json, string name) => json[name] is JsonValue value && value.TryGetValue<string>(out var text) ? text : "";
}
