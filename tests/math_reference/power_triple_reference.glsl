// Precision fallback for ambiguous binary32 rounding, retaining the native
// cubic log/exp polynomials rather than changing their approximation.
vec3 tripleFloatAdd(vec3 a,float b) {
    precise vec2 s=pairSum(a.x,b),t=pairSum(a.y,s.y),u=pairSum(a.z,t.y);
    precise vec2 v=pairSum(s.x,t.x),w=pairSum(v.y,u.x),r=pairSum(v.x,w.x);
    precise vec2 low=pairSum(r.y,w.y+u.y),hi=pairSum(r.x,low.x),tail=pairSum(hi.y,low.y);
    return vec3(hi.x,tail.x,tail.y);
}
vec3 tripleAdd(vec3 a,vec3 b) {
    return tripleFloatAdd(tripleFloatAdd(tripleFloatAdd(a,b.z),b.y),b.x);
}
vec3 tripleMul(vec3 a,vec3 b) {
    precise vec3 result=vec3(0);
    for(int i=2;i>=0;--i)for(int j=2;j>=0;--j) {
        precise float p=a[i]*b[j];
        precise float ca=4097.0*a[i],cb=4097.0*b[j];
        precise float ah=ca-(ca-a[i]),al=a[i]-ah,bh=cb-(cb-b[j]),bl=b[j]-bh;
        precise float error=((ah*bh-p)+ah*bl+al*bh)+al*bl;
        result=tripleFloatAdd(tripleFloatAdd(result,error),p);
    }
    return result;
}
float nativePowerFarPrecise(float x,float y) {
    uint bits=floatBitsToUint(x),mantissa=bits&0x7fffffu;
    int exponent=int(bits>>23)-127;
    if((bits&0x7f800000u)==0u) {
        int shift=23-findMSB(mantissa);mantissa=(mantissa<<uint(shift))&0x7fffffu;exponent=-126-shift;
    }
    uint index=(mantissa>>15)+((mantissa>>14)&1u);
    float m=uintBitsToFloat(mantissa|0x3f000000u),center=0.5+float(index)*(1.0/512.0);
    precise vec3 r=tripleMul(vec3(center-m,0,0),powerInverse3[index]);
    precise vec3 poly=tripleAdd(tripleMul(oneThird3,r),vec3(0.5,0,0));
    poly=tripleAdd(tripleMul(poly,r),vec3(1,0,0));
    precise vec3 logarithm=tripleAdd(tripleAdd(tripleMul(vec3(float(exponent),0,0),powerLn23),powerLog3[index]),-tripleMul(r,poly));
    precise vec3 z=tripleMul(vec3(y,0,0),logarithm),scaled=tripleMul(z,powerMultiplier3);
    int n=int(floor(scaled.x));float fraction=scaled.x-float(n);
    if(fraction>0.5||(fraction==0.5&&(scaled.y>0.0||(scaled.y==0.0&&(scaled.z>0.0||(scaled.z==0.0&&(n&1)!=0))))))++n;
    int tableIndex=n&63,scale=(n-tableIndex)/64;
    r=tripleAdd(z,-tripleMul(vec3(float(n),0,0),powerStep3));
    poly=tripleAdd(tripleMul(oneSixth3,r),vec3(0.5,0,0));
    poly=tripleAdd(tripleMul(poly,r),vec3(1,0,0));
    precise vec3 value=tripleAdd(tripleMul(tripleMul(r,poly),powerExp3[tableIndex]),powerExp3[tableIndex]);
    bits=floatBitsToUint(value.x);
    float halfUlp=uintBitsToFloat((bits&0x7f800000u)-24u*0x800000u);
    if(abs(value.y)==halfUlp&&value.z!=0.0&&((value.y>0.0)==(value.z>0.0))) {
        if(value.y>0.0)++bits;else --bits;
    }
    int e=int((bits>>23)&255u)-127+scale;
    if(e>=-126&&e<=127)return uintBitsToFloat((uint(e+127)<<23)|(bits&0x7fffffu));
    // The caller restricts fallback to normal finite output.
    return value.x;
}
