// Original IQM finite binary32 Euclidean norm, rounded nearest/ties-to-even.
// Inputs: zero or normal floats with unbiased exponent[-90,100]. No sqrt,
// floating intermediate products, Float64 or Int64 capability is needed.
uvec3 norm_square(uint m,uint shift) {
    uint hi,lo; umulExtended(m,m,hi,lo);
    if(shift==0u) return uvec3(lo,hi,0u);
    return uvec3(lo<<shift,(hi<<shift)|(lo>>(32u-shift)),hi>>(32u-shift));
}
uvec3 norm_add(uvec3 a,uvec3 b) {
    uint low=a.x+b.x,carry=uint(low<a.x);
    uint middle=a.y+b.y,upper=uint(middle<a.y);
    uint next=middle+carry; upper+=uint(next<middle);
    return uvec3(low,next,a.z+b.z+upper);
}
int norm_compare(uvec3 a,uvec3 b) {
    if(a.z!=b.z) return a.z>b.z?1:-1;
    if(a.y!=b.y) return a.y>b.y?1:-1;
    if(a.x!=b.x) return a.x>b.x?1:-1;
    return 0;
}
float finite_hypot(float x,float y) {
    uint a=floatBitsToUint(x)&0x7fffffffu,b=floatBitsToUint(y)&0x7fffffffu;
    if(a<b) {uint t=a;a=b;b=t;}
    if(b==0u) return uintBitsToFloat(a);
    uint ea=a>>23,eb=b>>23,d=ea-eb;
    // b/a<2^-13: the norm lies strictly below the first midpoint above a.
    if(d>=14u) return uintBitsToFloat(a);
    uint ma=(a&0x7fffffu)|0x800000u,mb=(b&0x7fffffu)|0x800000u;
    // In units of 2^(eb-127-23), compare 4*(a*a+b*b) with
    // (2*mantissa(candidate)+1)^2 * 2^(2*(exponent(candidate)-eb)).
    // Maximal shift28 and25-bit midpoint significands fit within96 bits.
    uvec3 sum=norm_add(norm_square(ma,2u*d+2u),norm_square(mb,2u));
    uint low=a,high=a+0x800000u;
    while(low<high) {
        uint candidate=low+(high-low)/2u;
        uint midpoint=2u*((candidate&0x7fffffu)|0x800000u)+1u;
        int order=norm_compare(sum,norm_square(midpoint,2u*((candidate>>23)-eb)));
        if(order==0) return uintBitsToFloat(candidate+(candidate&1u));
        if(order>0) low=candidate+1u; else high=candidate;
    }
    return uintBitsToFloat(low);
}
