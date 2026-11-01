param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$VulkanBin=$env:VULKAN_SDK+'/Bin')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh output required'}
New-Item -ItemType Directory $OutputDirectory | Out-Null
$records=@()
foreach($operation in @('sqrt','fma','log','log2','exp','exp2','pow')){
    $expression=switch($operation){'fma'{'fma(x,y,x)'} 'pow'{'pow(x,y)'} default{"$operation(x)"}}
    $source=@"
#version 450
#extension GL_ARB_gpu_shader_fp64 : require
layout(local_size_x=1) in;
layout(set=0,binding=0,std430) readonly buffer Input { double values[]; } src;
layout(set=0,binding=1,std430) writeonly buffer Output { double values[]; } dst;
void main(){ double x=src.values[0],y=src.values[1]; precise double result=$expression; dst.values[0]=result; }
"@
    $shader=Join-Path $OutputDirectory "$operation.comp"
    [IO.File]::WriteAllText($shader,$source,[Text.UTF8Encoding]::new($false))
    # Native compiler rejection is an expected measurement, not a script failure.
    $oldPreference=$ErrorActionPreference
    $ErrorActionPreference='Continue'
    $log=& "$VulkanBin/glslangValidator.exe" -V --target-env vulkan1.2 $shader -o "$OutputDirectory/$operation.spv" 2>&1
    $exitCode=$LASTEXITCODE
    $ErrorActionPreference=$oldPreference
    $log | Set-Content "$OutputDirectory/$operation.compile.log"
    $validated=$false;$conversions=$null;$doubleInstructions=@()
    if($exitCode -eq 0){
        & "$VulkanBin/spirv-val.exe" --target-env vulkan1.2 "$OutputDirectory/$operation.spv" 2>&1 | Set-Content "$OutputDirectory/$operation.validate.log"
        if($LASTEXITCODE -ne 0){throw "Invalid compiled module: $operation"}
        $validated=$true
        & "$VulkanBin/spirv-dis.exe" "$OutputDirectory/$operation.spv" -o "$OutputDirectory/$operation.spvasm"
        if($LASTEXITCODE -ne 0){throw 'Disassembly failed'}
        $assembly=[IO.File]::ReadAllText("$OutputDirectory/$operation.spvasm")
        $conversions=([regex]::Matches($assembly,'OpFConvert')).Count
        $doubleInstructions=@($assembly -split "`n" | Where-Object {$_ -match 'OpExtInst %double'})
        if($conversions -ne 0 -or $doubleInstructions.Count -eq 0){throw "Unexpected precision: $operation"}
    }
    $records+=[PSCustomObject]@{operation=$operation;compiler_exit=$exitCode;validated=$validated;float_conversions=$conversions;double_instructions=$doubleInstructions}
}
& "$VulkanBin/glslangValidator.exe" --version | Set-Content "$OutputDirectory/compiler_version.txt"
$records | ConvertTo-Json -Depth 5 | Set-Content "$OutputDirectory/results.json"
foreach($required in @('sqrt','fma')){if(-not ($records | Where-Object operation -eq $required).validated){throw "Missing required Double arithmetic: $required"}}
# This probe establishes toolchain capability only; successful compilation is
# never evidence of numerical parity, and no production runtime is modified.
'Completed capability probe; no GPU numerical parity claimed.' | Set-Content "$OutputDirectory/COMPLETE.txt"
$records | Select-Object operation,compiler_exit,validated | Format-Table
