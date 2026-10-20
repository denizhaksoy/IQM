// SPDX-License-Identifier: MIT
// Graphs adapted from AMD win-libm (2002-2019 Advanced Micro Devices, Inc.).
// See amd_win_libm/LICENSE.txt and original assembly sources.
// Compensated float-pair helpers and GLSL adaptation for IQM numerical tests.
vec2 pairSum(float a,float b) {
    precise float s=a+b, v=s-a;
    precise float e=(a-(s-v))+(b-v);
    return vec2(s,e);
}
vec2 pairAdd(vec2 a,vec2 b) {
    precise vec2 s=pairSum(a.x,b.x);
    precise float e=s.y+a.y+b.y;
    return pairSum(s.x,e);
}
vec2 pairMul(vec2 a,vec2 b) {
    precise float hi=a.x*b.x;
    precise float ca=4097.0*a.x,cb=4097.0*b.x;
    precise float ah=ca-(ca-a.x),al=a.x-ah;
    precise float bh=cb-(cb-b.x),bl=b.x-bh;
    precise float residual=((ah*bh-hi)+ah*bl+al*bh)+al*bl;
    precise float lo=residual+a.x*b.y+a.y*b.x;
    return pairSum(hi,lo);
}
vec2 pairDiv(vec2 a,vec2 b) {
    precise float q=a.x/b.x;
    precise vec2 r=pairAdd(a,-pairMul(vec2(q,0),b));
    precise float correction=(r.x+r.y)/b.x;
    return pairSum(q,correction);
}
// Exact ties-to-even division for the normal finite operands in log's near1
// branch. Long division avoids both relaxed GPU division and midpoint loss in
// a finite compensated expansion. No subnormal quotient occurs in this branch.
float nearOneRatio(float a,float b) {
    uint ua=floatBitsToUint(a),ub=floatBitsToUint(b);
    if((ua&0x7fffffffu)==0u) return a;
    int e=int((ua>>23)&255u)-int((ub>>23)&255u);
    uint am=(ua&0x7fffffu)|0x800000u,bm=(ub&0x7fffffu)|0x800000u;
    if(am<bm) {am<<=1; --e;}
    uint remainder=am-bm,quotient=0x800000u;
    for(int bit=22;bit>=0;--bit) {
        remainder<<=1;
        if(remainder>=bm) {remainder-=bm; quotient|=1u<<uint(bit);}
    }
    uint twice=remainder<<1;
    if(twice>bm || (twice==bm && (quotient&1u)!=0u)) ++quotient;
    if(quotient==0x1000000u) {quotient>>=1; ++e;}
    return uintBitsToFloat(((ua^ub)&0x80000000u)|(uint(e+127)<<23)|(quotient&0x7fffffu));
}
float accurateLog(float x) {
    precise float f=x-1.0;
    if(abs(f)<nearThreshold) {
        precise float denominator=2.0+f;
        precise float u=nearOneRatio(f,denominator);
        precise float correction=f*u;
        u=u+u;
        precise float square=u*u,cube=u*square;
        precise float poly=fma(square,nearC0,nearC1);
        precise float tail=fma(poly,cube,-correction);
        return f+tail;
    }
    uint bits=floatBitsToUint(x),mantissa=bits&0x7fffffu;
    precise float exponent=float(int(bits>>23)-127);
    uint index=(mantissa>>16)+((mantissa>>15)&1u);
    precise float m=uintBitsToFloat(mantissa|0x3f000000u);
    precise float center=uintBitsToFloat((index<<16)|0x3f000000u);
    precise float difference=center-m;
    precise float r=difference*inverseTable[index];
    precise float poly=fma(polyC0,r,polyC1),square=r*r;
    precise float q=fma(poly,square,r);
    precise float low=fma(log2Tail,exponent,-q);
    precise float high=fma(log2Lead,exponent,leadTable[index]);
    low=low+tailTable[index];
    return high+low;
}
float accurateExp(float x) {
    precise vec2 q=pairMul(vec2(x,0),expMultiplier);
    int n=int(floor(q.x));
    precise float fraction=q.x-float(n);
    if(fraction>0.5 || (fraction==0.5 && (q.y>0.0 || (q.y==0.0 && (n&1)!=0)))) ++n;
    int index=n&63,exponent=(n-index)/64;
    precise vec2 r=pairAdd(vec2(x,0),-pairMul(vec2(float(n),0),expStep));
    precise vec2 poly=pairAdd(pairMul(expC0,r),expC1);
    precise vec2 tail=pairAdd(pairMul(pairMul(r,r),poly),r);
    precise vec2 value=pairAdd(pairMul(tail,expTable[index]),expTable[index]);
    precise float rounded=value.x+value.y;
    return rounded*uintBitsToFloat(uint(exponent+127)<<23);
}
