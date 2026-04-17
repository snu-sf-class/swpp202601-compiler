#include "args.h"
#include "lib.h"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  auto argparse_result = scargs::parse(argc, argv);
  if (!argparse_result.has_value()) {
    const auto err = argparse_result.error();
    std::cerr << err.what() << "\n";
    return 1;
  }

  auto args = argparse_result.value();
  auto compile_result = sc::compile(args.input_filename, args.output_filename,
                                    args.verbose_printing);
  return 0;
}
