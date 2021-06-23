#!/usr/bin/bash

# Test main program 

./force_cover examples/example.cc -- > testing/observed_example.cc
diff testing/observed_example.cc testing/expected_outputs/example.cc
./force_cover examples/header_file_with_templates.hpp -- -language c++ -std=c++14 > testing/header_file_with_templates.hpp || true
diff testing/header_file_with_templates.hpp testing/expected_outputs/header_file_with_templates.hpp
clang++ testing/observed_example.cc -fprofile-instr-generate -fcoverage-mapping -o testing/observed_example
cd testing && ./observed_example
llvm-profdata merge default.profraw -o default.profdata
llvm-cov show observed_example -instr-profile=default.profdata > example_coverage.txt
cd ..
coverage run -a fix_coverage.py testing/example_coverage.txt
diff testing/example_coverage.txt testing/expected_outputs/example_coverage.txt
cp examples/coverage_report_with_macros.txt testing
coverage run -a fix_coverage.py testing/coverage_report_with_macros.txt
diff testing/coverage_report_with_macros.txt testing/expected_outputs/coverage_report_with_macros.txt
cp examples/coverage_report_with_templates.txt testing
coverage run -a fix_coverage.py testing/coverage_report_with_templates.txt
diff testing/coverage_report_with_templates.txt testing/expected_outputs/coverage_report_with_templates.txt

# Test usage message

result=$(coverage run -a fix_coverage.py)
if [ "$result" != "Usage: python fix_coverage.py [name_of_coverage_report_file]" ];
then 
    exit 1; 
fi