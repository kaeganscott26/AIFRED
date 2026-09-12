#include "MixMemoryIndex.h"

#if JUCE_WINDOWS
#include <winsqlite/winsqlite3.h>
#endif
#include <bit>
#include <cmath>

namespace aifred {
namespace {
constexpr double transientLifetimeSeconds=600.0;
#if JUCE_WINDOWS
sqlite3* db(void* value) noexcept {return static_cast<sqlite3*>(value);}
void execute(sqlite3* database,const char* sql) noexcept {sqlite3_exec(database,sql,nullptr,nullptr,nullptr);}
#endif
}

MixMemoryIndex::MixMemoryIndex():sessionId_(juce::Uuid().toString())
{
#if JUCE_WINDOWS
  const auto configured=juce::SystemStats::getEnvironmentVariable("AIFRED_MEMORY_DB_PATH",{});
  const auto file=configured.isNotEmpty()?juce::File(configured):juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("AIFRED").getChildFile("aifred-memory.sqlite");
  file.getParentDirectory().createDirectory();
  sqlite3* opened=nullptr;
  if(sqlite3_open_v2(file.getFullPathName().toRawUTF8(),&opened,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,nullptr)==SQLITE_OK)
  {
    database_=opened;storageAvailable_=true;
    execute(opened,"CREATE TABLE IF NOT EXISTS sessions(id TEXT PRIMARY KEY,started REAL NOT NULL,updated REAL NOT NULL,profile TEXT NOT NULL,snapshot_count INTEGER NOT NULL DEFAULT 0);");
    execute(opened,"CREATE TABLE IF NOT EXISTS snapshots(id INTEGER PRIMARY KEY AUTOINCREMENT,session_id TEXT NOT NULL,timestamp REAL NOT NULL,profile TEXT NOT NULL,mode TEXT NOT NULL,reference_id TEXT,rms REAL,true_peak REAL,crest REAL,lufs REAL,width REAL,correlation REAL,bands_json TEXT NOT NULL);");
    execute(opened,"CREATE INDEX IF NOT EXISTS idx_aifred_snapshots_session_time ON snapshots(session_id,timestamp DESC);");
    sqlite3_stmt* statement=nullptr;
    if(sqlite3_prepare_v2(opened,"INSERT INTO sessions(id,started,updated,profile,snapshot_count) VALUES(?,?,?,?,0);",-1,&statement,nullptr)==SQLITE_OK)
    {
      const auto now=juce::Time::getCurrentTime().toMilliseconds()/1000.0;
      sqlite3_bind_text(statement,1,sessionId_.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_bind_double(statement,2,now);sqlite3_bind_double(statement,3,now);
      sqlite3_bind_text(statement,4,"MIX_BALANCED",-1,SQLITE_STATIC);sqlite3_step(statement);
    }
    sqlite3_finalize(statement);
    execute(opened,"DELETE FROM snapshots WHERE session_id IN (SELECT id FROM sessions ORDER BY updated DESC LIMIT -1 OFFSET 10);");
    execute(opened,"DELETE FROM sessions WHERE id IN (SELECT id FROM sessions ORDER BY updated DESC LIMIT -1 OFFSET 10);");
    if(sqlite3_prepare_v2(opened,"SELECT COUNT(*) FROM sessions;",-1,&statement,nullptr)==SQLITE_OK&&sqlite3_step(statement)==SQLITE_ROW)persistedSessions_=sqlite3_column_int(statement,0);
    sqlite3_finalize(statement);
  }
  else if(opened)sqlite3_close(opened);
#endif
}

MixMemoryIndex::~MixMemoryIndex()
{
#if JUCE_WINDOWS
  if(database_)sqlite3_close(db(database_));
#endif
}

std::uint64_t MixMemoryIndex::candleFingerprint(const BetaView& state) const noexcept
{
  std::uint64_t hash=1469598103934665603ull;
  const auto mix=[&](float value){hash^=std::bit_cast<std::uint32_t>(value);hash*=1099511628211ull;};
  hash^=static_cast<std::uint64_t>(state.metrics.liveCandleCount);hash*=1099511628211ull;
  // The last slot is the still-forming candle. Earlier slots change only when
  // the existing 3-second candle state commits; this subscribes without
  // changing its timing.
  for(std::size_t i=0;i<9;++i){mix(state.metrics.liveCandleOpen[i]);mix(state.metrics.liveCandleHigh[i]);mix(state.metrics.liveCandleLow[i]);mix(state.metrics.liveCandleClose[i]);}
  return hash;
}

void MixMemoryIndex::observe(const BetaView& state,AnalysisMode mode,const juce::String& reference,bool hostAvailable)
{
  updated_=false;hostAvailable_=hostAvailable;profile_=state.activeProfile;
  const auto now=juce::Time::getCurrentTime().toMilliseconds()/1000.0;prune(now);
  if(!state.hasSignal||!state.valuesValid||state.metrics.liveCandleCount<=0)return;
  const auto fingerprint=candleFingerprint(state);if(fingerprint==lastFingerprint_)return;
  lastFingerprint_=fingerprint;transient_.push_back({now,fingerprint});updated_=true;persist(state,mode,reference,now);prune(now);
}

void MixMemoryIndex::prune(double now)
{
  while(!transient_.empty()&&now-transient_.front().timestamp>transientLifetimeSeconds){transient_.pop_front();++expired_;}
}

void MixMemoryIndex::persist(const BetaView& state,AnalysisMode mode,const juce::String& reference,double now)
{
#if JUCE_WINDOWS
  if(!database_)return;auto* database=db(database_);sqlite3_stmt* statement=nullptr;
  juce::Array<juce::var> bands;for(const auto value:state.metrics.spectrumBands)bands.add(value);const auto bandsJson=juce::JSON::toString(juce::var(bands),true);
  if(sqlite3_prepare_v2(database,"INSERT INTO snapshots(session_id,timestamp,profile,mode,reference_id,rms,true_peak,crest,lufs,width,correlation,bands_json) VALUES(?,?,?,?,?,?,?,?,?,?,?,?);",-1,&statement,nullptr)==SQLITE_OK)
  {
    const auto profile=juce::String(core::profile(state.activeProfile).name.data());const auto modeText=mode==AnalysisMode::Reference?"reference":mode==AnalysisMode::Compare?"compare":"analyze";
    sqlite3_bind_text(statement,1,sessionId_.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_bind_double(statement,2,now);sqlite3_bind_text(statement,3,profile.toRawUTF8(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(statement,4,modeText,-1,SQLITE_STATIC);sqlite3_bind_text(statement,5,reference.toRawUTF8(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(statement,6,state.metrics.rmsDb);sqlite3_bind_double(statement,7,state.metrics.truePeakDb);sqlite3_bind_double(statement,8,state.metrics.crestDb);
    sqlite3_bind_double(statement,9,state.metrics.shortTermLufs);sqlite3_bind_double(statement,10,state.metrics.stereoWidth*100.0f);sqlite3_bind_double(statement,11,state.metrics.correlation);
    sqlite3_bind_text(statement,12,bandsJson.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_step(statement);
  }
  sqlite3_finalize(statement);
  if(sqlite3_prepare_v2(database,"UPDATE sessions SET updated=?,profile=?,snapshot_count=snapshot_count+1 WHERE id=?;",-1,&statement,nullptr)==SQLITE_OK)
  {const auto profile=juce::String(core::profile(state.activeProfile).name.data());sqlite3_bind_double(statement,1,now);sqlite3_bind_text(statement,2,profile.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(statement,3,sessionId_.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_step(statement);}
  sqlite3_finalize(statement);
  if(sqlite3_prepare_v2(database,"DELETE FROM snapshots WHERE session_id=? AND timestamp<?;",-1,&statement,nullptr)==SQLITE_OK)
  {sqlite3_bind_text(statement,1,sessionId_.toRawUTF8(),-1,SQLITE_TRANSIENT);sqlite3_bind_double(statement,2,now-transientLifetimeSeconds);sqlite3_step(statement);}
  sqlite3_finalize(statement);
#else
  juce::ignoreUnused(state,mode,reference,now);
#endif
}

MemoryTelemetry MixMemoryIndex::telemetry() const noexcept
{
  MemoryTelemetry result;result.transientSnapshots=static_cast<int>(transient_.size());result.persistedSessions=persistedSessions_;result.expiredSnapshots=expired_;
  result.newestAgeSeconds=transient_.empty()?0.0:juce::Time::getCurrentTime().toMilliseconds()/1000.0-transient_.back().timestamp;
  result.activeProfile=profile_;result.updated=updated_;result.storageAvailable=storageAvailable_;result.hostAvailable=hostAvailable_;return result;
}

} // namespace aifred
