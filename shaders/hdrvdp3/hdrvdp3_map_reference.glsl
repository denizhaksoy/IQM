#version 450
layout(local_size_x=64) in;
layout(constant_id=31) const uint map_abi=13u;
layout(std430,binding=0) readonly buffer Input { uint words[]; } input_data;
layout(std430,binding=1) writeonly buffer Output { uint words[]; } output_data;
layout(push_constant) uniform Push { uint count,mode,width,height,out_width,out_height,taps,plan_offset; } pc;
float load_float(uint i) { return uintBitsToFloat(input_data.words[i]); }
// Unsigned 64-bit helpers; x=low word, y=high word. A jammed right shift
// retains a sticky bit for exact ties-to-even rounding after positive addition.
uvec2 shift_jam(uvec2 v,int shift) {
    if((v.x|v.y)==0u) return v;
    if(shift<=0) {
        uint s=uint(-shift);
        if(s==0u) return v;
        if(s<32u) return uvec2(v.x<<s,(v.y<<s)|(v.x>>(32u-s)));
        return uvec2(0u,v.x<<(s-32u));
    }
    if(shift>=64) return uvec2(1u,0u);
    uint s=uint(shift);
    if(s<32u) return uvec2((v.x>>s)|(v.y<<(32u-s))|uint((v.x&((1u<<s)-1u))!=0u),v.y>>s);
    if(s==32u) return uvec2(v.y|uint(v.x!=0u),0u);
    s-=32u;
    return uvec2((v.y>>s)|uint(v.x!=0u || (v.y&((1u<<s)-1u))!=0u),0u);
}
uint round_word(uvec2 v,int shift) {
    if(shift<=0) return v.x<<uint(-shift);
    if(shift>64) return 0u;
    if(shift==64) return uint(v.y>0x80000000u || (v.y==0x80000000u && v.x!=0u));
    uint top,remainder,midpoint; bool sticky=false;
    if(shift<32) {
        top=(v.x>>uint(shift))|(v.y<<uint(32-shift));
        remainder=v.x&((1u<<uint(shift))-1u); midpoint=1u<<uint(shift-1);
    } else if(shift==32) {
        top=v.y; remainder=v.x; midpoint=0x80000000u;
    } else {
        uint s=uint(shift-32); top=v.y>>s;
        remainder=v.y&((1u<<s)-1u); midpoint=1u<<(s-1u); sticky=v.x!=0u;
    }
    if(remainder>midpoint || (remainder==midpoint && (sticky || (top&1u)!=0u))) ++top;
    return top;
}
float positive_fma(float a,float b,float c) {
    uint ua=floatBitsToUint(a),ub=floatBitsToUint(b),uc=floatBitsToUint(c);
    uint am=ua&0x7fffffu,bm=ub&0x7fffffu,cm=uc&0x7fffffu;
    int ae=int((ua>>23)&255u),be=int((ub>>23)&255u),ce=int((uc>>23)&255u);
    if(ae!=0) am|=0x800000u;
    if(be!=0) bm|=0x800000u;
    if(ce!=0) cm|=0x800000u;
    if(am==0u||bm==0u) return c;
    int ep=max(ae,1)+max(be,1)-254-46;
    int ec=max(ce,1)-127-23;
    uvec2 product; umulExtended(am,bm,product.y,product.x);
    int pt=product.y!=0u?32+findMSB(product.y):findMSB(product.x);
    int ct=cm!=0u?findMSB(cm):-10000;
    int emax=max(ep+pt,ec+ct),unit=emax-62;
    uvec2 p=shift_jam(product,unit-ep),s=shift_jam(uvec2(cm,0u),unit-ec);
    uint carry; uvec2 sum; sum.x=uaddCarry(p.x,s.x,carry); sum.y=p.y+s.y+carry;
    int top=sum.y!=0u?32+findMSB(sum.y):findMSB(sum.x),e=unit+top;
    if(e < -126) return uintBitsToFloat(round_word(sum,-149-unit));
    uint mantissa=round_word(sum,top-23);
    if(mantissa>=0x1000000u) {mantissa>>=1; ++e;}
    if(e>127) return uintBitsToFloat(0x7f800000u);
    return uintBitsToFloat((uint(e+127)<<23)|(mantissa&0x7fffffu));
}
// Finite IEEE division for resize's positive, nonzero normalization.
float divide_finite(float a,float b) {
    uint ua=floatBitsToUint(a),ub=floatBitsToUint(b),sign=(ua^ub)&0x80000000u;
    if((ub&0x7fffffffu)==0x7f800000u) return uintBitsToFloat(sign);
    uint am=ua&0x7fffffu,bm=ub&0x7fffffu;
    int ae=int((ua>>23)&255u),be=int((ub>>23)&255u);
    if((ua&0x7fffffffu)==0u) return uintBitsToFloat(sign);
    if(ae==0) { int shift=23-findMSB(am); am<<=shift; ae=1-shift; } else am|=0x800000u;
    if(be==0) { int shift=23-findMSB(bm); bm<<=shift; be=1-shift; } else bm|=0x800000u;
    int e=ae-be;
    if(am<bm) { am<<=1; --e; }
    uint remainder=am-bm,quotient=0x800000u;
    for(int bit=22;bit>=0;--bit) {
        remainder<<=1;
        if(remainder>=bm) { remainder-=bm; quotient|=1u<<uint(bit); }
    }
    if(e < -126) {
        int shift=-126-e;
        if(shift>24) return uintBitsToFloat(sign);
        uint lost=quotient&((1u<<uint(shift))-1u),midpoint=1u<<uint(shift-1);
        quotient>>=uint(shift);
        if(lost>midpoint || (lost==midpoint && (remainder!=0u || (quotient&1u)!=0u))) ++quotient;
        return uintBitsToFloat(sign|quotient);
    }
    uint twice=remainder<<1;
    if(twice>bm || (twice==bm && (quotient&1u)!=0u)) ++quotient;
    if(quotient==0x1000000u) { quotient>>=1; ++e; }
    if(e>127) return uintBitsToFloat(sign|0x7f800000u);
    return uintBitsToFloat(sign|(uint(e+127)<<23)|(quotient&0x7fffffu));
}
// Generator injects the positive power and exact product helpers here.
float exact_sqrt(float x) {
    uint bits=floatBitsToUint(x),m=bits&0x7fffffu;
    if((bits&0x7fffffffu)==0x7f800000u) return x;
    int e=int((bits>>23)&255u)-127;
    if((bits&0x7fffffffu)==0u) return x;
    if(e==-127) { int shift=23-findMSB(m); m<<=shift; e=-126-shift; }
    else m|=0x800000u;
    if((e&1)!=0) {m<<=1; --e;}
    uvec2 n=uvec2(m<<23,m>>9);
    uint root=0u;
    for(int b=23;b>=0;--b) {
        uint candidate=root|(1u<<uint(b)),hi,lo;
        umulExtended(candidate,candidate,hi,lo);
        if(hi<n.y || (hi==n.y && lo<=n.x)) root=candidate;
    }
    uint hi,lo,borrow; umulExtended(root,root,hi,lo);
    uint remainder=usubBorrow(n.x,lo,borrow),remainder_hi=n.y-hi-borrow;
    if(remainder_hi!=0u || remainder>root) ++root;
    if(root>=0x1000000u) {root>>=1; e+=2;}
    return uintBitsToFloat((uint(e/2+127)<<23)|(root&0x7fffffu));
}
float pupil_diameter(float luminance,float area,float age_correction) {
    float x=nativePower(divide_finite(exactProduct(luminance,area),846.0),0.41);
    precise float denominator=x+2.0;
    precise float sd=7.75-exactProduct(5.75,divide_finite(x,denominator));
    precise float factor=0.02132-exactProduct(0.009562,sd);
    precise float correction=exactProduct(age_correction,factor);
    return sd+correction;
}
void main() {
    if(map_abi!=13u) return;
    uint i=gl_GlobalInvocationID.x; if(i>=pc.count) return;
    precise float result;
    if(pc.mode==23u) {
        uint x=i%pc.out_width,y=i/pc.out_width; int sum=0;
        for(uint t=0u;t<pc.taps;++t) {
            uint p=pc.plan_offset+2u*(x*pc.taps+t);
            sum+=int(input_data.words[y*pc.width+input_data.words[p]])*int(input_data.words[p+1u]);
        }
        output_data.words[i]=uint(sum); return;
    } else if(pc.mode==24u) {
        if(i>=pc.out_width*pc.out_height) return;
        uint x=i%pc.out_width,y=i/pc.out_width; uvec2 sum=uvec2(0u);
        for(uint t=0u;t<pc.taps;++t) {
            uint p=pc.plan_offset+2u*(y*pc.taps+t); int hi,lo;
            imulExtended(int(input_data.words[input_data.words[p]*pc.width+x]),int(input_data.words[p+1u]),hi,lo);
            uint lower=sum.x+uint(lo); sum=uvec2(lower,sum.y+uint(hi)+uint(lower<sum.x));
        }
        // Exact signed Q32 to IEEE binary64 bits; no floating conversion.
        uint sign=sum.y&0x80000000u;
        if(sign!=0u) {uint lower=~sum.x+1u;sum=uvec2(lower,~sum.y+uint(lower==0u));}
        uvec2 bits=uvec2(0u);
        if((sum.x|sum.y)!=0u) {
            int top=sum.y!=0u?32+findMSB(sum.y):findMSB(sum.x); uint shift=uint(52-top);
            uvec2 mantissa;
            if(shift==0u) mantissa=sum;
            else if(shift<32u) mantissa=uvec2(sum.x<<shift,(sum.y<<shift)|(sum.x>>(32u-shift)));
            else mantissa=uvec2(0u,sum.x<<(shift-32u));
            bits=uvec2(mantissa.x,(mantissa.y&0xfffffu)|(uint(top-32+1023)<<20)|sign);
        }
        output_data.words[2u*i]=bits.x; output_data.words[2u*i+1u]=bits.y; return;
    } else if(pc.mode==22u) {
        if(i>=pc.width) return;
        uint grid=pc.plan_offset,parameters=grid+7u,slopes=parameters+28u;
        float query=clamp(accurateLog10(load_float(i)),load_float(grid),load_float(grid+6u));
        uint lo=0u,hi=7u;
        while(lo<hi) {uint mid=(lo+hi)/2u;if(query<load_float(grid+mid))hi=mid;else lo=mid+1u;}
        uint lower=clamp(lo,1u,6u)-1u;
        precise float offset=query-load_float(grid+lower);
        output_data.words[5u*i]=floatBitsToUint(query);
        for(uint k=0u;k<4u;++k) {
            precise float product=exactProduct(load_float(slopes+4u*lower+k),offset);
            precise float parameter=product+load_float(parameters+4u*lower+k);
            output_data.words[5u*i+1u+k]=floatBitsToUint(parameter);
        }
        return;
    } else if(pc.mode==21u) {
        if(i!=0u) return;
        float maximum=0.0; uint selected=0u;
        for(uint x=0u;x<8u;++x) for(uint y=0u;y<8u;++y) {
            uint row=y*8u+x;
            float magnitude=row==0u?0.0:finite_hypot(load_float(2u*row),load_float(2u*row+1u));
            output_data.words[row]=floatBitsToUint(magnitude);
            if(magnitude>maximum) {maximum=magnitude;selected=x*8u+y;}
        }
        output_data.words[64]=selected;
        return;
    } else if(pc.mode==20u) {
        precise vec2 sum=vec2(0.0);
        for(uint k=0u;k<64u;++k) sum=pairAdd(sum,vec2(load_float(k),0.0));
        float mean=exactProduct(sum.x,1.0/64.0);
        result=i==0u?mean:nativePower(10.0,mean);
    } else if(pc.mode==17u) {
        float normalization=uintBitsToFloat(pc.taps);
        precise float r=divide_finite(load_float(i),normalization);
        precise float t=divide_finite(load_float(pc.plan_offset+i),normalization);
        result=t-r;
    } else if(pc.mode==18u) {
        precise float difference=load_float(pc.plan_offset+i)-load_float(i);
        result=divide_finite(abs(difference),load_float(i))>0.001?1.0:0.0;
    } else if(pc.mode==19u) {
        precise vec2 sum=vec2(0.0);
        for(uint k=0u;k<64u;++k) sum=pairAdd(sum,vec2(load_float(k),0.0));
        float mean=exactProduct(sum.x,1.0/64.0);
        result=load_float(i)-mean;
    } else if(pc.mode==15u) {
        if(i>=pc.width) return;
        uint grid=pc.plan_offset,cone=grid+2048u,rod=cone+2048u,cone_slope=rod+2048u,rod_slope=cone_slope+2047u;
        uint parameters=rod_slope+2047u;
        precise float channels[3];
        for(uint ch=0u;ch<3u;++ch) {
            float before=exactProduct(load_float(i),load_float(parameters+1u+ch));
            float after=exactProduct(clamp(before,1e-8,1e10),load_float(parameters));
            float query=accurateLog10(clamp(after,1e-5,1e5));
            uint lo=0u,hi=2048u;
            while(lo<hi) { uint middle=(lo+hi)/2u; if(query<load_float(grid+middle)) hi=middle; else lo=middle+1u; }
            uint lower=clamp(lo,1u,2047u)-1u;
            precise float offset=query-load_float(grid+lower);
            float slope=load_float((ch==2u?rod_slope:cone_slope)+lower);
            precise float product=exactProduct(slope,offset);
            channels[ch]=product+load_float((ch==2u?rod:cone)+lower);
            output_data.words[13u*i+ch]=floatBitsToUint(before);
            output_data.words[13u*i+3u+ch]=floatBitsToUint(after);
            output_data.words[13u*i+6u+ch]=floatBitsToUint(query);
            output_data.words[13u*i+9u+ch]=floatBitsToUint(channels[ch]);
        }
        precise float lm=channels[0]+channels[1];
        result=lm+channels[2]; output_data.words[13u*i+12u]=floatBitsToUint(result);
        return;
    } else if(pc.mode==16u) {
        result=accurateLog10(clamp(load_float(i),1e-5,1e5));
    } else if(pc.mode==12u) {
        if(i!=0u) return;
        precise float sum=0.0;
        for(uint x=0u;x<pc.width;++x) for(uint y=0u;y<pc.height;++y)
            sum=sum+accurateLog(load_float(y*pc.width+x));
        float geo=accurateExp(divide_finite(sum,float(pc.plan_offset)));
        float ref=pupil_diameter(geo,load_float(pc.plan_offset),load_float(pc.plan_offset+2u));
        float age=pupil_diameter(geo,load_float(pc.plan_offset),load_float(pc.plan_offset+1u));
        float reduction=divide_finite(exactProduct(age,age),exactProduct(ref,ref));
        output_data.words[0]=floatBitsToUint(geo);
        output_data.words[1]=floatBitsToUint(ref);
        output_data.words[2]=floatBitsToUint(age);
        output_data.words[3]=floatBitsToUint(reduction);
        return;
    } else if(pc.mode==13u) {
        precise float sum=load_float(i)+load_float(pc.plan_offset+i);
        result=exactProduct(0.5,sum);
    } else if(pc.mode==14u) {
        precise float l=exactProduct(load_float(i),load_float(pc.plan_offset));
        precise float m=exactProduct(load_float(i),load_float(pc.plan_offset+1u));
        result=l+m;
    } else if(pc.mode==10u) {
        precise float sum=0.0;
        for(uint x=0u;x<pc.width;++x) for(uint y=0u;y<pc.height;++y) {
            uint index=y*pc.width+x;
            float masked=exactProduct(load_float(index),load_float(pc.plan_offset+index));
            float power=nativePower(abs(masked),0.8);
            precise float term=divide_finite(power,float(pc.plan_offset));
            sum=sum+term;
        }
        result=nativePower(sum,1.25);
    } else if(pc.mode==11u) {
        if(i!=0u) return;
        precise float error=0.0;
        for(uint band=0u;band<pc.width;++band) {
            precise float term=divide_finite(load_float(band),float(pc.width));
            error=error+term;
        }
        precise float input_value=load_float(pc.width)+error;
        precise float quality=6.0-accurateLog(input_value);
        float loss=nativePower(max(10.0-quality,0.0),1.2812);
        precise float product=exactProduct(0.5200,loss),jod=10.0-product;
        output_data.words[0]=floatBitsToUint(error);
        output_data.words[1]=floatBitsToUint(quality);
        output_data.words[2]=floatBitsToUint(jod);
        return;
    } else if(pc.mode==6u) {
        uint parameters=2u*pc.plan_offset;
        float q=load_float(parameters);
        precise float a=exactProduct(load_float(parameters+1u),nativePower(abs(load_float(i)),q));
        precise float b=exactProduct(load_float(parameters+2u),nativePower(abs(load_float(pc.plan_offset+i)),q));
        result=a+b;
    } else if(pc.mode==7u) {
        precise float sum=0.0+max(load_float(i),0.0);
        result=sum+max(load_float(pc.plan_offset+i),0.0);
    } else if(pc.mode==8u) {
        uint parameters=3u*pc.plan_offset;
        float difference=load_float(i),power=load_float(parameters),neural=load_float(parameters+1u);
        float excitation=nativePower(abs(difference),power);
        excitation=uintBitsToFloat(floatBitsToUint(excitation)|(floatBitsToUint(difference)&0x80000000u));
        if(neural<0.0) neural=nativePower(divide_finite(1.0,load_float(pc.plan_offset+i)),2.0*power);
        float mask=load_float(2u*pc.plan_offset+i);
        precise float square=exactProduct(mask,mask),sum=neural+square;
        result=divide_finite(excitation,exact_sqrt(sum));
    } else if(pc.mode==9u) {
        float input_value=load_float(i);
        result=nativePower(abs(input_value),uintBitsToFloat(pc.taps));
        result=uintBitsToFloat(floatBitsToUint(result)|(floatBitsToUint(input_value)&0x80000000u));
    } else if(pc.mode==2u) {
        precise float q=clamp(load_float(i),load_float(pc.plan_offset),load_float(pc.plan_offset+255u));
        uint lo=0u,hi=256u;
        while(lo<hi) { uint middle=(lo+hi)/2u; if(q<load_float(pc.plan_offset+middle)) hi=middle; else lo=middle+1u; }
        uint lower=clamp(lo,1u,255u)-1u;
        precise float offset=q-load_float(pc.plan_offset+lower);
        precise float product=load_float(pc.plan_offset+512u+lower)*offset;
        result=product+load_float(pc.plan_offset+256u+lower);
    } else if(pc.mode==5u) {
        result=clamp(load_float(i),-5.0,5.0);
    } else if(pc.mode==3u) {
        precise float reference=abs(load_float(i))*uintBitsToFloat(pc.taps);
        precise float test=abs(load_float(pc.plan_offset+i))*uintBitsToFloat(pc.taps);
        result=min(reference,test);
    } else if(pc.mode==4u) {
        int x=int(i%pc.width),y=int(i/pc.width);
        precise float sum=0.0;
        for(int dx=-1;dx<=1;++dx) for(int dy=1;dy>=-1;--dy) {
            int sx=x+dx,sy=y+dy;
            if(sx>=0&&sy>=0&&sx<int(pc.width)&&sy<int(pc.height)) {
                float value=load_float(uint(sy)*pc.width+uint(sx));
                if(pc.height>=32u) sum=positive_fma(value,1.0/9.0,sum);
                else { precise float product=value*(1.0/9.0); sum=sum+product; }
            }
        }
        result=sum;
    } else {
        uint x=i%pc.out_width,y=i/pc.out_width;
        uint coordinate=pc.mode==0u?x:y;
        uint plan=pc.plan_offset+coordinate*(1u+2u*pc.taps);
        precise float sum=0.0;
        for(uint tap=0u;tap<pc.taps;++tap) {
            uint source=input_data.words[plan+1u+2u*tap];
            uint index=pc.mode==0u?y*pc.width+source:source*pc.width+x;
            precise float product=load_float(index)*load_float(plan+2u+2u*tap);
            sum=sum+product;
        }
        result=divide_finite(sum,load_float(plan));
    }
    output_data.words[i]=floatBitsToUint(result);
}
