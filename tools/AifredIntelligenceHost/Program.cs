using System.Net;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using Aifred.Intelligence;

var channelFile = Path.Combine(AppContext.BaseDirectory, "channel.json");
var channel = File.Exists(channelFile) ? ContextContract.Text(JsonNode.Parse(File.ReadAllText(channelFile))!.AsObject(), "channel") : "official";
var channelArgument = Array.IndexOf(args, "--channel");
if (channelArgument >= 0 && channelArgument + 1 < args.Length) channel = args[channelArgument + 1];
if (channel is not ("beta" or "official")) throw new ArgumentException("channel must be beta or official");
var store = new SettingsStore(channel);
var settings = store.Load();
var router = new ProviderRouter();
using var listener = new HttpListener();
using var slots = new SemaphoreSlim(8);
listener.Prefixes.Add($"http://127.0.0.1:{settings.Port}/");
listener.Start();
Console.WriteLine($"AifredIntelligenceHost {channel} listening on {settings.Port}");
while (listener.IsListening)
{
    var request = await listener.GetContextAsync();
    if (!await slots.WaitAsync(0)) { await Write(request, new JsonObject { ["error"]="Host is busy." }, 429); continue; }
    _ = Handle(request);
}

async Task Handle(HttpListenerContext http)
{
    try
    {
        var path = http.Request.Url?.AbsolutePath;
        if (http.Request.HttpMethod=="GET" && path=="/health")
        {
            var status=await router.CheckAsync(Volatile.Read(ref settings).WithEnvironmentOverrides());
            await Write(http,new JsonObject { ["host_identity"]="AifredIntelligenceHost",["product_channel"]=channel,["context_schema"]="aifred.filtered-mix.v1",["ai_available"]=status.Available,["provider"]=status.Provider,["model_name"]=status.Model,["last_error"]=status.Error });
        }
        else if (http.Request.HttpMethod=="POST" && path=="/chat")
        {
            var body=await Read(http.Request);
            if(body["context"] is not JsonObject context) {await Write(http,new JsonObject{["error"]="FilteredMixContext is required."},400);return;}
            var error=ContextContract.Validate(context,channel);
            var question=ContextContract.Text(body,"message");
            if(error!=null || question.Length is 0 or >2048) {await Write(http,new JsonObject{["error"]=error??"Question is missing or too long."},400);return;}
            var response=await router.ChatAsync(Volatile.Read(ref settings).WithEnvironmentOverrides(),question,context);
            await Write(http,new JsonObject{["response"]=response.Response,["error"]=response.Error,["plugin_instance_id"]=context["plugin_instance_id"]!.DeepClone(),["session_id"]=context["session_id"]!.DeepClone(),["product_channel"]=channel,["profile_id"]=context["profile_id"]!.DeepClone()},response.Success?200:503);
        }
        else if (path=="/v1/settings" && http.Request.HttpMethod=="GET")
            await Write(http,new JsonObject{["ok"]=true,["config"]=Volatile.Read(ref settings).ToPublicJson()});
        else if (path=="/v1/settings" && http.Request.HttpMethod=="POST")
        {
            var replacement=HostSettings.FromJson(await Read(http.Request),Volatile.Read(ref settings)) with {Port=settings.Port};
            store.Save(replacement);Volatile.Write(ref settings,replacement);
            await Write(http,new JsonObject{["ok"]=true,["config"]=replacement.ToPublicJson()});
        }
        else await Write(http,new JsonObject{["error"]="Unknown route."},404);
    }
    catch(Exception error) when(error is JsonException or InvalidOperationException or ArgumentException or IOException)
    {
        Console.Error.WriteLine($"Request failed: {error.GetType().Name}");
        await Write(http,new JsonObject{["error"]="Invalid request or unavailable local storage."},400);
    }
    finally {slots.Release();}
}
static async Task<JsonObject> Read(HttpListenerRequest request)
{
    const int limit=524288;
    if(request.ContentLength64>limit)throw new ArgumentException("Request too large.");
    using var memory=new MemoryStream();var buffer=new byte[8192];int count;
    using var timeout=new CancellationTokenSource(TimeSpan.FromSeconds(10));
    while((count=await request.InputStream.ReadAsync(buffer,timeout.Token))>0)
    {if(memory.Length+count>limit)throw new ArgumentException("Request too large.");memory.Write(buffer,0,count);}
    return JsonNode.Parse(memory.ToArray()) as JsonObject??throw new ArgumentException("Object required.");
}
static async Task Write(HttpListenerContext http,JsonObject json,int code=200)
{
    var data=Encoding.UTF8.GetBytes(json.ToJsonString());
    try {http.Response.StatusCode=code;http.Response.ContentType="application/json";http.Response.ContentLength64=data.Length;await http.Response.OutputStream.WriteAsync(data);}
    catch(HttpListenerException) { Console.Error.WriteLine("Client disconnected before response delivery."); }
    finally {http.Response.Close();}
}
