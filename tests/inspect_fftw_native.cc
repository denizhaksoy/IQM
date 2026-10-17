// Diagnostic Octave extension: print the exact plan used by its native wrapper.
// No copied Octave/FFTW implementation; only calls public installed interfaces.
#include <octave/oct.h>
#include <octave/oct-fftw.h>
#include <fftw3.h>
#include <cstdlib>
#include <cstdint>

DEFUN_DLD(inspect_fftw_native, args, , "Inspect native single FFTW plan") {
    if (args.length()!=2) print_usage();
    const bool inverse=args(1).bool_value();
    octave_value_list result;
    void* plan=nullptr;
    if (!inverse) {
        const FloatNDArray input=args(0).float_array_value();
        const dim_vector dims=input.dims();
        if (dims.ndims()!=2) error("Only 2D inputs supported");
        FloatComplexNDArray output(dims);
        octave::fftw::fftNd(input.data(),output.rwdata(),2,dims);
        const auto offset=(dims.numel()/dims(0))*((dims(0)-1)/2);
        plan=octave::float_fftw_planner::create_plan(2,dims,1,1,dims.numel(),
            input.data(),output.rwdata()+offset);
        result(0)=output;
    } else {
        const FloatComplexNDArray input=args(0).float_complex_array_value();
        const dim_vector dims=input.dims();
        if (dims.ndims()!=2) error("Only 2D inputs supported");
        FloatComplexNDArray output(dims);
        octave::fftw::ifftNd(input.data(),output.rwdata(),2,dims);
        plan=octave::float_fftw_planner::create_plan(FFTW_BACKWARD,2,dims,1,1,
            dims.numel(),input.data(),output.rwdata());
        result(0)=output;
    }
    if(!plan) error("Native FFTW plan not available");
    char* printed=fftwf_sprint_plan(reinterpret_cast<fftwf_plan>(plan));
    if(!printed) error("Cannot print FFTW plan");
    result(1)=std::string(printed); std::free(printed);
    double adds,muls,fmas;
    fftwf_flops(reinterpret_cast<fftwf_plan>(plan),&adds,&muls,&fmas);
    RowVector counts(3); counts(0)=adds; counts(1)=muls; counts(2)=fmas;
    result(2)=counts;
    result(3)=octave::float_fftw_planner::threads();
    return result;
}
