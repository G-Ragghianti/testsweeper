#include <complex>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "example.hh"

// -----------------------------------------------------------------------------
using libtest::ParamType;
using libtest::DataType;
using libtest::char2datatype;
using libtest::datatype2char;
using libtest::datatype2str;
using libtest::ansi_bold;
using libtest::ansi_red;
using libtest::ansi_normal;
using libtest::get_wtime;

// -----------------------------------------------------------------------------
enum Section {
    newline = 0,  // zero flag forces newline
    level1,
    level2,
    level3,
};

const char* section_names[] = {
   "",  // none
   "Level 1",
   "Level 2",
   "Level 3",
};

// { "", nullptr, Section::newline } entries force newline in help
std::vector< libtest::routines_t > routines = {
    // Level 1
    { "foo",    test_foo,       Section::level1 },
    { "foo2",   test_foo,       Section::level1 },
    { "foo3",   test_foo,       Section::level1 },
    { "foo4",   test_foo,       Section::level1 },
    { "foo5",   test_foo,       Section::level1 },
    { "foo6",   test_foo,       Section::level1 },
    { "foo7",   test_foo,       Section::level1 },
    { "foo8",   test_foo,       Section::level1 },

    // Level 2
    { "bar",    test_bar,       Section::level2 },
    { "bar2",   test_bar,       Section::level2 },
    { "bar3",   test_bar,       Section::level2 },
    { "",       nullptr,        Section::newline },

    { "bar4",   test_bar,       Section::level2 },
    { "bar5",   test_bar,       Section::level2 },
    { "bar6",   test_bar,       Section::level2 },

    // Level 3
    { "baz",    test_baz,       Section::level3 },
    { "baz2",   test_baz,       Section::level3 },
    { "baz3",   test_baz,       Section::level3 },
    { "",       nullptr,        Section::newline },

    { "baz4",   test_baz,       Section::level3 },
    { "baz5",   test_baz,       Section::level3 },
};

// -----------------------------------------------------------------------------
// Params class
// List of parameters

Params::Params():
    ParamsBase(),

    // w = width
    // p = precision
    // def = default
    // ----- test framework parameters
    //         name,       w,    type,             def, valid, help
    check     ( "check",   0,    ParamType::Value, 'y', "ny",  "check the results" ),
    ref       ( "ref",     0,    ParamType::Value, 'n', "ny",  "run reference; sometimes check -> ref" ),

    //          name,      w, p, type,             def, min,  max, help
    tol       ( "tol",     0, 0, ParamType::Value,  50,   1, 1000, "tolerance (e.g., error < tol*epsilon to pass)" ),
    repeat    ( "repeat",  0,    ParamType::Value,   1,   1, 1000, "times to repeat each test" ),
    verbose   ( "verbose", 0,    ParamType::Value,   0,   0,   10, "verbose level" ),
    cache     ( "cache",   0,    ParamType::Value,  20,   1, 1024, "total cache size, in MiB" ),

    // ----- routine parameters
    //          name,      w,    type,            def,              char2enum,     enum2char,     enum2str,     help
    datatype  ( "type",    4,    ParamType::List, DataType::Double, char2datatype, datatype2char, datatype2str, "s=single (float), d=double, c=complex-single, z=complex-double" ),

    //          name,      w, p, type,            def,   min,     max, help
    dim       ( "dim",     6,    ParamType::List,          0, 1000000, "m x n x k dimensions" ),

    // ----- output parameters
    // min, max are ignored
    //          name,                  w, p, type,              def, min, max, help
    error     ( "SLATE\nerror",       11, 4, ParamType::Output, nan,   0,   0, "numerical error" ),
    ortho     ( "SLATE\north. error", 11, 4, ParamType::Output, nan,   0,   0, "orthogonality error" ),
    time      ( "SLATE\ntime (s)",    11, 4, ParamType::Output, nan,   0,   0, "time to solution" ),
    gflops    ( "SLATE\nGflop/s",     11, 4, ParamType::Output, nan,   0,   0, "Gflop/s rate" ),

    ref_error ( "Ref.\nerror",        11, 4, ParamType::Output, nan,   0,   0, "reference numerical error" ),
    ref_ortho ( "Ref.\north. error",  11, 4, ParamType::Output, nan,   0,   0, "reference orthogonality error" ),
    ref_time  ( "Ref.\ntime (s)",     11, 4, ParamType::Output, nan,   0,   0, "reference time to solution" ),
    ref_gflops( "Ref.\nGflop/s",      11, 4, ParamType::Output, nan,   0,   0, "reference Gflop/s rate" ),

    // default -1 means "no check"
    okay      ( "status",              6,    ParamType::Output,  -1,   0,   0, "success indicator" )
{
    // mark standard set of output fields as used
    okay  .value();
    error .value();
    time  .value();
    gflops.value();

    // mark framework parameters as used, so they will be accepted on the command line
    check  .value();
    tol    .value();
    repeat .value();
    verbose.value();
    cache  .value();

    // routine's parameters are marked by the test routine; see main
}

// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
    using libtest::QuitException;

    int status = 0;
    try {
        // print input so running `test [input] > out.txt` documents input
        printf( "input: %s", argv[0] );
        for (int i = 1; i < argc; ++i) {
            printf( " %s", argv[i] );
        }
        printf( "\n" );

        // Usage: test routine [params]
        if (argc < 2 ||
            strcmp( argv[1], "-h" ) == 0 ||
            strcmp( argv[1], "--help" ) == 0)
        {
            usage( argc, argv, routines, section_names );
            throw QuitException();
        }

        // find routine to test
        const char* routine = argv[1];
        libtest::test_func_ptr test_routine = find_tester( routine, routines );
        if (test_routine == nullptr) {
            usage( argc, argv, routines, section_names );
            throw std::runtime_error(
                std::string("routine ") + routine + " not found" );
        }

        // mark fields that are used (run=false)
        Params params;
        test_routine( params, false );

        // parse parameters after routine name
        try {
            params.parse( routine, argc-2, argv+2 );
        }
        catch (const std::exception& ex) {
            params.help( routine );
            throw;
        }

        // run tests
        int repeat = params.repeat.value();
        libtest::DataType last = params.datatype.value();
        params.header();
        do {
            if (params.datatype.value() != last) {
                last = params.datatype.value();
                printf( "\n" );
            }
            for (int iter = 0; iter < repeat; ++iter) {
                try {
                    test_routine( params, true );
                }
                catch (const std::exception& ex) {
                    fprintf( stderr, "%s%sError: %s%s\n",
                             ansi_bold, ansi_red, ex.what(), ansi_normal );
                    params.okay.value() = false;
                }

                params.print();
                status += ! params.okay.value();
                params.reset_output();
            }
            if (repeat > 1) {
                printf( "\n" );
            }
        } while( params.next() );

        if (status) {
            printf( "%d tests FAILED.\n", status );
        }
        else {
            printf( "All tests passed.\n" );
        }
    }
    catch (const QuitException& ex) {
        // pass: no error to print
    }
    catch (const std::exception& ex) {
        fprintf( stderr, "\n%s%sError: %s%s\n",
                 ansi_bold, ansi_red, ex.what(), ansi_normal );
        status = -1;
    }

    return status;
}

// -----------------------------------------------------------------------------
// traits class maps data type to norm_t and scalar_t.

// for float, double:
// norm and scalar are float, double, respectively.
template< typename T >
class traits
{
public:
    typedef T norm_t;
    typedef T scalar_t;
};

// for std::complex<float>, std::complex<double>:
// norm   is float, double, respectively;
// scalar is std::complex<float>, std::complex<double>, respectively.
template< typename T >
class traits< std::complex<T> >
{
public:
    typedef T norm_t;
    typedef std::complex<T> scalar_t;
};

// -----------------------------------------------------------------------------
template< typename T >
void test_foo_work( Params &params, bool run )
{
    typedef typename traits<T>::norm_t norm_t;

    // get & mark input and non-standard output values
    int64_t n = params.dim.n();
    int64_t cache = params.cache.value();
    bool check = (params.check.value() == 'y');
    bool ref = (params.ref.value() == 'y');

    // mark non-standard output values
    params.ref_time.value();
    params.ref_gflops.value();

    // adjust header to msec
    params.time.name( "SLATE\ntime (ms)" );
    params.ref_time.name( "Ref.\ntime (ms)" );

    if (! run)
        return;

    // ----------
    // setup
    double time;
    double gflop = 2*n;

    // run test
    libtest::flush_cache( cache );
    time = get_wtime();
    usleep( 100*n );  // placeholder; 0.0001 n sec
    time = get_wtime() - time;
    params.time.value()   = time * 1000;  // msec
    params.gflops.value() = gflop / time;

    if (ref) {
        // run LAPACK
        libtest::flush_cache( cache );
        time = get_wtime();
        usleep( 200*n );  // placeholder; 0.0002 n sec
        time = get_wtime() - time;
        params.ref_time.value()   = time * 1000;  // msec
        params.ref_gflops.value() = gflop / time;
    }

    // check error
    if (check) {
        norm_t error = 1.23456e-17 * n;  // placeholder
        norm_t eps = std::numeric_limits< norm_t >::epsilon();
        norm_t tol = params.tol.value() * eps;
        params.error.value() = error;
        params.okay.value()  = (error < tol);
    }
}

// -----------------------------------------------------------------------------
void test_foo( Params &params, bool run )
{
    switch (params.datatype.value()) {
        case libtest::DataType::Integer:
            test_foo_work< float >( params, run );
            break;

        case libtest::DataType::Single:
            test_foo_work< float >( params, run );
            break;

        case libtest::DataType::Double:
            test_foo_work< double >( params, run );
            break;

        case libtest::DataType::SingleComplex:
            test_foo_work< std::complex<float> >( params, run );
            break;

        case libtest::DataType::DoubleComplex:
            test_foo_work< std::complex<double> >( params, run );
            break;
    }
}

// -----------------------------------------------------------------------------
void test_bar( Params &params, bool run )
{
    test_foo( params, run );
}

// -----------------------------------------------------------------------------
void test_baz( Params &params, bool run )
{
    test_foo( params, run );
}
