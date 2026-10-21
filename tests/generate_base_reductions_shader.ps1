param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory -Path $OutputDirectory|Out-Null
& "$PSScriptRoot/generate_power_shaders.ps1" -OutputDirectory "$OutputDirectory/math_sources"
$source=[IO.File]::ReadAllText("$OutputDirectory/math_sources/map.comp")
$start=$source.IndexOf('void main()');if($start -lt 0){throw 'Missing map entrypoint'}
$shader=$source.Substring(0,$start)
$shader+=@'
// Isolated batched64/256 reduction experiment, not a production MapGPU ABI.
void main(){
    uint batch=gl_GlobalInvocationID.x,n=pc.width;
    if(batch>=pc.count||(n!=64u&&n!=256u))return;
    uint base=batch*2u*n,outbase=batch*(n+3u);
    precise vec2 sum=vec2(0),logsum=vec2(0);
    for(uint k=0u;k<n;++k){
        sum=pairAdd(sum,vec2(load_float(base+k),0));
        logsum=pairAdd(logsum,vec2(load_float(base+n+k),0));
    }
    float reciprocal=n==64u?1.0/64.0:1.0/256.0;
    float mean=exactProduct(sum.x,reciprocal),logmean=exactProduct(logsum.x,reciprocal);
    if(pc.mode==1u){
        // Interleaved output for a subsequent GPU residual FFT, without readback.
        for(uint k=0u;k<n;++k){precise float centered=load_float(base+k)-mean;
            output_data.words[batch*2u*n+2u*k]=floatBitsToUint(centered);
            output_data.words[batch*2u*n+2u*k+1u]=0u;
        }return;
    }
    for(uint k=0u;k<n;++k){precise float centered=load_float(base+k)-mean;output_data.words[outbase+k]=floatBitsToUint(centered);}
    output_data.words[outbase+n]=floatBitsToUint(mean);
    output_data.words[outbase+n+1u]=floatBitsToUint(logmean);
    output_data.words[outbase+n+2u]=floatBitsToUint(nativePower(10.0,logmean));
}
'@
[IO.File]::WriteAllText("$OutputDirectory/reductions.comp",$shader,[Text.UTF8Encoding]::new($false))
