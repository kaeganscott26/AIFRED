using System.Text;
using System.Text.Json.Nodes;

namespace Aifred.Intelligence;

/// <summary>
/// Builds the provider-facing, evidence-bound prompt after FilteredMixContext
/// validation. The provider receives readable sections rather than transport
/// JSON, while the values remain exactly those supplied by the DSP pipeline.
/// </summary>
public static class IntelligencePrompt
{
    public const string System =
        "You are AIFRED, a knowledgeable and conversational mix-engineering assistant embedded in the producer's workflow. " +
        "The producer's question is the actual user intent. The labeled mix context is read-only evidence from AIFRED's " +
        "validated DSP pipeline: aifred_engine, BufferHunter, ObservationSnapshot, aifred_filter, and FilteredMixContext. " +
        "Use supplied measurements exactly as stated; never invent, recompute, normalize, replace, or silently round a value. " +
        "Answer from the evidence and distinguish direct observations from supported interpretations and suggestions. " +
        "Use RMS, LUFS, true peak, width, correlation, crest, spectrum, dynamics, punch, and reference relationships only " +
        "when the supplied context supports them. Punch has no independent value unless one is supplied. " +
        "A healthy mix can be described as healthy. Do not force every answer into a problem and a fix. " +
        "Respect producer intent, answer follow-up questions naturally, and never claim that you heard or listened to audio. " +
        "Unavailable data is unknown, not zero. If no compatible reference is selected, say so clearly. " +
        "Do not dump, explain, or imitate JSON or the transport schema unless the producer explicitly asks for debugging. " +
        "Keep the response conversational and useful for the question asked.";

    public static string Build(string question, JsonObject context)
    {
        var output = new StringBuilder(4096);
        output.AppendLine("PRODUCER QUESTION");
        output.AppendLine(question.Trim());
        output.AppendLine();

        output.AppendLine("MODE");
        output.AppendLine(Text(context, "mode", "analyze"));
        output.AppendLine();

        output.AppendLine("AUTHORITATIVE MEASURED CONTEXT");
        AppendContextState(output, context);
        output.AppendLine();
        output.AppendLine("MEASUREMENTS");
        AppendMetrics(output, context["metrics"] as JsonArray);
        output.AppendLine();
        output.AppendLine("SPECTRUM EVIDENCE");
        AppendBands(output, context["bands"] as JsonArray);
        output.AppendLine();

        output.AppendLine("REFERENCE STATE");
        AppendReferenceState(output, context);
        output.AppendLine();

        output.AppendLine("PROVENANCE, EVIDENCE, CONFIDENCE, AND AVAILABILITY");
        output.AppendLine("Every measurement above is copied from the validated FilteredMixContext supplied by the producer's DSP pipeline.");
        output.AppendLine("The context's freshness, sufficiency, signal state, metric availability, and reference compatibility bound what can be concluded.");
        output.AppendLine("No numeric confidence score is supplied; describe uncertainty from those stated states instead of inventing confidence.");
        output.AppendLine("Observation is fact. Interpretation is a supported reading of facts. Suggestion is optional and must respect producer intent.");
        output.AppendLine();

        output.AppendLine("AIFRED BEHAVIOR");
        output.AppendLine("Answer the producer's question directly in natural language. Mention healthy aspects when the evidence supports them.");
        output.AppendLine("Separate what the measurements show from what the producer might try. Do not claim an unavailable metric or reference relationship.");
        return output.ToString().TrimEnd();
    }

    static void AppendContextState(StringBuilder output, JsonObject context)
    {
        output.AppendLine("- observation state: " + Text(context, "observation_state", "unavailable"));
        output.AppendLine("- available: " + BoolText(context, "available"));
        output.AppendLine("- fresh: " + BoolText(context, "fresh"));
        output.AppendLine("- signal active: " + BoolText(context, "signal_active"));
        output.AppendLine("- sufficient observation: " + BoolText(context, "sufficient_observation"));
        output.AppendLine("- profile: " + Text(context, "profile_id", "unavailable") + " v" + Text(context, "profile_version", "unavailable"));
        output.AppendLine("- measurement configuration: " + Text(context, "measurement_configuration_id", "unavailable"));
        output.AppendLine("- observation duration: " + Text(context, "observation_seconds", "unavailable") + " seconds");
        output.AppendLine("- age: " + Text(context, "age_seconds", "unavailable") + " seconds");
        output.AppendLine("- transport: " + (Bool(context, "transport_known") ? BoolText(context, "transport_playing") : "unavailable"));
    }

