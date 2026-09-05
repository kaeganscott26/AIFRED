"""Independent FFmpeg ebur128 comparison; generated fixtures only, no DAW claim."""
from pathlib import Path
import array,json,math,re,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2]
ffmpeg=shutil.which('ffmpeg')
if not ffmpeg: raise SystemExit('FFmpeg unavailable: independent meter validation not performed')
reports=ROOT/'out/windows-x64/build/reports';reports.mkdir(parents=True,exist_ok=True)
fixture=reports/'meter-reference.f32le'
samples=array.array('f')
for i in range(40*48000):
    amplitude=.1 if i<20*48000 else 10**(-30/20)
    value=amplitude*math.sin(2*math.pi*1000*i/48000)
    samples.extend((value,value))
with fixture.open('wb') as stream:samples.tofile(stream)
measured=json.loads(subprocess.check_output([str(ROOT/'out/windows-x64/build/shared-dsp/aifred_fixture_meter.exe'),str(fixture)],text=True))
run=subprocess.run([ffmpeg,'-hide_banner','-f','f32le','-ar','48000','-ac','2','-i',str(fixture),'-af','ebur128=peak=true','-f','null','-'],text=True,capture_output=True,check=True)
summary=run.stderr.rsplit('Summary:',1)[1]
reference={'integrated_lufs':float(re.search(r'I:\s*([-\d.]+) LUFS',summary)[1]),'true_peak_dbtp':float(re.search(r'Peak:\s*([-\d.]+) dBFS',summary)[1]),'lra_lu':float(re.search(r'LRA:\s*([-\d.]+) LU',summary)[1])}
tolerances={'integrated_lufs':.15,'true_peak_dbtp':.3,'lra_lu':1.0}
result={'fixture':'48 kHz stereo 1 kHz sine, 20 s at -20 dBFS peak then 20 s at -30 dBFS peak','aifred':measured,'ffmpeg_ebur128':reference,'tolerances':tolerances,'ffmpeg_version':subprocess.check_output([ffmpeg,'-version'],text=True).splitlines()[0]}
(reports/'meter-reference.json').write_text(json.dumps(result,indent=2)+'\n')
for name,tolerance in tolerances.items():
    if abs(measured[name]-reference[name])>tolerance:raise SystemExit(f'{name} reference comparison failed: {result}')
print(json.dumps(result,indent=2))
