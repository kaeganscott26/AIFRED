import {mkdtempSync, mkdirSync, copyFileSync, cpSync, writeFileSync, readFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {resolve, dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {createRequire} from 'node:module';

const root=resolve(dirname(fileURLToPath(import.meta.url)), '..');
const source=join(root,'apps/website');
const require=createRequire(join(root,'apps/package.json'));
const {build}=require('esbuild');
const branch=process.argv[2];
if(!branch || !/^[a-zA-Z0-9._/-]+$/.test(branch))throw new Error('Provide the Pages branch explicitly.');
for(const check of ['check-website-modules.mjs','check-website-assets.mjs']) {
 const result=spawnSync(process.execPath,[join(root,'tools',check)],{cwd:root,stdio:'inherit'});
 if(result.status)process.exit(result.status);
}
// Pages does not honor .assetsignore. Package only this source directory's
// public files; retain the temporary package for inspection and recovery.
const staging=mkdtempSync(join(tmpdir(),'aifred-pages-'));
const output=join(staging,'AIFRED_Official-','apps','website');
mkdirSync(output,{recursive:true});
const publicFiles=['index.html','404.html','styles.css','app.js','config.js','ops.html','ops.css','ops.js','_headers'];
for(const file of publicFiles)copyFileSync(join(source,file),join(output,file));
cpSync(join(source,'assets'),join(output,'assets'),{recursive:true});
await build({entryPoints:[join(source,'_worker.js')],outfile:join(output,'_worker.js'),bundle:true,format:'esm',platform:'browser',target:'es2022',external:['cloudflare:*','node:*'],legalComments:'none'});
// Read bindings from the authoritative config, kept outside the asset output.
let config=readFileSync(join(source,'wrangler.toml'),'utf8');
config=config.replace('pages_build_output_dir = "apps/website"','pages_build_output_dir = "AIFRED_Official-/apps/website"');
writeFileSync(join(staging,'wrangler.toml'),config);
console.log(`Pages package from ${source}: ${output}`);
const sha=spawnSync('git',['rev-parse','HEAD'],{cwd:root,encoding:'utf8'}).stdout.trim();
const changed=spawnSync('git',['status','--porcelain','--','apps/website'],{cwd:root,encoding:'utf8'}).stdout.length>0;
const result=spawnSync(process.execPath,[join(root,'apps/node_modules/wrangler/bin/wrangler.js'),'pages','deploy',output,'--cwd',staging,'--project-name','aifred-site','--branch',branch,'--commit-hash',sha,`--commit-dirty=${changed}`],{cwd:root,stdio:'inherit'});
process.exit(result.status||0);
