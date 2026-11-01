// Exact comparison wrapper. Never substitutes official tensors/scores for C++ work.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const [exe,reference,test,data,dll,stockPath,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: executable reference.png test.png model-data fftw-float-dll stock-score.json fresh-root');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const stock=JSON.parse(fs.readFileSync(stockPath,'utf8'));
if(stock.input_precision!=='single'||stock.backend!=='CPU'||stock.Q_class!=='single'||stock.Q_JOD_class!=='single')throw Error('Stock baseline precision/backend mismatch');
const ppd=62.729130063099447;if(stock.ppd!==ppd)throw Error('Viewing condition mismatch');
for(const [i,p] of [reference,test].entries())if(stock.inputs[i].source_sha256!==sha(p))throw Error('PNG SHA256 differs from baseline');
const files={executable:exe,reference_png:reference,test_png:test,fftw_float_dll:dll,stock_score:stockPath};
for(const name of ['ciexyz31.csv','emission_spectra_oled.csv','log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt'])files[name]=path.join(data,name);
const provenance=Object.fromEntries(Object.entries(files).map(([k,p])=>[k,{path:path.resolve(p),sha256:sha(p)}]));
const run=cp.spawnSync(path.resolve(exe),[reference,test,data,dll,String(ppd),out],{stdio:'inherit',windowsHide:true});
if(run.error)throw run.error;if(run.status!==0)throw Error(`C++ failed (${run.status})`);
const actual=JSON.parse(fs.readFileSync(path.join(out,'score.json'),'utf8'));
const exact=actual.Q_hex===stock.Q_hex&&actual.Q_JOD_hex===stock.Q_JOD_hex;
fs.writeFileSync(path.join(out,'comparison.json'),JSON.stringify({exact,Q_hex:actual.Q_hex,reference_Q_hex:stock.Q_hex,Q_JOD_hex:actual.Q_JOD_hex,reference_Q_JOD_hex:stock.Q_JOD_hex,provenance},null,2)+'\n');
console.log(exact?'EXACT Q AND Q_JOD':'BIT MISMATCH: not an exact pass');process.exitCode=exact?0:2;
