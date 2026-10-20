// Float log10 graph with AMD's MIT-licensed log10_128 tables. See SOURCES.json.
float log10Fma(float a,float b,float c) {
    precise vec2 result=pairAdd(pairMul(vec2(a,0),vec2(b,0)),vec2(c,0));
    return result.x+result.y;
}
float accurateLog10(float x) {
    precise float f=x-1.0;
    if(abs(f)<0.0625) {
        precise float denominator=2.0+f;
        precise float u=nearOneRatio(f,denominator),correction=f*u;
        u=u+u;
        precise float square=u*u,cube=u*square;
        precise float poly=log10Fma(square,1.0/80.0,1.0/12.0);
        precise float tail=log10Fma(poly,cube,-correction);
        float high=uintBitsToFloat(floatBitsToUint(f)&0xffff0000u);
        precise float low=f-high;
        low=tail+low;
        precise float a=low*log10ELead,b=high*log10ETail,c=high*log10ELead;
        b=log10Fma(low,log10ETail,b);
        b=b+a;
        return b+c;
    }
    uint bits=floatBitsToUint(x),mantissa=bits&0x7fffffu;
    precise float exponent=float(int(bits>>23)-127);
    uint index=(mantissa>>16)+((mantissa>>15)&1u);
    float m=uintBitsToFloat(mantissa|0x3f000000u);
    float center=uintBitsToFloat((index<<16)|0x3f000000u);
    precise float difference=center-m,r=difference*inverseTable[index];
    precise float poly=log10Fma(1.0/3.0,r,0.5),square=r*r;
    precise float q=log10Fma(poly,square,r);
    q=q*log10E;
    precise float low=log10Fma(log10TwoTail,exponent,-q);
    precise float high=log10Fma(log10TwoLead,exponent,log10Lead[index]);
    low=low+log10Tail[index];
    return high+low;
}
