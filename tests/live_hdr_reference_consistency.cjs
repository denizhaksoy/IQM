// All observations must agree; a majority can never make an outlier pass.
function referenceConsistency(records){
 if(!Array.isArray(records)||records.length<1)throw Error('Reference observations required');
 const first=records[0],profile=s=>JSON.stringify({version:s.reference_version,backend:s.backend,precision:s.input_precision,ppd:s.ppd,threads:s.fftw_threads,planner:s.fftw_planner,inputs:s.inputs.map(i=>({name:i.name,source:i.source_sha256,linear:i.linear_sha256,values:i.values}))});
 for(const r of records){
  if(r.reference_version!=='3.0.7'||r.backend!=='CPU'||r.input_precision!=='single'||r.Q_class!=='single'||r.Q_JOD_class!=='single'||r.fftw_threads!==3||r.fftw_planner!=='estimate'||profile(r)!==profile(first))throw Error('Reference observations use different inputs/settings');
  for(const key of ['Q_hex','Q_JOD_hex']){if(!/^[0-9a-f]{8}$/i.test(r[key]))throw Error('Invalid Single reference bits');const b=Buffer.alloc(4);b.writeUInt32LE(parseInt(r[key],16));if(!Number.isFinite(b.readFloatLE()))throw Error('Nonfinite reference score');}
 }
 return {observations:records.length,all_identical:records.every(r=>r.Q_hex===first.Q_hex&&r.Q_JOD_hex===first.Q_JOD_hex),Q_hex:first.Q_hex,Q_JOD_hex:first.Q_JOD_hex};
}
module.exports={referenceConsistency};
