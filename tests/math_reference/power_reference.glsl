// Positive finite native-style power approximation. Tables are generated from
// log(1+j/256), 2/(1+j/256), 2^(j/64); near1 uses an atanh series.
// Matching a native implementation is a tested property, not an IEEE guarantee.
float nativePower(float x,float y) {
    if((floatBitsToUint(x)&0x7fffffffu)==0u) return 0.0;
    precise vec2 logarithm;
    precise float f=x-1.0;
    bool near=abs(f)<0.0625;
    if(near) {
        precise vec2 v=pairDiv(vec2(f,0),pairSum(2.0,f));
        precise vec2 correction=pairMul(v,vec2(f,0));
        precise vec2 u=pairAdd(v,v),square=pairMul(u,u),cube=pairMul(u,square);
        precise vec2 a=pairAdd(pairMul(square,nearA),nearC);
        precise vec2 b=pairAdd(pairMul(square,nearB),nearD);
        precise vec2 tail=pairAdd(pairMul(cube,a),pairMul(pairMul(square,square),pairMul(cube,b)));
        logarithm=pairAdd(vec2(f,0),pairAdd(tail,-correction));
    } else {
        uint bits=floatBitsToUint(x),mantissa=bits&0x7fffffu;
        int exponent=int(bits>>23)-127;
        if((bits&0x7f800000u)==0u) {
            int shift=23-findMSB(mantissa);
            mantissa=(mantissa<<uint(shift))&0x7fffffu;
            exponent=-126-shift;
        }
        uint index=(mantissa>>15)+((mantissa>>14)&1u);
        float m=uintBitsToFloat(mantissa|0x3f000000u);
        float center=0.5+float(index)*(1.0/512.0);
        precise vec2 r=pairMul(vec2(center-m,0),powerInverse[index]);
        precise vec2 poly=pairAdd(pairMul(oneThird,r),vec2(0.5,0));
        poly=pairAdd(pairMul(poly,r),vec2(1,0));
        precise vec2 q=pairMul(r,poly);
        logarithm=pairAdd(pairAdd(pairMul(vec2(float(exponent),0),powerLn2),powerLog[index]),-q);
    }
    precise vec2 z=pairMul(vec2(y,0),logarithm);
    // Native finite-path range guards precede final binary32 rounding. In
    // particular its underflow cutoff is ln(min-subnormal), not ln(half-min).
    if(z.x < -103.2789306640625 || (z.x == -103.2789306640625 && z.y<=0.0)) return 0.0;
    if(z.x > 88.72283935546875 || (z.x == 88.72283935546875 && z.y>0.0)) return uintBitsToFloat(0x7f800000u);
    precise vec2 scaled=pairMul(z,powerMultiplier);
    int n=int(floor(scaled.x)); float fraction=scaled.x-float(n);
    if(fraction>0.5 || (fraction==0.5 && (scaled.y>0.0 || (scaled.y==0.0 && (n&1)!=0)))) ++n;
    int index=n&63,exponent=(n-index)/64;
    precise vec2 r=pairAdd(z,-pairMul(vec2(float(n),0),powerStep));
    precise vec2 poly=pairAdd(pairMul(oneSixth,r),vec2(0.5,0));
    poly=pairAdd(pairMul(poly,r),vec2(1,0));
    precise vec2 value=pairAdd(pairMul(pairMul(r,poly),powerExp[index]),powerExp[index]);
    uint bits=floatBitsToUint(value.x);
    int e=int((bits>>23)&255u)-127+exponent;
    if(!near&&e>=-126&&e<=127) {
        float ulp=uintBitsToFloat((bits&0x7f800000u)-23u*0x800000u);
        if(abs(abs(value.y)-0.5*ulp)<ulp*(1.0/65536.0))return nativePowerFarPrecise(x,y);
    }
    if(e>127) return uintBitsToFloat(0x7f800000u);
    if(e>=-126) return uintBitsToFloat((uint(e+127)<<23)|(bits&0x7fffffu));
    int shift=-126-e;
    if(shift>24) return 0.0;
    uint mantissa=(bits&0x7fffffu)|0x800000u;
    uint lost=mantissa&((1u<<uint(shift))-1u),midpoint=1u<<uint(shift-1);
    uint rounded=mantissa>>uint(shift);
    if(lost>midpoint || (lost==midpoint && (value.y>0.0 || (value.y==0.0 && (rounded&1u)!=0u)))) ++rounded;
    return uintBitsToFloat(rounded);
}