    static void AppendMetrics(StringBuilder output, JsonArray? metrics)
    {
        if (metrics is null)
        {
            output.AppendLine("- unavailable: no metric list supplied");
            return;
        }

        foreach (var item in metrics)
        {
            if (item is not JsonObject metric) continue;
            var name = Text(metric, "display_name", Text(metric, "metric", "measurement"));
            var unit = Text(metric, "unit", "unit unavailable");
            var availability = Bool(metric, "available") ? "available" : "unavailable";
            output.Append("- ").Append(name).Append(" [").Append(unit).Append("]: ").Append(availability);
            if (Bool(metric, "available"))
            {
                output.Append("; typical=").Append(Value(metric, "typical"));
                output.Append("; latest=").Append(Value(metric, "latest"));
                output.Append("; observed range=").Append(Value(metric, "minimum"));
                output.Append(" to ").Append(Value(metric, "maximum"));
                output.Append("; trend=").Append(Text(metric, "trend", "unavailable"));
                output.Append("; reference relationship=").Append(Text(metric, "semantic_state", Text(metric, "reference_relationship", "unavailable")));
                output.Append("; reference bounds=").Append(Value(metric, "reference_low"));
                output.Append(" to ").Append(Value(metric, "reference_high"));
            }
            output.AppendLine();
        }
    }

    static void AppendBands(StringBuilder output, JsonArray? bands)
    {
        if (bands is null)
        {
            output.AppendLine("- unavailable: no spectrum bands supplied");
            return;
        }

        var available = 0;
        foreach (var item in bands)
        {
            if (item is not JsonObject band) continue;
            if (!Bool(band, "available")) continue;
            ++available;
            output.Append("- ").Append(Value(band, "centre_hz")).Append(" Hz ")
                .Append(Text(band, "region", "band")).Append(": ")
                .Append(Value(band, "typical")).Append(" dBFS");
            var relationship = Text(band, "semantic_state", Text(band, "reference_relationship", "unavailable"));
            if (!string.Equals(relationship, "unavailable", StringComparison.OrdinalIgnoreCase))
                output.Append("; reference relationship=").Append(relationship);
            output.AppendLine();
        }
        if (available == 0) output.AppendLine("- unavailable: no spectrum band is currently available");
    }

    static void AppendReferenceState(StringBuilder output, JsonObject context)
    {
        var compatibility = Text(context, "reference_compatibility", "no_reference");
        if (compatibility.Equals("no_reference", StringComparison.OrdinalIgnoreCase))
        {
            output.AppendLine("- no compatible reference is selected");
            return;
        }

        output.AppendLine("- selected reference id: " + Text(context, "reference_id", "unavailable"));
        output.AppendLine("- compatibility: " + compatibility);
        output.AppendLine("- use reference relationships only where the context marks them as compatible and available");
        if (context["compare_b"] is JsonObject compare)
        {
            output.AppendLine("- compare target is supplied as a second measured context: " + Text(compare, "observation_state", "unavailable"));
        }
    }

    static string Text(JsonObject json, string name, string fallback = "unavailable")
    {
        if (json[name] is not JsonNode node) return fallback;
        var text = node is JsonValue value && value.TryGetValue<string>(out var stringValue)
            ? stringValue : node.ToString();
        return string.IsNullOrWhiteSpace(text) ? fallback : text;
    }

    static string Value(JsonObject json, string name) => json[name] is null ? "unavailable" : json[name]!.ToString();

    static bool Bool(JsonObject json, string name) =>
        json[name] is JsonValue value && value.TryGetValue<bool>(out var result) && result;

    static string BoolText(JsonObject json, string name) => Bool(json, name) ? "true" : "false";
}
