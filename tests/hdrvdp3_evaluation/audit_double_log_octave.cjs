const fs=require('fs'),p=require('path');const root=process.argv[2],read=f=>fs.readFileSync(p.join(root,f));
const native=read('octave_queries.f64'),cpu=read('run/expected.f64'),gpu=read('compensated/run/actual.f64');
if(native.length!==663580*16||cpu.length!==native.length||gpu.length!==native.length||!read('run/input.f64').equals(read('compensated/run/input.f64')))throw Error('Shape/input mismatch');
const result={scope:'Octave11.3 built-in query functions on identical native-derived inputs;not complete metric execution.',queries_per_operation:663580,CPU_vs_Octave:[0,0],compensated_GPU_vs_Octave:[0,0]};
for(let j=0;j<native.length/8;j++){const n=native.readBigUInt64LE(j*8);result.CPU_vs_Octave[j%2]+=Number(cpu.readBigUInt64LE(j*8)!==n);result.compensated_GPU_vs_Octave[j%2]+=Number(gpu.readBigUInt64LE(j*8)!==n);}
fs.writeFileSync(p.join(root,'octave_audit.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
