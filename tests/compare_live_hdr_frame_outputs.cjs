// Compare already completed runs without recomputing or substituting scores.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [exe,reference,test,data,dll,stockPath,actualPath,out]=process.argv.slice(2);
if(!out||fs.existsSync(out))throw Error('Usage: exe reference.png test.png model-data fftw-dll stock.json actual.json fresh-comparison.json');
const sha=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const stock=JSON.parse(fs.readFileSync(stockPath,'utf8')),actual=JSON.parse(fs.readFileSync(actualPath,'utf8'));
if(stock.reference_version!=='3.0.7'||stock.input_precision!=='single'||stock.backend!=='CPU'||stock.Q_class!=='single'||stock.Q_JOD_class!=='single'||stock.ppd!==actual.ppd||stock.fftw_threads!==actual.fftw_threads||stock.fftw_planner!==actual.fftw_planner)throw Error('Precision/settings/backend mismatch');
if(actual.age!==24||actual.display!=='oled'||actual.peak!==2000||actual.ambient_lux!==200||actual.reflectivity!==.005)throw Error('Unexpected local display model');
for(const [i,p] of [reference,test].entries())if(stock.inputs[i].source_sha256!==sha(p)||stock.inputs[i].values!==3*actual.width*actual.height)throw Error('Reference tensor source/shape differs');
const files={executable:exe,reference_png:reference,test_png:test,fftw_float_dll:dll,stock_score:stockPath,actual_score:actualPath};
for(const name of ['ciexyz31.csv','emission_spectra_oled.csv','log_cone_smith_pokorny_1975.csv','cie_scotopic_lum.txt'])files[name]=path.join(data,name);
const provenance=Object.fromEntries(Object.entries(files).map(([key,p])=>[key,{path:path.resolve(p),sha256:sha(p)}]));
const exact=actual.Q_hex===stock.Q_hex&&actual.Q_JOD_hex===stock.Q_JOD_hex;
fs.writeFileSync(out,JSON.stringify({exact,Q_hex:actual.Q_hex,reference_Q_hex:stock.Q_hex,Q_JOD_hex:actual.Q_JOD_hex,reference_Q_JOD_hex:stock.Q_JOD_hex,provenance},null,2)+'\n');
console.log(exact?'EXACT Q AND Q_JOD':'BIT MISMATCH: not an exact pass');process.exitCode=exact?0:2;
